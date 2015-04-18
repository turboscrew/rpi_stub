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
#define INSTR_COND_UNCOND 0xf0000000

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

// This is frequently used for secondary classification too
#define INSTR_BIT4_MASK 0x00000010

// Extension to secondary classification
#define INSTR_DPI2_MASK 0x00600000
#define INSTR_DPI2_0 0x00000000
#define INSTR_DPI2_1 0x00200000
#define INSTR_DPI2_2 0x00400000
#define INSTR_DPI2_3 0x00600000

// place for single data transfer immediate opcode
extern uint32_t sdt_imm;

// linear doesn't have branch address
// how can we tell which coding? T1/T2/A1/A2/...?
// maybe it's better to execute instructions with different registers?
// (asm needed)


// arith & log - so many different instructions that it's better
// to edit the instruction and execute it to find out effects
void check_op_pc(uint32_t instr, branch_info *info)
{
	// if dest is PC
	if ((instr & 0x0000f000) == 0x0000f000)
	{
		if ((instr & INSTR_COND_MASK) == INSTR_COND_AL)
		{
			info->btype = INSTR_BRTYPE_BRANCH;
			info->bval = (unsigned int) 0;
			return;
		}
		else
		{
			info->btype = INSTR_BRTYPE_CONDBR;
			info->bval = (unsigned int) 0;
			return;
		}
	}
	else
	{
		info->btype = INSTR_BRTYPE_LINEAR;
		info->bval = (unsigned int) 0;
		return;
	}
}

void check_branching(unsigned int address, branch_info *info)
{

	uint32_t instr;

	// some scratchpads for various uses
	uint32_t tmp1;
	uint32_t tmp2;
	uint32_t tmp3;
	uint32_t tmp4;
	uint64_t dtmp;
	uint32_t *iptr;

	instr = *((uint32_t *) address);

	if ((instr & INSTR_COND_MASK) == INSTR_COND_UNCOND)
	{
		// check unconditionals (= specials)
		info->btype = INSTR_BRTYPE_BRANCH;
		info->bval = (unsigned int) 0;
		return;
	}

	switch (instr & INSTR_CLASS_MASK)
	{
		case INSTR_C0:
			switch (instr & INSTR_SC_MASK)
			{
				case INSTR_SC_1:
					if (instr & INSTR_BIT4_MASK) // mult
					{
						tmp1 = instr & 0x00000f00;
						tmp1 = rpi2_reg_context.storage[tmp1];
						tmp2 = instr & 0x0000000f;
						tmp2 = rpi2_reg_context.storage[tmp2];
						dtmp = tmp1 * tmp2;
						// high dest is PC?
						if (instr & 0x000f0000)
						{
							info->btype = INSTR_BRTYPE_LINEAR;
							info->bval = (unsigned int) (dtmp >> 32);
						}
						// low dest is PC?
						else if (instr & 0x0000f000)
						{
							info->btype = INSTR_BRTYPE_LINEAR;
							info->bval = (unsigned int) (dtmp & 0xffffffff);
						}
						else
						{
							info->btype = INSTR_BRTYPE_LINEAR;
							info->bval = (unsigned int) 0;
						}
						return;
					}
					else
					{
						// arithmetic & locig
						check_op_pc(instr, info);
						return;
					}
					break;
				case INSTR_SC_2:
					if (instr & INSTR_BIT4_MASK)
					{
						// swap or msr/mrs
						// swaps,mrs,msr: if d == 15 then UNPREDICTABLE
						info->btype = INSTR_BRTYPE_LINEAR;
						info->bval = (unsigned int) 0;
						return;
					}
					else
					{
						// arithmetic & locig
						check_op_pc(instr, info);
						return;
					}
					break;
				default:
					// arithmetic & locig
					check_op_pc(instr, info);
					return;
					break;
			}
			break;
		case INSTR_C1:
			switch (instr & INSTR_SC_MASK)
			{
				case INSTR_SC_2:
					if (instr & INSTR_BIT4_MASK)
					{
						// msr imm
						info->btype = INSTR_BRTYPE_LINEAR;
						info->bval = (unsigned int) 0;
						return;
					}
					else
					{
						// arithmetic & locig
						check_op_pc(instr, info);
						return;
					}
					break;
				default:
					// arithmetic & locig
					check_op_pc(instr, info);
					return;
					break;
			}
			break;
		case INSTR_C2:
			// Single Data Transfer - imm
			// if wback && n == t then UNPREDICTABLE
			tmp1 = (instr & 0x00000fff); // immediate
			tmp2 = (instr & 0x01200000); // P and W bit
			if (!(instr & 0x00800000)) // immediate sign negative?
			{
				tmp1 = -tmp1;
			}

			if ((instr & 0x0000f000) == (15 << 12)) // if Rd = PC
			{
				if (instr & 0x00100000) // load
				{
					tmp3 = (instr & 0x000f0000) >> 16; // Rn
					tmp3 = rpi2_reg_context.storage[tmp3]; // (Rn)
#if 1
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
#else
					// usermode access?
					if (tmp2 == 0x01200000) //pre-indexing
					{
						// Value to be loaded in PC
						tmp4 = *((uint32_t *)(tmp3 + tmp1));
					}
					else // post-indexing works after setting Rd
					{
						tmp4 = *((uint32_t *)tmp3);
					}
					if (instr & 0x00400000) // byte
					{
						tmp4 &= 0x000000ff;
					}
#endif
					info->btype = INSTR_BRTYPE_BRANCH;
					info->bval = (unsigned int) tmp4;
				}
				else // store doesn't change Rd
				{
					info->btype = INSTR_BRTYPE_LINEAR;
					info->bval = (unsigned int) 0;
				}
				return;
			}
			else if ((instr & 0x000f0000) == (15 << 16)) // if Rn = PC
			{
				tmp3 = rpi2_reg_context.storage[15]; // PC
				if ((!tmp2) || (tmp2 == 0x01200000)) // pre/post-indexing?
				{
					tmp4 = tmp3 + tmp1;
					info->btype = INSTR_BRTYPE_BRANCH;
					info->bval = (unsigned int) tmp4;
				}
				else // no effect on PC
				{
					info->btype = INSTR_BRTYPE_LINEAR;
					info->bval = (unsigned int) 0;
				}
				return;
			}
			else // PC not involved
			{
				info->btype = INSTR_BRTYPE_LINEAR;
				info->bval = (unsigned int) 0;
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
				info->btype = INSTR_BRTYPE_BRANCH;
				info->bval = (unsigned int) 0;
				return;
			}
			else
			{
				info->btype = INSTR_BRTYPE_CONDBR;
				info->bval = (unsigned int) 0;
				return;
			}
			break;
		case INSTR_C6:
			// LDC/STC
			info->btype = INSTR_BRTYPE_LINEAR;
			info->bval = (unsigned int) 0;
			return;
			break;
		case INSTR_C7:
			if (instr & 0x01000000)
			{
				// Software interrupt
				info->btype = INSTR_BRTYPE_LINEAR;
				info->bval = (unsigned int) 0;
				return;
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
					info->btype = INSTR_BRTYPE_LINEAR;
					info->bval = (unsigned int) 0;
					return;
				}
			}
			break;
		}
}


