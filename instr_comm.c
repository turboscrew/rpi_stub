/*
instr_comm.c

Copyright (C) 2015 Juha Aaltonen

This file is part of standalone gdb stub for Raspberry Pi 2B.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "instr_comm.h"
#include "rpi2.h"
#include <stdint.h>
#include "log.h"
// places for opcodes that we need to actually execute
// extern uint32_t mrs_regb;
extern uint32_t executed_instruction;
// another place for opcode - for reading other mode registers
extern unsigned int read_instruction;

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

// CP15 accessing - must be macros, because there are parameters
// that go into an assembly code as constant literals
#define READ_CP15(retvar, opc, crn, crm, opc2) \
	asm volatile (\
		"mrc p15, %c[op], %[reg], c%c[rn], c%c[rm], %c[op2]\n\t"\
		:[reg] "=r" (retvar)\
		:[op]"I"(opc), [rn]"I"(crn), [rm]"I"(crm), [op2]"I"(opc2):\
	)

#define WRITE_CP15(value, opc, crn, crm, opc2) \
	asm volatile (\
		"mcr p15, %c[op], %[reg], c%c[rn], c%c[rm], %c[op2]\n\t"\
		::[reg]"r"(value), [op]"I"(opc), [rn]"I"(crn), [rm]"I"(crm),\
		[op2]"I"(opc2):\
	)

#define READ_REG_MODE(retvar, rg, mode) \
    asm volatile (\
    "mrs %[reg], r%c[rn]_" #mode "\n\t"\
    :[reg] "=r" (retvar)\
    :[rn]"I"(rg):\
    )

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
void exec_instr(unsigned int instr, unsigned int r0, unsigned int r1,
		unsigned int r2, unsigned int r3)
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

// The next function needs this
unsigned int instr_comm_regret = 0;
// reads banked register of another mode
// mode is processor mode (INSTR_PMODE_XXX)
// reg is register number (16 = spsr)
unsigned int get_mode_reg(unsigned int mode, unsigned int reg)
{
	unsigned int retval;
	unsigned int *rd_ptr;
	unsigned int tmp = 0x01000200;
	unsigned int secure_mode = 0;
	static unsigned int mode_store; // due to different SPs used
	instr_comm_regret = 0;

	// 0x01000200 = mrs r1, r8_usr (R and M fields are zero)
	// R = bit 22; if R = 1 lr => spsr
	// M = bits 19-16, 8

	rd_ptr = (unsigned int *) read_instruction;

	// if monitor mode
	if (mode == INSTR_PMODE_MON)
	{
		switch (reg)
		{
		case 13: // sp_mon
			tmp = 0x01a0100d; // mov r1, r13
			*rd_ptr = tmp;
			break;
		case 14: // lr_mon
			tmp = 0x01a0100e; // mov r1, r14
			*rd_ptr = tmp;
			break;
		case 16: // spsr_mon
			tmp = 0x014f1000; // mrs r1, spsr
			*rd_ptr = tmp;
			break;
		default:
			// unpredictable
			tmp = 0;
			break;
		}
	}
	else if ((mode == INSTR_PMODE_USR) || (mode == INSTR_PMODE_SYS)
		|| (mode == INSTR_PMODE_FIQ))
	{
		if((reg == 16) && (mode == INSTR_PMODE_FIQ)) // spsr_fiq
		{
			tmp |= 0x0e << 16;
			tmp |= 1 << 22; // R-bit
			*rd_ptr = tmp;
		}
		else if ((reg > 7) && (reg < 15))
		{
			if (mode == INSTR_PMODE_FIQ)
			{
				tmp |= 1 << 19;
			}
			tmp |= (reg - 8) << 16;
			*rd_ptr = tmp;
		}
		else
		{
			// illegal
			tmp = 0;
		}
	}
	else
	{
		if (mode == INSTR_PMODE_HYP)
		{
			tmp |= ((1 << 8) | (1 << 19));
			switch (reg)
			{
			case 13:
				tmp |= 0x07 << 16;
				break;
			case 14:
				tmp |= 0x06 << 16;
				break;
			case 16:
				tmp |= 1 << 22; // R-bit
				tmp |= 0x06 << 16;
				break;
			default:
				tmp = 0;
			}
		}
		else
		{
			tmp |= 1 << 8;
			if ((reg == 13) || (reg == 14) || (reg == 16))
			{
				switch (mode)
				{
				case INSTR_PMODE_IRQ:
					tmp |= 0x0 << 17;
					break;
				case INSTR_PMODE_SVC:
					tmp |= 0x1 << 17;
					break;
				case INSTR_PMODE_ABT:
					tmp |= 0x2 << 17;
					break;
				case INSTR_PMODE_UND:
					tmp |= 0x3 << 17;
					break;
				default:
					tmp = 0;
					break;
				}
				if (tmp)
				{
					switch (reg)
					{
					case 13:
						tmp |= 1 << 16;
						break;
					case 14:
						tmp |= 0 << 16;
						break;
					case 16:
						tmp |= 1 << 22; // R-bit
						tmp |= 0 << 16;
						break;
					default:
						tmp = 0;
					}
				}
			}
			else
			{
				tmp = 0;
			}
		}
	}

	if (!tmp) // bad instruction
	{
		read_instruction = 0; // mark instruction as bad
		return 0;
	}

	// ensure secure mode
	// SCR.SCD (bit 7) = 0 - enable SMC
	// HCR.TSC (bit 19) = 0 - SMC is not trapped to HYP
	// reset should zero both registers

	/* Let's have another look - probably SMC is needed
	secure_mode = get_SCR();
	if (secure_mode & 1)
	{
		set_SCR(secure_mode & ~(1)); // clear lowest bit
	}
	*/

/*
    asm volatile (
    "@\n\t"
    "@ set monitor mode - least restrictions\n\t"
    "push {r0, r1}\n\t"
    "ldr r0, =mode_store\n\t"
	"mrs r1, cpsr\n\t"
	"str r1, [r0]\n\t"
    "and r1, r1, 0x1f @ clear M\n\t"
    "or r1, r1, 0x16 @ set monitor\n\t"
    "msr cpsr, r1\n\t"
    "@\n\t"
    "@ read reg\n\t"
    "read_instruction:\n\t"
    "mrs r1, r0_usr @ to be patched\n\t"
    "ldr r0, =regret\n\t"
 	"str r1, [r0]\n\t"
   	"@\n\t"
    "@ restore mode\n\t"
    "ldr r0, =mode_store\n\t"
    "ldr r1, [r0]\n\t"
    "msr cpsr, r1\n\t"
    "pop {r0, r1}\n\t"
    );
    // restore security mode
*/

    asm volatile (
    "@\n\t"
    "push {r0, r1}\n\t"
    "@ read reg\n\t"
    "read_instruction:\n\t"
    "mrs r1, r8_usr @ to be patched\n\t"
    "ldr r0, =instr_comm_regret\n\t"
 	"str r1, [r0]\n\t"
   	"@\n\t"
    "pop {r0, r1}\n\t"
    );

	return retval;
}


// return the value of 'pos'th bit (31 - 0)
inline unsigned int bit(unsigned int value, int pos)
{
	return ((value >> pos) & 1);
}

// return the value of bits in the range
inline unsigned int bitrng(unsigned int value, int hi, int lo)
{
	return ((value >> lo) & ((1 << (hi - lo + 1)) - 1));
}

// return the value formed by bits selected by the mask
unsigned int bits(unsigned int value, unsigned int mask)
{
	unsigned int retval = 0;
	unsigned int curr_bit = 0;
	unsigned int tmp;
	int i, j;

	j = 0; // value bit counter
	tmp = 0;

	// all mask bits
	for (i = 0; i < 32; i++)
	{
		tmp = mask & (1 << i); // get a mask bit
		if (tmp)
		{
			tmp &= value; // get the masked bit from value
			tmp >>= i; // and make it the lowest bit
			retval |= tmp << (j++); // put as next bit in retval
		}
	}
	return retval;
}

// return UNDEFINED address
inline instr_next_addr_t set_undef_addr(void)
{
	instr_next_addr_t und_addr = {
			.address = 0,
			.flag = INSTR_ADDR_UNDEF
	};
	return und_addr;
}

// mark address as UNPREDICTABLE
inline instr_next_addr_t set_unpred_addr(instr_next_addr_t addr)
{
	addr.flag |= INSTR_ADDR_UNPRED;
	return addr;
}

// return address as valid ARM-mode address
inline instr_next_addr_t set_arm_addr(unsigned int addr)
{
	instr_next_addr_t tmp;
	tmp.address = addr;
	tmp.flag = INSTR_ADDR_ARM;
	return tmp;
}

// return address as valid Thumb-mode address
inline instr_next_addr_t set_thumb_addr(unsigned int addr)
{
	instr_next_addr_t tmp;
	tmp.address = addr;
	tmp.flag = INSTR_ADDR_THUMB;
	return tmp;
}

// returns sign-extended 32-bit value
inline int sx32(unsigned int val, int hi, int lo)
{
	unsigned int tmp = 0;
	// if negative, add sign extension bits
	if (bit(val, hi)) tmp = (~0) << hi;
	tmp |= bitrng(val, hi, lo); // add value-bits
	return (int) tmp;
}

// returns zero if instruction condition doesn't match the
// stored CPSR condition flags (makes instruction a NOP)
int will_branch(unsigned int instr)
{
	unsigned int flags;
	if ((instr & 0xf0000000) == 0xf << 28) return 1; // NV / special
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

// get reg from usr mode
unsigned int get_usr_reg(unsigned int reg)
{
	unsigned int retval = 0;

	if (reg < 8)
	{
		// regs 0 - 7 are common to all modes
		retval = rpi2_reg_context.storage[reg];
	}
	switch (reg)
	{
	case 8:
		READ_REG_MODE(retval, 8, usr);
		break;
	case 9:
		READ_REG_MODE(retval, 9, usr);
		break;
	case 10:
		READ_REG_MODE(retval, 10, usr);
		break;
	case 11:
		READ_REG_MODE(retval, 11, usr);
		break;
	case 12:
		READ_REG_MODE(retval, 12, usr);
		break;
	case 13:
		// because asm doesn't support 'r13_<mode>'
	    asm volatile (\
	    "mrs %[reg], SP_usr\n\t"\
	    :[reg] "=r" (retval)::\
	    );
		break;
	case 14:
		// because asm doesn't support 'r13_<mode>'
	    asm volatile (\
	    "mrs %[reg], LR_usr\n\t"\
	    :[reg] "=r" (retval)::\
	    );
		break;
	case 15:
		// there is no 'user mode PC' or equivalent
		retval = rpi2_reg_context.storage[15];
		break;
	default:
		// shouldn't get here
		break;
	}
	return retval;
}

// find out which extensions are implemented
unsigned int read_ID_PFR1()
{
	unsigned int retval;
	READ_CP15(retval, 0, 0, 1, 1);
	return retval;
}

// get processor mode
unsigned int get_proc_mode()
{
	return rpi2_reg_context.reg.cpsr & 0x1f;
}

// checks if processor mode was any of the suggested
// needless arguments can be set to zero
unsigned int check_proc_mode(unsigned int mode1, unsigned int mode2,
		unsigned int mode3, unsigned int mode4)
{
	uint32_t m = rpi2_reg_context.reg.cpsr & 0x1f;
	if (m == mode1) return mode1;
	if (m == mode2) return mode2;
	if (m == mode3) return mode3;
	if (m == mode4) return mode4;
	return 0;
}

unsigned int get_elr_hyp()
{
	unsigned int retval;
	asm volatile (
		"mrs %[reg], ELR_hyp\n\t"
		:[reg] "=r" (retval)
	);
	return retval;
}

unsigned int get_HCR()
{
	unsigned int retval;
	READ_CP15(retval, 4, 1, 1, 0);
	return retval;
}

unsigned int get_SCR()
{
	unsigned int retval;
	READ_CP15(retval, 0, 1, 1, 0);
	return retval;
}

void set_SCR(unsigned int m)
{
	WRITE_CP15(m, 0, 1, 1, 0);
}

unsigned int get_NSACR()
{
	unsigned int retval;
	READ_CP15(retval, 0, 1, 1, 2);
	return retval;
}


// returns 1 if secure state, 0 otherwise
unsigned int get_security_state()
{
	return ((get_SCR() & 1) == 0); // SCR bit 0 == 0 -> secure state
}

// returns 1 if coprocessor is accessible in non-secure state
// if coproc == 16 -> access to FIQ banked regs in non-safe mode
// if coproc = 11 or 10 return 4-bit status
// (cp 11 and cp10 are handled together)
unsigned int check_coproc_access(unsigned int coproc)
{
	unsigned int retval, cp_reg_val;
	if (coproc == 15) return 1;
	READ_CP15(cp_reg_val, 0, 1, 1, 2); // Non-Secure Access Control Register
	if (coproc == 16) return (cp_reg_val >> 19) & 1; // FIQ banked regs
	if (coproc == 14) return (cp_reg_val >> 20) & 1;
	if (coproc == 10 || coproc == 11)
	{
		retval = (cp_reg_val >> 10) & 0x03; // cp 11 and cp10 enables
		retval |= (cp_reg_val >> 12) & 0x0c; // SIMD and D16-D31 enables
		return retval;
	}
	if ((coproc == 13) || (coproc == 12) || (coproc < 10))
		return (cp_reg_val >> coproc) & 1;
	return 0; // no such coproc, so no access.
}

/*
stuff for state and state-transition checks

mrc, p15, opc, rn, crn, crm, opc2

// Which extensions do we have and are them enabled
HaveSecurityExt()
HaveVirtExt()

Implemented:
MRC p15, 0, <Rt>, c0, c1, 0 ; Read ID_PFR0 into Rt
bits[7:4] = 0b0011 both 16 and 32-bit thumb
bits[3:0] = 0b0001 ARM instructions
MRC p15, 0, <Rt>, c0, c1, 1 ; Read ID_PFR1 into Rt
bits[19:16]= 0b0001 Generic Timer Extension implemented
bits[15:12]= 0b0001 Virtualization Extensions implemented.
bits[11:8]= 0b0010 Support for two-stack programmers' model.
bits[7:4]=	0b0001 Security Extensions implemented.
			0b0010 As for 0b0001, and adds the ability to set the NSACR.RFR bit.
bits[3:0]=	0b0001 SupportedSupport for the standard programmers' model for ARMv4 and later. Model must support User, FIQ,
			IRQ, Supervisor, Abort, Undefined and System modes
MRC p15, 0, <Rt>, c0, c1, 4 ; Read ID_MMFR0 into Rt
bits[7:4] = 0b0000 Not supported, if non-zero (R-profile), VMSA support must be zero
bits[3:0] = 0b0000 Not supported, if non-zero (A-profile), PMSA-support must be zero

MRC p15, 0, <Rt>, c0, c2, 0 ; Read ID_ISAR0 into Rt
...
MRC p15, 0, <Rt>, c0, c2, 4 ; Read ID_ISAR4 into Rt
(different instructions implemented)

MRC p15, 0, <Rt>, c0, c1, 2 ; Read ID_DFR0 into Rt
(debug-support)

MRC p15, 0, <Rt>, c1, c1, 2 ; Read NSACR into Rt
MCR p15, 0, <Rt>, c1, c1, 2 ; Write Rt to NSACR
(NSACR, Non-Secure Access Control Register, Security Extensions)

VMRS <Rt>, MVFR0 ; Read MVFR0 into Rt
VMRS <Rt>, MVFR1 ; Read MVFR1 into Rt
(supported Media and VFP Features)

MRC p15, 0, <Rt>, c1, c1, 0 ; Read SCR into Rt
MCR p15, 0, <Rt>, c1, c1, 0 ; Write Rt to SCR
(Secure Configuration Register - secure state only)

MRC p15, 0, <Rt>, c1, c0, 0 ; Read SCTLR into Rt
MCR p15, 0, <Rt>, c1, c0, 0 ; Write Rt to SCTLR
(The SCTLR provides the top level control of the system)

CPACR:
MRC p15, 0, <Rt>, c1, c0, 2 ; Read CPACR into Rt
MCR p15, 0, <Rt>, c1, c0, 2 ; Write Rt to CPACR
ASEDIS, bit[31]
Disable Advanced SIMD functionality:
0 This bit does not cause any instructions to be UNDEFINED.
1 All instruction encodings identified in the Alphabetical list of instructions on
page A8-300 as being Advanced SIMD instructions, but that are not VFPv3 or VFPv4
instructions, are UNDEFINED when accessed from PL1 and PL0 modes.

 */
