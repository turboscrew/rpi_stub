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

// places for opcodes that we need to actually execute
// extern uint32_t mrs_regb;
extern uint32_t executed_instruction;

// some scratchpads
uint32_t tmp1;
uint32_t tmp2;
uint32_t tmp3;
uint32_t tmp4;
int32_t itmp1;
int32_t itmp2;
int32_t itmp3;
int32_t itmp3;
int64_t lltmp;
int16_t  htmp1;
int16_t  htmp2;
uint64_t dtmp;
uint32_t *iptr;

// returns zero if instruction condition doesn't match the
// stored CPSR condition flags (makes instruction a NOP)
int will_branch(uint32_t instr)
{
	uint32_t flags;
	if ((instr & 0xf0000000) == 0xf << 28) return 0; // NV / special
	if ((instr & 0xf0000000) == 0xe << 28) return 1; // AL
	// true conditional
	flags = (rpi2_reg_context.reg.cpsr & 0xf0000000) >> 28; // flags
	switch ((instr & 0xf0000000) >> 28) // condition
	{
	case 0x0: // EQ (Z=1)
		return ((flags & INSTR_FLAG_Z) != 0);
		break;
	case 0x1: // NE (Z=0)
		return ((flags & INSTR_FLAG_Z) == 0);
		break;
	case 0x2: // CS (C=1)
		return ((flags & INSTR_FLAG_C) != 0);
		break;
	case 0x3: // CC (C=0)
		return ((flags & INSTR_FLAG_C) == 0);
		break;
	case 0x4: // MI (N=1)
		return ((flags & INSTR_FLAG_N) != 0);
		break;
	case 0x5: // PL (N=0)
		return ((flags & INSTR_FLAG_N) == 0);
		break;
	case 0x6: // VS (V=1)
		return ((flags & INSTR_FLAG_V) != 0);
		break;
	case 0x7: // VC (V=0)
		return ((flags & INSTR_FLAG_V) == 0);
		break;
	case 0x8: // HI (Z=0, C=1)
		return (((flags & INSTR_FLAG_Z) || (flags & INSTR_FLAG_C))
				== (flags & INSTR_FLAG_C));
		break;
	case 0x9: // LS (Z=1, C=0)
		return (((flags & INSTR_FLAG_Z) || (flags & INSTR_FLAG_C))
				== (flags & INSTR_FLAG_Z));
		break;
	case 0xa: // GE (N == V)
		return ( // I don't trust that two 'trues' are equal
				(((flags & INSTR_FLAG_N) == 0) && ((flags & INSTR_FLAG_V) == 0))
				|| (((flags & INSTR_FLAG_N) != 0) && ((flags & INSTR_FLAG_V) != 0))
				);
		break;
	case 0xb: // LT (N != V)
		return ( // I don't trust that two 'trues' are equal
				(((flags & INSTR_FLAG_N) != 0) && ((flags & INSTR_FLAG_V) == 0))
				|| (((flags & INSTR_FLAG_N) == 0) && ((flags & INSTR_FLAG_V) != 0))
				);
		break;
	case 0xc: // GT (Z=0 N=V)
		return ( // I don't trust that two 'trues' are equal
				((flags & INSTR_FLAG_Z) == 0) && (
					(((flags & INSTR_FLAG_N) == 0) && ((flags & INSTR_FLAG_V) == 0))
					|| (((flags & INSTR_FLAG_N) != 0) && ((flags & INSTR_FLAG_V) != 0))
				));
		break;
	case 0xd: // LE (Z=1, N!=V)
		return ( // I don't trust that two 'trues' are equal
				((flags & INSTR_FLAG_Z) != 0) && (
					(((flags & INSTR_FLAG_N) != 0) && ((flags & INSTR_FLAG_V) == 0))
					|| (((flags & INSTR_FLAG_N) == 0) && ((flags & INSTR_FLAG_V) != 0))
				));
		break;
	default: // Something is very wrong if we get here
		return 0;
		break;
	}
	return 0;
}

// execute instruction with edited registers
// TODO: This needs to be explained more clearly
// registers used are r0, r1, r2 and r3
// the original registers are given in the parameters
// The parameter value is the number of the register whose (stored)
// content goes to r0 etc. If r0 value is 5, the value of r5 in the
// stored register context is used as the r0 value
// The way the registers are used is upto the edited instruction
// the used registers are returned in the 'tmp'-variables
// r0 -> tmp1, r1->tmp2, ...
// The stored register context is used, but not altered
void exec_instr(uint32_t instr, uint32_t r0, uint32_t r1,
			uint32_t r2, uint32_t r3)
{
	tmp1 = rpi2_reg_context.storage[r0]; // r0
	tmp2 = rpi2_reg_context.storage[r1]; // r1
	tmp3 = rpi2_reg_context.storage[r2]; // r2
	tmp4 = rpi2_reg_context.storage[r3]; // r3
	iptr = (uint32_t *) executed_instruction;
	*iptr = tmp4;
	asm(
		"push {r0, r1, r2, r3, r4}\n\t"
		"ldr r4, =tmp1\n\t"
		"ldr r0, [r4]\n\t"
		"ldr r4, =tmp2\n\t"
		"ldr r1, [r4]\n\t"
		"ldr r4, =tmp3\n\t"
		"ldr r2, [r4]\n\t"
		"ldr r4, =tmp4\n\t"
		"ldr r3, [r4]\n\t"
		"executed_instruction: .word 0 @ execute instr with our registers\n\t"
		"ldr r4, =tmp1\n\t"
		"str r0, [r4] @ return results to tmps\n\t"
		"ldr r4, =tmp2\n\t"
		"str r1, [r4] @ return results to tmps\n\t"
		"ldr r4, =tmp3\n\t"
		"str r2, [r4] @ return results to tmps\n\t"
		"ldr r4, =tmp4\n\t"
		"str r3, [r4] @ return results to tmps\n\t"
		"pop {r0, r1, r2, r3, r4}\n\t"
		);
}

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

