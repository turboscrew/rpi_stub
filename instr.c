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

#define INSTR_COND_MASK 0xf0000000
#define INSTR_COND_AL 0xe0000000
#define INSTR_COND_NV 0xf0000000

// Instruction classification
#define INSTR_CLASS_MASK 0x0e000000
#define INSTR_C0 0x00000000
#define INSTR_C1 0x02000000
#define INSTR_C2 0x04000000
#define INSTR_C3 0x06000000
#define INSTR_C4 0x08000000
#define INSTR_C5 0x0a000000
#define INSTR_C6 0x0c000000
#define INSTR_C7 0x0e000000

// Instruction secondary classification
#define INSTR_SC_MASK 0x01800000
#define INSTR_SC_0 0x00000000
#define INSTR_SC_1 0x00800000
#define INSTR_SC_2 0x01000000
#define INSTR_SC_3 0x01800000

// These are used for classification too
#define INSTR_BIT4_MASK (1 << 4)
#define INSTR_BIT20_MASK (1 << 20)
#define INSTR_IMMEDIATE_MASK 0x00200000

// Extension to secondary classification
#define INSTR_DPI2_MASK 0x00600000
#define INSTR_DPI2_0 0x00000000
#define INSTR_DPI2_1 0x00200000
#define INSTR_DPI2_2 0x00400000
#define INSTR_DPI2_3 0x00600000

 // PSR flags
// N Z C V
#define INSTR_FLAG_N 0x8
#define INSTR_FLAG_Z 0x4
#define INSTR_FLAG_C 0x2
#define INSTR_FLAG_V 0x1

// places for opcodes that we need to actually execute
extern uint32_t sdt_imm;
extern uint32_t mrs_regb;

// some scratchpads
uint32_t tmp1;
uint32_t tmp2;
uint32_t tmp3;
uint32_t tmp4;
uint64_t dtmp;
uint32_t *iptr;

// I realized a bit later that functions 'bit' and 'bits' are
// quite useful, but since I already had the main instruction
// classification masks and values, I didn't change the use of them.
// Also the masks and comparisons are more optimal without shifting.

// return the value of 'pos'th bit (31 - 0)
inline uint32_t bit(uint32_t value, int pos)
{
	return ((value >> pos) & 1);
}

// return the value of bits in the range
inline uint32_t bits(uint32_t value, int hi, int lo)
{
	return ((value >> lo) & ((1 << (hi - lo + 1)) - 1));
}

// just in case it's useful
inline int log_xor(int val1, int val2)
{
	return ((val1 && (!val2)) || ((!val1) && val2));
}


// linear doesn't have branch address
// how can we tell which coding? T1/T2/A1/A2/...?
// maybe it's better to execute instructions with different registers?
// (asm needed)

// watch out for ARM/Thumb mode changing instructions

int will_branch(uint32_t instr)
{
	uint32_t flags;
	if ((instr & 0xf0000000) == 0xf) return 0; // NV / special
	if ((instr & 0xf0000000) == 0xe) return 1; // AL
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

// arith & log - so many different instructions that it's better
// to edit the instruction and execute it to find out effects
// but only if the destination register is PC
unsigned int check_arith(uint32_t instr)
{
	unsigned int new_pc = rpi2_reg_context.reg.r15;

	return new_pc;
}

unsigned int check_mult(uint32_t instr)
{
	unsigned int new_pc = rpi2_reg_context.reg.r15;

	tmp1 = instr & 0x00000f00;
	tmp1 = rpi2_reg_context.storage[tmp1];
	tmp2 = instr & 0x0000000f;
	tmp2 = rpi2_reg_context.storage[tmp2];
	dtmp = tmp1 * tmp2;
	// high dest is PC?
	if (instr & 0x000f0000)
	{
		new_pc = (unsigned int) (dtmp >> 32);
	}
	// low dest is PC?
	else if (instr & 0x0000f000)
	{
		new_pc = (unsigned int) (dtmp & 0xffffffff);
	}
	else
	{
		// no effect on PC
	}
	return new_pc;
}

unsigned int check_msr_reg(uint32_t instr)
{
	unsigned int new_pc = rpi2_reg_context.reg.r15;

	// if user mode, then can't even guess
	tmp1 = (uint32_t) rpi2_reg_context.reg.cpsr;
	if ((tmp1 & 0x1f) == 0) // user mode
	{
		// UNPREDICTABLE - whether banked or not
		// the bits 15 - 0 are UNKNOWN
		// no effect on PC
		new_pc = INSTR_ADDR_UNDEF;
	}
	else
	{
		// privileged mode - both reg and banked reg
		tmp2 = (instr & 0xffff0fff) | (1 << 12); // edit Rd = r1
		iptr = (uint32_t *) mrs_regb;
		*iptr = tmp2;
		asm(
			"push {r0, r1}\n\t"
			"mrs r1, cpsr @ save cpsr\n\t"
			"push {r1}\n\t"
			"ldr r0, =tmp1 @ set cpsr\n\t"
			"msr cpsr, r0 @ note: user mode is already excluded\n\t"
			"mrs_regb: .word 0 @ execute instr with our registers\n\t"
			"ldr r0, =tmp2\n\t"
			"str r1, [r0] @ store result to tmp2\n\t"
			"pop {r1} @ restore cpsr\n\t"
			"msr cpsr, r1\n\t"
			"pop {r0, r1}\n\t"
			);
		new_pc = (unsigned int) tmp2;
	}
	return new_pc;
}

unsigned int check_single_data_xfer_imm(uint32_t instr)
{
	unsigned int new_pc = rpi2_reg_context.reg.r15;

	// if wback && n == t then UNPREDICTABLE
	tmp1 = (instr & 0x00000fff); // immediate
	tmp2 = (instr & 0x01200000); // P and W bit
	if (!(instr & 0x00800000)) // immediate sign negative?
	{
		tmp1 = -tmp1;
	}

	// if Rd = PC
	if ((instr & 0x0000f000) == (15 << 12))
	{
		if (instr & 0x00100000) // load
		{
			tmp3 = (instr & 0x000f0000) >> 16; // Rn
			tmp3 = rpi2_reg_context.storage[tmp3]; // (Rn)
			tmp4 = (instr & 0xffff0fff) | (1 << 12); // edit Rd=r1
			tmp4 = (tmp4 & 0xfff0ffff); // edit Rn=r0
			iptr = (uint32_t *) sdt_imm;
			*iptr = tmp4;
			asm(
				"push {r0, r1}\n\t"
				"ldr r0, =tmp4\n\t"
				"ldr r1, [r0] @ instruction\n\t"
				"ldr r0, =sdt_imm\n\t"
				"str r1, [r0]\n\t"
				"ldr r0, =tmp3 @ r0 <- Rn\n\t"
				"ldr r0, [r0]\n\t"
				"sdt_imm: .word 0 @ execute instr with our registers\n\t"
				"ldr r0, =tmp4\n\t"
				"str r1, [r0] @ store result to tmp4\n\t"
				"pop {r0, r1}\n\t"
			);
			new_pc = (unsigned int) tmp4;
		}
		else
		{
			// no effect on Rd (PC)
		}
	}
	// Rn = PC
	else
	{
		tmp3 = rpi2_reg_context.storage[15]; // PC
		if ((!tmp2) || (tmp2 == 0x01200000)) // pre/post-indexing?
		{
			tmp4 = tmp3 + tmp1;
			new_pc = (unsigned int) tmp4;
		}
		else
		{
			// no effect on PC
		}
	}
	return new_pc;
}

unsigned int check_branching(unsigned int address)
{

	uint32_t instr;
	unsigned int new_pc = rpi2_reg_context.reg.r15;

	instr = *((uint32_t *) address);

	if ((instr & INSTR_COND_MASK) == INSTR_COND_NV)
	{
		// check unconditionals (= specials)
		return new_pc;
	}

	// if condition is not met, the instruction is 'NOP'
	if (will_branch(instr))
	{
		return new_pc;
	}

	switch (instr & INSTR_CLASS_MASK)
	{
		case INSTR_C0:
			switch (instr & INSTR_SC_MASK)
			{
				case INSTR_SC_1:
					if (instr & INSTR_BIT4_MASK) // mult
					{
						new_pc = check_mult(instr);
					}
					else
					{
						// arithmetic & logic
						new_pc = check_arith(instr);
					}
					break;
				case INSTR_SC_2:
					// swaps,mrs,msr: if d == 15 then UNPREDICTABLE
					if (instr & INSTR_BIT4_MASK)
					{
						if (bit(instr, 21))
						{
							// MSR - Rn not altered, changes to state
							// are taken into account at next instruction
							// no effect on PC
						}
						else
						{
							// SWP(B) (if PC is involved, UNPREDICTABLE)
							// Rn and Rt2 don't change
							if (bits(instr,15,12) == 15) // Rt = PC
							{
								tmp1 = bits(instr, 19, 16); // Rn
								iptr = (uint32_t *) rpi2_reg_context.storage[tmp1];
								if (bit(instr,22))
								{
									// SWPB

									new_pc = (unsigned int) *((uint8_t *)iptr);
								}
								else
								{
									//SWP
									new_pc = (unsigned int) (*iptr);
								}
							}
							else
							{
								// no effect on PC
							}
						}
					}
					else
					{
						if (bit(instr, 20) == 0)
						{
							// make a function

							// MRS - reg + banked
							// only changes the Rd
							if (bits(instr, 15, 12) == 15) // Rd = PC
							{
								new_pc = check_msr_reg(instr);
							}
							else // Rd is not PC
							{
								// no effect on PC
							}
						}
						else
						{
							// arithmetic & logic
							new_pc = check_arith(instr);
						}
					}
					break;
				default:
					// arithmetic & logic
					new_pc = check_arith(instr);
					break;
			}
			break;
		case INSTR_C1:
			switch (instr & INSTR_SC_MASK)
			{
				case INSTR_SC_2:
					if (bit(instr, 20))
					{
						if (bits(instr, 22, 21) == 2)
						{
							// movt
							if (bits(instr, 15, 12) == 15) // Rd = PC
							{
								// put together the 16-bit immediate
								tmp1 = instr & 0x0fff;
								tmp1 |= (bits(instr, 19, 16) << 12);
								tmp1 <<= 16; // move to upper half
								// stuff lower half from PC
								tmp1 |= (uint32_t)(rpi2_reg_context.reg.r15 & 0x0000ffff);
								new_pc = (unsigned int) tmp1;
							}
							else
							{
								// arithmetic & logic
								new_pc = check_arith(instr);
							}
						}
						else
						{
							// msr imm - doesn't affect core registers
						}
					}
					else
					{
						// arithmetic & logic
						new_pc = check_arith(instr);
					}
					break;
				default:
					// arithmetic & logic
					new_pc = check_arith(instr);
					break;
			}
			break;
		case INSTR_C2:
			// Single Data Transfer - imm
			// if Rd = PC or Rn = PC
			if ((bits(instr, 19, 16) == 15) || (bits(instr, 15, 12) == 15))
			{
				new_pc = check_single_data_xfer_imm(instr);
			}
			else
			{
				// no effect on PC
			}
			break;
		case INSTR_C3:
			if (instr & INSTR_BIT4_MASK)
			{
				// Media instructions / undef
			}
			else
			{
				// Single Data Transfer - register
				// if m == 15 then UNPREDICTABLE;
				// if wback && (n == 15 || n == t) then UNPREDICTABLE;

			}
			break;
		case INSTR_C4:
			// Block Data Transfer
			break;
		case INSTR_C5:
			// branch
			if ((instr & INSTR_COND_MASK) == INSTR_COND_AL)
			{
			}
			else
			{
			}
			break;
		case INSTR_C6:
			// LDC/STC
			break;
		case INSTR_C7:
			if (instr & 0x01000000)
			{
				// Software interrupt
				new_pc = INSTR_ADDR_NONE;
			}
			else
			{
				if (instr & INSTR_BIT4_MASK)
				{
					// MRC/MCR
				}
				else
				{
					// Co-processor data operations
				}
			}
			break;
		default:
			// logically impossible
			new_pc = INSTR_ADDR_UNDEF;
			break;
	}
	return new_pc;
}


