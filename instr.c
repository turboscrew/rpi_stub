/*
 * instr.c
 *
 *  Created on: Apr 15, 2015
 *      Author: jaa
 */

/*
 * Stuff needed for finding out where to place traps in single stepping
 */

#include <stdint.h>
#include "instr.h"
#include "rpi2.h"
#include "ARM_decode_table.h"


// handle thumb instructions
instr_next_addr_t next_address_thumb(unsigned int address)
{
	instr_next_addr_t retval;
	unsigned int instr; // both half-words if 32-bit
	unsigned short *tptr; // thumb instruction pointer
	// not supported yet
	// either one or two 16-bit integers
	retval = set_undef_addr();
	tptr = (unsigned short *) address;
	instr = (*tptr++) << 16;
	instr |= *tptr;
	// call THUMB decoding
	// TODO: add Thumb support
	return retval;
}

// handle ARM instructions
instr_next_addr_t next_address_arm(unsigned int address)
{
	instr_next_addr_t retval;
	unsigned int instr;

	// TODO: not implemented yet
	retval = set_undef_addr();


	instr = *((unsigned int *) address);
	retval = ARM_decoder_dispatch(instr);

	// if execution is linear, address = 0xffffffff is returned
	// here we get the true address
	if ((retval.flag == INSTR_ADDR_ARM) && (retval.address == 0xffffffff))
	{
		// get next address for linear execution
		// PABT leaves PC to point at the instruction that caused PABT,
		// in our case BKPT. We have to skip that to reach the address
		// after the BKPT (BKPT is replaced with the restored instruction).
		retval.address = rpi2_reg_context.reg.r15 + 4;
	}

	return retval;
}

// finds out the branch info about the instruction at address
instr_next_addr_t  next_address(unsigned int address)
{
	instr_next_addr_t retval;

	retval = set_undef_addr();

	// if 'J'-bit is set, not supported
	if (!(rpi2_reg_context.reg.cpsr & (1 << 24)))
	{
		// ARM or Thumb? (bit 5 = 'T'-bit)
		if (rpi2_reg_context.reg.cpsr & (1 << 5))
		{
			// check 16-bit alignment of the address
			if (!(address & 1))
			{
				retval = next_address_thumb(address);
			}
		}
		else
		{
			// check 32-bit alignment of the address
			if (!(address & 3))
			{
				retval = next_address_arm(address);
			}
		}
	}
	return retval;
}

