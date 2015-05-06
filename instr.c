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
extern uint32_t mrs_regb;
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
		// this can't be done with exec_instr(), because this requires
		// also mode changes
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
			tmp1 = bits(instr, 19, 16); // Rn
			tmp2 = bits(instr, 15, 12); // Rd
			tmp3 = 0; // not used
			tmp4 = (instr & 0xffff0fff) | (1 << 12); // edit Rd=r1
			tmp4 = (tmp4 & 0xfff0ffff); // edit Rn=r0
			// execute the edited instruction
			exec_instr(tmp4, tmp1, tmp2, tmp3, tmp3);
			new_pc = tmp2;
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

// next address for media instructions
// TODO: maybe this should be broken into smaller functions
// first level cases with instruction groups are good candidates
unsigned int check_media_instr(uint32_t instr)
{
	uint32_t new_pc = rpi2_reg_context.reg.r15;

	// These need to be decoded far enough to tell whether instruction is
	// one of these or not. If not, it causes UND-exception.
	switch (bits(instr, 24, 23))
	{
	case 0: // parallel add&subtr (Rd: bits 15-12, Rn:19-16, Rm:3-0)
		// unify all these with execution (if Rd = PC)
		// bit 22=1: unsigned, bits 21,20: 00=undef, 01=basic, 10=Q and 11=H
		// SADD16/UADD16/QADD16/UQADD16/SHADD16/UHADD16
		// SASX/UASX/QASX/UQASX/SHASX/UHASX
		// SSAX/USAX/QSAX/UQSAX/SHSAX/UHSAX
		// SSUB16/USUB16/QSUB16/UQSUB16/SHSUB16/UHSUB16
		// SADD8/UADD8/QADD8/UQADD8/SHADD8/UHADD8
		// SSUB8/USUB8/QSUB8/UQSUB8/SHSUB8/UHSUB8
		if(bits(instr, 21, 20) == 0)
		{
			new_pc = INSTR_ADDR_UNDEF;
			break;
		}

		if (bits(instr, 11, 8) != 1)
		{
			new_pc = INSTR_ADDR_UNPRED;
		}

		if ((bits(instr, 7, 5) == 5) || (bits(instr, 7, 5) == 6))
		{
			new_pc = INSTR_ADDR_UNDEF;
			break;
		}
		if (bits(instr, 15, 12) == 15) // Rd is PC
		{
			// privileged mode - both reg and banked reg
			tmp1 = bits(instr, 19, 16); // Rn
			tmp2 = bits(instr, 15, 12); // Rd
			tmp3 = bits(instr, 3, 0); // Rm
			tmp4 = (instr & 0xfff00ff0) | (1 << 12); // edit Rd = r1, Rn = r0
			tmp4 = instr | 2; // edit Rm = r2
			// execute the edited instruction
			exec_instr(instr, tmp1, tmp2, tmp3, tmp3);
			new_pc = tmp2;
		}
		break;
	case 1: // packing, saturation, reversal
		if (bits(instr, 7, 5) == 3)
		{
			// Instructions handled here:
			// SXTAB16, SXTAH, UXTAB16, UXTAB, UXTAH
			// SXTB16, SXTB, SXTH, UXTB16, UXTB, UXTH

			if (bits(instr, 9, 8) != 0)
			{
				new_pc = INSTR_ADDR_UNPRED;
				break;
			}

			switch (bits(instr, 22, 20))
			{
			case 1:
			case 5:
				new_pc = INSTR_ADDR_UNDEF;
				break;
			case 2:
				if (bits(instr, 19, 16) != 15)
				{
					new_pc = INSTR_ADDR_UNDEF;
					break;
				}
				// fallthrough
			case 0:
			case 3:
			case 4:
			case 6:
			case 7:
				if (bits(instr, 15, 12) != 15) // Rd is not PC
				{
					break;
				}
				// here Rn can't be PC. Instead it's a different instruction
				if  (bits(instr, 19, 16) == 15)
				{
					tmp4 = (instr & 0xffff0ff0) | (1 << 12); // edit Rd = r1, no Rn
				}
				else
				{
					tmp4 = (instr & 0xfff00ff0) | (1 << 12); // edit Rd = r1, Rn = r0
				}
				tmp4 = instr | 2; // edit Rm = r2

				tmp1 = bits(instr, 19, 16); // Rn
				tmp2 = bits(instr, 15, 12); // Rd
				tmp3 = bits(instr, 3, 0); // Rm
				// execute edited instruction
				exec_instr(tmp4, tmp1, tmp2, tmp3, tmp3);
				new_pc = tmp2;
				break;
			default:
				// logically impossible
				break;
			}
		}
		else if (bits(instr, 6, 5) == 1)
		{
			switch (bits(instr, 22, 20))
			{
			case 0: // SEL
				if (bit(instr, 7) == 0)
				{
					new_pc = INSTR_ADDR_UNDEF;
					break;
				}

				if ((bits(instr, 19, 16) != 0xf) || bits(instr, 11, 8 != 0xf))
				{
					new_pc = INSTR_ADDR_UNPRED;
					break;
				}

				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					tmp1 = rpi2_reg_context.storage[bits(instr, 19, 16)]; // (Rn)
					tmp2 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rm)
					tmp3 = rpi2_reg_context.reg.cpsr;
					tmp4 = 0;
					// select Rn or Rm by the GE-bits
					tmp4 |= ((bit(tmp3,19)?tmp1:tmp2) & 0xff000000);
					tmp4 |= ((bit(tmp3,18)?tmp1:tmp2) & 0x00ff0000);
					tmp4 |= ((bit(tmp3,17)?tmp1:tmp2) & 0x0000ff00);
					tmp4 |= ((bit(tmp3,16)?tmp1:tmp2) & 0x000000ff);
					new_pc = (unsigned int) tmp4;
				}
				break;
			case 2: // SSAT16
				if (bit(instr, 7) == 1)
				{
					new_pc = INSTR_ADDR_UNDEF;
					break;
				}

				if (bits(instr, 11, 8) != 0xf)
				{
					new_pc = INSTR_ADDR_UNPRED;
					break;

				}

				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					htmp1 = (int16_t)(1 << (bits(instr, 19, 16) - 1)); // 2^(n-1)
					tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)

					// upper half
					htmp2 = (int16_t)((tmp1 & 0xffff0000) >> 16);
					if (htmp2 > (htmp1 - 1)) htmp2 = htmp1 - 1;
					else if (htmp2 < -htmp1) htmp2 = -htmp1;
					// 16-bit signed as a bit field into 32-bit unsigned
					tmp2 = (uint32_t)(uint16_t) htmp2;
					tmp2 <<= 16; // back to the place
					new_pc = tmp2;

					// lower half
					htmp2 = (int16_t)(tmp1 & 0x0000ffff);
					if (htmp2 > (htmp1 - 1)) htmp2 = htmp1 - 1;
					else if (htmp2 < -htmp1) htmp2 = -htmp1;
					// 16-bit signed as a bit field into 32-bit unsigned
					tmp2 = (uint32_t)(uint16_t) htmp2;
					tmp2 &= 0xffff;
					new_pc |= tmp2;
				}
				break;
			case 3: // REV/REV16
				if ((bits(instr, 19, 16) != 0xf) || bits(instr, 11, 8 != 0xf))
				{
					new_pc = INSTR_ADDR_UNPRED;
					break;
				}

				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rm)
					if (bit(instr, 7) == 0)
					{
						// REV
						tmp2 = (tmp1 & 0xff000000)  >> 24; // result lowest
						tmp2 |= (tmp1 & 0x00ff0000) >> 8;
						tmp2 |= (tmp1 & 0x0000ff00) << 8;
						tmp2 |= (tmp1 & 0x000000ff) << 24; // result highest
					}
					else
					{
						// REV16
						tmp2 = (tmp1 & 0xff000000)  >> 8;
						tmp2 |= (tmp1 & 0x00ff0000) << 8; // result highest
						tmp2 |= (tmp1 & 0x0000ff00) >> 8; // result lowest
						tmp2 |= (tmp1 & 0x000000ff) << 8;
					}
					new_pc = tmp2;
				}
				break;
			case 6: // USAT16
				if (bit(instr, 7) == 1)
				{
					new_pc = INSTR_ADDR_UNDEF;
					break;
				}

				if (bits(instr, 11, 8) != 0xf)
				{
					new_pc = INSTR_ADDR_UNPRED;
					break;

				}

				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					htmp1 = (int16_t)(1 << (bits(instr, 19, 16) - 1)); // 2^(n-1)
					tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)

					// upper half
					htmp2 = (int16_t)((tmp1 & 0xffff0000) >> 16);
					if (htmp2 > (htmp1 - 1)) htmp2 = htmp1 - 1;
					else if (htmp2 < 0) htmp2 = 0;
					// 16-bit signed as a bit field into 32-bit unsigned
					tmp2 = (uint32_t)(uint16_t) htmp2;
					tmp2 <<= 16; // back to the place
					new_pc = tmp2;

					// lower half
					htmp2 = (int16_t)(tmp1 & 0x0000ffff);
					if (htmp2 > (htmp1 - 1))  htmp2 = htmp1 - 1;
					else if (htmp2 < 0) htmp2 = 0;
					tmp2 = (uint32_t)(uint16_t)htmp2;
					new_pc |= tmp2 & 0xffff; // back to the place
				}
				break;
			case 7: // RBIT/REVSH
				if ((bits(instr, 19, 16) != 0xf) || bits(instr, 11, 8 != 0xf))
				{
					new_pc = INSTR_ADDR_UNPRED;
					break;
				}

				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					if (bit(instr, 7) == 0)
					{
						// RBIT
						tmp1 = rpi2_reg_context.reg.r15;
						// swap odd and even bits
						tmp1 = ((tmp1 & 0xaaaaaaaa) >> 1) | ((tmp1 & 0x55555555) << 1);
						// swap bit pairs
						tmp1 = ((tmp1 & 0xcccccccc) >> 2) | ((tmp1 & 0x33333333) << 2);
						// swap nibbles
						tmp1 = ((tmp1 & 0xf0f0f0f0) >> 4) | ((tmp1 & 0x0f0f0f0f) << 4);
						// swap bytes
						tmp1 = ((tmp1 & 0xff00ff00) >> 8) | ((tmp1 & 0x00ff00ff) << 8);
						// swap half words
						tmp1 = ((tmp1 & 0xffff0000) >> 16) | ((tmp1 & 0x0000ffff) << 16);
						new_pc = tmp1;
					}
					else
					{
						// REVSH
						// Rm[15:8] -> Rd[7:0], Rm[7:0] -> Rd[31:8](sign extended)
						tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rm)
						// sign-extend lowest byte
						itmp1 = (int32_t)(int8_t)(tmp1 & 0xff);
						// and make it to upper 24 bits
						tmp2 = (uint32_t) (itmp1 << 8) & 0xffffff00;
						tmp2 |= (tmp1 >> 8)& 0x000000ff; // Rm[15:8] -> Rd[7:0]
						new_pc = tmp2;
					}
				}
				break;
			default:
				// undef
				new_pc = INSTR_ADDR_UNDEF;
				break;
			}
		}
		else if (bit(instr, 5) == 0)
		{
			if (bits(instr, 22, 20) == 0)
			{
				// PKH
				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					// operand2
					tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rm)
					tmp2 = bits(instr, 11, 7); // imm5
					tmp3 = rpi2_reg_context.storage[bits(instr, 19, 16)]; // (Rn)
					if (bit(instr, 6))
					{
						// PKHTB: shift=ASR (sign extended shift)
						// >> is ASR only for signed numbers
						itmp1 = (int32_t)tmp1;
						itmp1 >>= tmp2;
						tmp1 = (uint32_t) itmp1;
						// Rd[31:16] = Rn[31:16]
						new_pc = tmp3 & 0xffff0000;
						// Rd[15:0] = shifted_op[15:0]
						new_pc |= tmp1 & 0xffff;
					}
					else
					{
						// PKHBT: shift=LSL
						tmp1 <<= tmp2;
						// Rd[31:16] = shifted_op[31:16]
						new_pc = tmp1 & 0xffff0000;
						// Rd[15:0] = Rn[15:0]
						new_pc |= tmp3 & 0xffff;
					}
				}
			}
			else if (bits(instr, 22, 21) == 1)
			{
				// SSAT
				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					// operand
					tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					tmp2 = bits(instr, 11, 7);
					if (bit(instr, 6))
					{
						if (tmp2 == 0) tmp2 = 32; // so it says in the docs
						// >> is ASR only for signed numbers
						itmp1 = (int32_t)tmp1;
						itmp1 >>= tmp2;
					}
					else
					{
						// << (LSL)
						itmp1 <<= tmp2;
					}

					// saturation limit
					itmp2 = (int32_t)(1 << (bits(instr, 20, 16) - 1)); // 2^(n-1)

					if (itmp1 > (itmp2 - 1)) itmp1 = itmp2 - 1;
					else if (itmp1 < -itmp2) itmp1 = -itmp2;

					new_pc = (uint32_t) itmp1;
				}
			}
			else if (bits(instr, 22, 21) == 3)
			{
				// USAT

				// operand
				tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
				tmp2 = bits(instr, 11, 7);
				if (bit(instr, 6))
				{
					if (tmp2 == 0) tmp2 = 32; // so it says in the docs
					// >> is ASR only for signed numbers
					itmp1 = (int32_t)tmp1;
					itmp1 >>= tmp2;
				}
				else
				{
					// << (LSL)
					itmp1 <<= tmp2;
				}

				// saturation limit
				itmp2 = (int32_t)(1 << (bits(instr, 20, 16) - 1)); // 2^(n-1)

				if (itmp1 > (itmp2 - 1)) itmp1 = itmp2 - 1;
				else if (itmp1 < 0) itmp1 = 0;

				new_pc = (uint32_t)itmp1;
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
		}
		else
		{
			new_pc = INSTR_ADDR_UNDEF;
		}
		break;
	case 2: // signed multiply
		switch (bits(instr, 22, 20))
		{
		case 0: // MAC 32-bit
			if (bit(instr, 7) == 0)
			{
				// SMLAD/SMUAD/SMLSD/SMUSD
				if (bits(instr, 11, 8) == 15) // Rd = PC
				{
					// operand2
					tmp1 = rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					tmp2 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					if (bit(instr, 5)) // M -> swap
					{
						htmp1 = (int16_t)(tmp1 & 0xffff);
						htmp2 = (int16_t)((tmp2 & 0xffff0000) >> 16);
						itmp1 = htmp1 * htmp2;
						htmp1 = (int16_t)((tmp1 & 0xffff0000) >> 16);
						htmp2 = (int16_t)(tmp2 & 0xffff);
						itmp2 = htmp1 * htmp2;
					}
					else
					{
						htmp1 = (int16_t)((tmp1 & 0xffff0000) >> 16);
						htmp2 = (int16_t)((tmp2 & 0xffff0000) >> 16);
						itmp1 = htmp1 * htmp2;
						htmp1 = (int16_t)(tmp1 & 0xffff);
						htmp2 = (int16_t)(tmp2 & 0xffff);
						itmp2 = htmp1 * htmp2;
					}

					if (bit(instr, 6))
					{
						// subtract
						itmp2 -= itmp1;
					}
					else
					{
						// add
						itmp2 += itmp1;
					}

					if (bits(instr, 15, 12) != 15)
					{
						// accumulate (Ra)
						itmp1 = (int32_t)rpi2_reg_context.storage[bits(instr, 15, 12)]; // (Ra)
						itmp2 += itmp1;
					}
					new_pc = (uint32_t) itmp2;
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 5: // MAC 64-bit upper
			if (bits(instr, 7, 6) == 0)
			{
				// SMMLA/SMMUL
				if (bits(instr, 19, 16) == 15) // Rd = PC
				{
					itmp1 = (int32_t)rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					itmp2 = (int32_t)rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					lltmp = (itmp1 * itmp2);
					if (bits(instr, 15, 12) == 15) // SMMLA
					{
						itmp1 = (int32_t)rpi2_reg_context.storage[bits(instr, 15, 12)]; // (Ra)
						lltmp += (((int64_t)itmp1) << 32);
					}
					if (bit(instr, 5)) lltmp += 0x80000000; // round
					new_pc = (uint32_t)((lltmp >> 32) & 0xffffffff);
				}
			}
			else if (bits(instr, 7, 6) == 3)
			{
				// SMMLS
				if (bits(instr, 19, 16) == 15) // Rd = PC
				{
					itmp1 = (int32_t)rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					itmp2 = (int32_t)rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					lltmp = (itmp1 * itmp2);
					itmp1 = (int32_t)rpi2_reg_context.storage[bits(instr, 15, 12)]; // (Ra)
					lltmp = (((int64_t)itmp1) << 32) - lltmp;
					if (bit(instr, 5)) lltmp += 0x80000000; // round
					new_pc = (uint32_t)((lltmp >> 32) & 0xffffffff);
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 1: // SDIV
			if (bits(instr, 7, 5) == 0)
			{
				if (bits(instr, 19, 16) == 15) // Rd = PC
				{
					// Check for division by zero
					tmp1 = rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					if (tmp1 == 0)
					{
						// division by zero
						new_pc = INSTR_ADDR_NONE;
					}
					else
					{
						tmp1 = bits(instr, 19, 16); // Rd
						tmp2 = bits(instr, 11, 8); // Rm
						tmp3 = bits(instr, 3, 0); // Rn
						tmp4 = (instr & 0xfff0f0f0) | (1 << 16); // edit Rd = r1, Rn = r0
						tmp4 = instr | (2 << 8); // edit Rm = r2
						// execute the edited instruction
						exec_instr(tmp4, tmp3, tmp1, tmp2, tmp2);
						new_pc = tmp1;
					}

				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 3: // UDIV
			if (bits(instr, 7, 5) == 0)
			{
				if (bits(instr, 19, 16) == 15) // Rd = PC
				{
					// Check for division by zero
					tmp1 = rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					if (tmp1 == 0)
					{
						// division by zero
						new_pc = INSTR_ADDR_NONE;
					}
					else
					{
						tmp1 = bits(instr, 19, 16); // Rd
						tmp2 = bits(instr, 11, 8); // Rm
						tmp3 = bits(instr, 3, 0); // Rn
						tmp4 = (instr & 0xfff0f0f0) | (1 << 16); // edit Rd = r1, Rn = r0
						tmp4 = instr | (2 << 8); // edit Rm = r2
						// execute the edited instruction
						exec_instr(tmp4, tmp3, tmp1, tmp2, tmp2);
						new_pc = tmp1;
					}
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 4: // MAC 64-bit
			if (bit(instr, 7) == 0)
			{
				if ((bits(instr, 19, 16) == 15) && (bits(instr, 15, 12) == 15))
				{
					// can't guess what happens
					new_pc = INSTR_ADDR_UNPRED;
					break;
				}
				// if either Rdh or Rdl is PC
				if ((bits(instr, 19, 16) == 15) || (bits(instr, 15, 12) == 15))
				{

					// operand2
					tmp1 = rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					tmp2 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					if (bit(instr, 5)) // swap
					{
						htmp1 = (int16_t)(tmp1 & 0xffff);
						htmp2 = (int16_t)((tmp2 >> 16) & 0xffff);
						itmp1 = htmp1 * htmp2;
						htmp1 = (int16_t)((tmp1 >> 16) & 0xffff);
						htmp2 = (int16_t)(tmp2 & 0xffff);
						itmp2 = htmp1 * htmp2;
					}
					else
					{
						htmp1 = (int16_t)((tmp1 >> 16) & 0xffff);
						htmp2 = (int16_t)((tmp2 >> 16) & 0xffff);
						itmp1 = htmp1 * htmp2;
						htmp1 = (int16_t)(tmp1 & 0xffff);
						htmp2 = (int16_t)(tmp2 & 0xffff);
						itmp2 = htmp1 * htmp2;

					}
					// accumulate
					tmp1 = rpi2_reg_context.storage[bits(instr, 19, 16)]; // (Rdh)
					tmp2 = rpi2_reg_context.storage[bits(instr, 15, 12)]; // (Rdl)
					lltmp = (int64_t)(int32_t)tmp1;
					lltmp = (lltmp << 32) + (int64_t)(int32_t)tmp2;
					if (bit(instr, 6))
					{
						// SMLSLD
						lltmp += ((int64_t)itmp2) - ((int64_t)itmp1);
					}
					else
					{
						// SMLALD
						lltmp += ((int64_t)itmp2) + ((int64_t)itmp1);
					}
					if (bits(instr, 19, 16) == 15)
					{
						// if PC was Rdh
						tmp2 = (uint32_t)(((uint64_t)lltmp) >> 32);
					}
					else
					{
						// PC was Rdl
						tmp1 = (uint32_t)(((uint64_t)lltmp) & ((uint64_t)0xffffffff));
					}
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		default:
			new_pc = INSTR_ADDR_UNDEF;
			break;
		}
		break;
	case 3:
		switch (bits(instr, 22, 21))
		{
		case 0: // USAD(A)8
			if ((bit(instr, 20) == 0) && (bits(instr, 7, 5) == 0))
			{
				if (bits(instr, 19, 16) == 15) // PC
				{
					tmp1 = rpi2_reg_context.storage[bits(instr, 11, 8)]; // (Rm)
					tmp2 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					// absolute differences
					tmp3 = 0;
					for (tmp4 = 0; tmp4 < 4; tmp4++)
					{
						htmp1 = (int16_t)(uint16_t)((tmp1 >> (8*tmp4)) & 0xff);
						htmp2 = (int16_t)(uint16_t)((tmp2 >> (8*tmp4)) & 0xff);
						if (htmp2 < htmp1) htmp1 -= htmp2;
						else htmp1 = htmp2 - htmp1;
						tmp3 += (uint32_t)(uint16_t) htmp1;
					}

					if (bits(instr, 15, 12) != 15)
					{
						// USADA8 (=USAD8 with Ra)
						tmp1 = rpi2_reg_context.storage[bits(instr, 15, 12)]; // (Ra)
						tmp3 += tmp1;
					}
					new_pc = tmp3;
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 1:
			// SBFX
			if (bits(instr, 6, 5) == 2)
			{
				if (bits(instr, 15,12) == 15) // Rd = PC
				{
					tmp1 = bits(instr, 20, 16); // width
					tmp2 = bits(instr, 11, 7); // lsb
					tmp1 += tmp2; // msb
					tmp3 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					tmp4 = bits(tmp3, tmp1, tmp2);
					// sign extend
					if (bit(tmp3, tmp1) == 1)
					{
						// for all bits above msb
						for (tmp2 = 31; tmp2 > tmp1; tmp2--)
						{
							tmp4 |= (1 << tmp2);
						}
					}
					new_pc = tmp4;
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 2:
			// BFC/BFI
			if (bits(instr, 6, 5) == 0)
			{
				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					tmp1 = bits(instr, 20, 16); // msb
					tmp2 = bits(instr, 11, 7); // lsb
					if (tmp2 > tmp1)
					{
						new_pc = INSTR_ADDR_UNPRED;
						break;
					}
					// make bitmask
					tmp3 = (~0) << (tmp1 - tmp2);
					tmp3 = (~tmp3) << tmp2; // bitmask
					// clear bits in Rd
					tmp4 = rpi2_reg_context.storage[bits(instr, 15, 12)]; // (Rn)
					tmp4 &= (~tmp3); // result of BFC

					if (bits(instr, 3, 0) != 15) // BFI
					{
						tmp1 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
						tmp1 &= tmp3;
						tmp4 |= tmp1;
					}
					new_pc = tmp4;
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 3:
			// UBFX/UDF
			if (bits(instr, 6, 5) == 2)
			{
				// UBFX
				if (bits(instr, 15, 12) == 15) // Rd = PC
				{
					tmp1 = bits(instr, 20, 16); // width
					tmp2 = bits(instr, 11, 7); // lsb
					tmp1 += tmp2; // msb
					if (tmp2 > tmp1)
					{
						new_pc = INSTR_ADDR_UNPRED;
						break;
					}
					tmp3 = rpi2_reg_context.storage[bits(instr, 3, 0)]; // (Rn)
					new_pc = bits(tmp3, tmp1, tmp2);
				}
			}
			else if (bit(instr, 20) && (bits(instr, 7, 5) == 7)
					&& (bits(instr, 31, 28) == 14))
			{
				// UDF - Permanently Undefined
				new_pc = INSTR_ADDR_UNDEF;
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		}
		break;
	default:
		// logically impossible
		break;
	}
	return (unsigned int) new_pc;
}

// get next PC value after executing the instruction in this address
unsigned int check_branching(unsigned int address)
{

	uint32_t instr;
	unsigned int new_pc = rpi2_reg_context.reg.r15;

	instr = *((uint32_t *) address);

	if ((instr & INSTR_COND_MASK) == INSTR_COND_NV)
	{
		switch (bits(instr, 27, 25))
		{
		case 0:
			// CPS/SETEND
			if (bits(instr, 24, 20) == 16)
			{
				if (bit(instr, 16) == 0)
				{
					// CPS
					if ((bits(instr, 15, 9) == 0) && (bit(instr, 5) == 0))
					{
						// check only if user mode (for now)
						// TODO: add current mode/mode update checks
						if (bits(rpi2_reg_context.reg.cpsr, 4, 0) == 0x10)
						{
							new_pc = INSTR_ADDR_UNPRED;
						}
						// Otherwise no effect on PC
					}
					else
					{
						new_pc = INSTR_ADDR_UNDEF;
					}
				}
				else
				{
					// SETEND (bit 9 = E)
					if (instr & 0x000efdff != 0)
					{
						new_pc = INSTR_ADDR_UNDEF;
					}
					// Doesn't affect PC just yet
					// the NEXT instruction is THUMB and may be 16-bits
				}
			}
			else
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			break;
		case 2:
			if (bit(instr, 21) == 0)
			{
				// PLI/PLD
				// "These instructions are memory system hints,
				// and the effect of each instruction is IMPLEMENTATION DEFINED"
				// these instructions don't affect PC
				// just check the rest of the "fixed" bits
				if (bit(instr, 24) == 0)
				{
					// PLI
					if (!(bits(instr, 22, 20) == 5) || !(bits(instr, 15, 12) == 15))
					{
						new_pc = INSTR_ADDR_UNDEF;
					}
					// PLI doesn't affect PC
				}
				else
				{
					// PLD
					if (bits(instr, 19, 16) == 15)
					{
						// label
						if (!(bits(instr, 22, 20) == 5) || !(bits(instr, 15, 12) == 15))
						{
							new_pc = INSTR_ADDR_UNDEF;
						}
					}
					else
					{
						// register
						if (!(bits(instr, 21, 20) == 1) || !(bits(instr, 15, 12) == 15))
						{
							new_pc = INSTR_ADDR_UNDEF;
						}
					}
				}
			}
			else
			{
				// CLREX/DSB/DMB/ISB
				// these instructions don't affect PC
				// just check the rest of the "fixed" bits
				if (bits(instr, 24, 8) != 0x17ff0)
				{
					new_pc = INSTR_ADDR_UNDEF;
				}
				switch (bits(instr, 7, 4))
				{
				case 1: // CLREX
					if (bits(instr, 3, 0) != 0xf)
					{
						new_pc = INSTR_ADDR_UNDEF;
					}
					break;
				case 4: // DSB
				case 5: // DMB
				case 6: // ISB
					// fully decoded
					break;
				default:
					new_pc = INSTR_ADDR_UNDEF;
					break;
				}
			}
			break;
		case 3:
			// PLI/PLD
			// "These instructions are memory system hints,
			// and the effect of each instruction is IMPLEMENTATION DEFINED"
			// these instructions don't affect PC
			// just check the rest of the "fixed" bits
			if (!(bits(instr, 21, 20) == 1) || !(bits(instr, 15, 12) == 15)
					|| !(bit(instr, 4) == 0))
			{
				new_pc = INSTR_ADDR_UNDEF;
			}
			// nothing more to decode
			break;
		case 4:
			// RFE/SRS
			if (bits(rpi2_reg_context.reg.cpsr, 4, 0) == 0x10)
			{
				// user mode
				new_pc = INSTR_ADDR_UNPRED;
			}
			else if (bits(rpi2_reg_context.reg.cpsr, 4, 0) == 0x1a)
			{
				// hyp mode
				new_pc = INSTR_ADDR_UNDEF;
			}
			else
			{
				if (bit(instr, 20) == 0)
				{
					// SRS
					// UNDEFINED in hyp mode
					// UNPREDICTABLE in User or System mode
					// TODO: Add the rest of restrictions
					if (bits(rpi2_reg_context.reg.cpsr, 4, 0) == 0x1f)
					{
						// sys mode
						new_pc = INSTR_ADDR_UNPRED;
					}
					else
					{
						// just check the rest of the "fixed" bits
						if (!(bit(instr, 22) == 0) || !(bits(instr, 19, 5) == 0x6828))
						{
							new_pc = INSTR_ADDR_UNDEF;
						}
						// doesn't affect PC
					}
				}
				else
				{
					// RFE - Return from exception
					// UNDEFINED in hyp mode
					// UNPREDICTABLE in User mode
					// TODO: Add the rest of restrictions
					if ((bit(instr, 22) == 0) && (bits(instr, 15, 0) == 0x00a0))
					{
						// return from exception
						new_pc = INSTR_ADDR_NONE;
					}
					else
					{
						new_pc = INSTR_ADDR_UNDEF;
					}
				}
			}
			break;
		case 5:
			// BLX
			tmp1 = bits(instr, 23, 0); // label
			if (bit(instr, 23))
			{
				// negative offset - sign extend to 30 bits
				tmp1 |= 0x3f000000;
			}
			tmp1 <<= 2; // to 32-bits
			tmp1 |= bit(instr, 24) << 1; // Add 'H' bit for Thumb address
			new_pc = tmp1;
			break;
		default:
			new_pc = INSTR_ADDR_UNDEF;
			break;
		}
		return new_pc;
	}

	// if condition is not met, the instruction is 'NOP'
	if (will_branch(instr))
	{
		return new_pc;
	}

	switch (instr & INSTR_CLASS_MASK)
	{
		// TODO: lots of LD-instructions missing (LDRD, LDREX, ...)
		case INSTR_C0:
			// Arith&log + LD/ST (reg)
			if (bit(instr, 4) == 0)
			{
				// bits 24 and 23
				switch (instr & INSTR_SC_MASK)
				{
					case INSTR_SC_2:
						if (bit(instr, 20) == 0) // specials
						{
							// bit 7
							if (bit(instr, 7) == 0)
							{
								switch (instr, 6, 5)
								{
								case 0:
									// bit 21 (0=MRS, 1=MSR), bit 9 (1=banked)
									// MSR - Rn not altered, changes to state
									// are taken into account at next instruction
									// no effect on PC
									// mrs,msr: if d == 15 then UNPREDICTABLE
									// bit 4 = 1
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
									break;
								case 1:
									// BXJ
									break;
								case 2:
									// empty
									break;
								case 3:
									// ERET
									break;
								}
							}
							else // bit(instr, 7) == 1
							{
								switch (instr, 22, 21)
								{
								case 0:
									// SMLA
									break;
								case 1:
									if (bit(instr, 5) == 0)
									{
										// SMLAW
									}
									else
									{
										// SMULW
									}
									break;
								case 2:
									// SMLAL
									break;
								case 3:
									// SMUL
									break;
								}
							}
						}
						else // tests
						{
							switch (bits(instr, 22, 21))
							{
							case 0: // TST
								break;
							case 1: // TEQ
								break;
							case 2: // CMP
								break;
							case 3: // CMN
								break;
							default:
								// logically impossible
								break;
							}
						}
						break;
					default:
						// arithmetic & logic
						new_pc = check_arith(instr);
						break;
				}
			}
			else // bit(instr, 4) == 1
			{
				if (bit(instr, 7) == 0)
				{
					// bits 24 and 23
					switch (instr & INSTR_SC_MASK)
					{
						case INSTR_SC_2:
							if (bit(instr, 20) == 0)
							{
								// spec first bits 6,5 then 22,21
								switch (bits(instr, 6, 5))
								{
								case 0:
									if (bit(instr, 22) == 0)
									{
										//BX
									}
									else
									{
										//CLZ
									}
									break;
								case 1:
									// BLX
									break;
								case 2:
									switch (bits(instr, 22, 21))
									{
									case 0:
										// QADD
										break;
									case 1:
										// QSUB
										break;
									case 2:
										// QDADD
										break;
									case 3:
										// QDSUB
										break;
									default:
										// logically impossible
										break;
									}
									break;
								case 3:
									switch (bits(instr, 22, 21))
									{
									case 0:
										break;
									case 1:
										// BKPT
										break;
									case 2:
										// HVC
										break;
									case 3:
										// SMC
										break;
									default:
										// logically impossible
										break;
									}
									break;
								}
							}
							else
							{
								// TST ...
								switch (bits(instr, 22, 21))
								{
								case 0: // TST
									break;
								case 1: // TEQ
									break;
								case 2: // CMP
									break;
								case 3: // CMN
									break;
								default:
									// logically impossible
									break;
								}
							}
							break;
						default:
							// arithmetic & logic
							new_pc = check_arith(instr);
							break;
					}

#if 0
					if (instr & INSTR_BIT4_MASK) // mult
					{
						new_pc = check_mult(instr);
					}
					// bit 4 = 1
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
#endif
				}
				else // bit(instr, 7) == 1
				{
					switch(bits(instr, 6, 5))
					{
					case 0:
						switch (bits(instr, 24, 23))
						{
						case 0:
							switch (bits(instr, 22, 21))
							{
							case 0:
								// MULS
								break;
							case 1:
								// MLAS
								break;
							case 2:
								// UMAAL
								break;
							case 3:
								// MLS
								break;
							}
							break;
						case 1:
							switch (bits(instr, 22, 21))
							{
							case 0:
								// UMULL
								new_pc = check_mult(instr);
								break;
							case 1:
								// UMLAL
								new_pc = check_mult(instr);
								break;
							case 2:
								// SMULL
								new_pc = check_mult(instr);
								break;
							case 3:
								// SMLAL
								new_pc = check_mult(instr);
								break;
							}
							break;
						case 2:
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
							break;
						case 3:
							switch (bits(instr, 22, 20))
							{
							case 0:
								// STREX
								break;
							case 1:
								// LDREX
								break;
							case 2:
								// STREXD
								break;
							case 3:
								// LDREXD
								break;
							case 4:
								// STREXB
								break;
							case 5:
								// LDREXB
								break;
							case 6:
								// STREXH
								break;
							case 7:
								// LDREXH
								break;
							}
							break;
						}
						break;
					case 1:
						// LDRH/STRH
						if (bit(instr, 20) == 0)
						{
							if (bit(instr, 22) == 0)
							{
								// STRHT/STRH reg
							}
							else
							{
								// STRHT/STRH imm
							}
						}
						else
						{
							if (bit(instr, 22) == 0)
							{
								// LDRHT/LDRH reg
							}
							else
							{
								// LDRHT/LDRH imm
							}
						}
						break;
					case 2:
						if (bit(instr, 20) == 0)
						{
							if (bit(instr, 22) == 0)
							{
								// LDRD reg
							}
							else
							{
								// LDRD imm
							}
						}
						else
						{
							if (bit(instr, 22) == 0)
							{
								// LDRSBT/LDRSB reg
							}
							else
							{
								// LDRSBT/LDRSB imm
							}
						}
						break;
					case 3:
						if (bit(instr, 20) == 0)
						{
							if (bit(instr, 22) == 0)
							{
								// STRD reg
							}
							else
							{
								// STRD imm
							}
						}
						else
						{
							if (bit(instr, 22) == 0)
							{
								// LDRST/LDRS reg
							}
							else
							{
								// LDRST/LDRS imm
							}
						}
						break;
					}
				}
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
				new_pc= check_media_instr(instr);
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


