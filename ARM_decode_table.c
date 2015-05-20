/*
 * ARM_decode_table.c
 *
 *  Created on: May 15, 2015
 *      Author: jaa
 */

// ARM instruction set decoder
// This implementation uses decoding table and a secondary
// decoding for multiplexed instruction encodings.
//
// Multiplexed instruction encodings are instruction encodings
// common to two or more instructions that cannot be told apart
// by masking and comparing the result to the data.
// They are often special cases - some field has a certain value.
//
// The decoding table is generated from a spreadsheet and edited
// mostly by scripts. The extra-field enum-names and decoder-function
// names are also defined in the spreadsheet.
// The outcome is in the files 'ARM_decode_table_data.h'
// ARM_decode_table_prototypes.h and ARM_decode_table_extras.h
//
// At this stage the function names and extra-field enum-values are
// just copied from the spreadsheet. The function names and the
// extra-values must match to those in the spreadsheet.

// the register context
#include "rpi2.h"
#include "ARM_decode_table.h"

// Extra info to help in decoding.
// Especially useful for decoding multiplexed instructions
// (= different instructions that have same mask and data).
// These are read from a file generated from the
// spreadsheet. The comma from the last entry is removed
// manually.
// TODO: add "UNPREDICTABLE"-bits check: the (0)s and (1)s.
// At the moment they are ignored.
typedef enum
{
#include "ARM_decode_table_extras.h"
} ARM_decode_extra_t;

// Decoding function prototypes are read from a file generated from the
// spreadsheet.
#include "ARM_decode_table_prototypes.h"

// the decoding table type
typedef struct
{
	unsigned int data;
	unsigned int mask;
	ARM_decode_extra_t extra;
	instr_next_addr_t (*decoder)(unsigned int, ARM_decode_extra_t);
} ARM_dec_tbl_entry_t;

// the decoding table itself.
// (See the description in the beginning of this file.)
// The initializer contents are read from a file generated from the
// spreadsheet. The comma from the last entry is removed
// manually.
ARM_dec_tbl_entry_t ARM_decode_table[] =
{
#include "ARM_decode_table_data.h"
};

// set next address for linear execution
// the address is set to 0xffffffff and flag to INSTR_ADDR_ARM
// that indicates the main ARM decoding function to set the address
// right for linear execution.
inline instr_next_addr_t set_addr_lin(void)
{
	instr_next_addr_t retval = {
			.flag = INSTR_ADDR_ARM,
			.address = 0xffffffff
	};
	return retval;
}

// decoder dispatcher - finds the decoding function and calls it
// TODO: Partition the table search (use bits 27 - 25 of the instruction.
instr_next_addr_t ARM_decoder_dispatch(unsigned int instr)
{
	int instr_count = sizeof(ARM_decode_table);
	instr_next_addr_t retval;
	int i;

	retval = set_undef_addr();

	for (i=0; i<instr_count; i++)
	{
		if ((instr & ARM_decode_table[i].mask) == ARM_decode_table[i].data)
		{
			retval = ARM_decode_table[i].decoder(instr, ARM_decode_table[i].extra);
			break;
		}
	}
	return retval;
}

// the decoding functions

// Sub-dispatcher - handles the multiplexed instruction encodings
// TODO: finish when V-regs are available and handling of standards is
// more clear.
instr_next_addr_t arm_mux(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	unsigned int tmp1, tmp2; // scratchpads
	retval = set_undef_addr();

	switch (extra)
	{
	case arm_mux_vbic_vmvn:
		// Check cmode to see if it's VBIC (imm) or VMVN (imm)
		if (bitrng(instr, 11,9) != 7)
		{
			/* neither changes the program flow
			// either VBIC or VMVN
			if (bitrng(instr, 11, 10) == 3)
			{
				// VMVN
			}
			else if (bit(instr, 8))
			{
				// VBIC
			}
			else
			{
				// VMVN
			}
			*/
			retval = set_addr_lin();
		}
		// else UNDEFINED
		break;
	case arm_mux_wfe_wfi:
		// WFE,WFI
		if ((bitrng(instr, 7, 0) == 2) || (bitrng(instr, 7, 0) == 2))
		{
			// neither changes the program flow
			retval = set_addr_lin();
		}
		// else UNDEFINED
		break;
	case arm_mux_vshrn_q_imm:
		// VSHRN,VQSHR{U}N (imm)
		// if Vm<0> == '1' then UNDEFINED;
		if (bit(instr,0) == 0)
		{
			/* neither changes the program flow
			// bits 24 and 8 == 0 => VSHRN
			if ((bit(instr, 24) == 0) && (bit(instr, 8) == 0))
			{
				// VSHRN
			}
			else
			{
				// VQSHR{U}N
			}
			*/
			retval = set_addr_lin();
		}
		// else UNDEFINED
		break;
	case arm_mux_vrshrn_q_imm:
		// VRSHRN,VQRSHR{U}N (imm)
		// if Vm<0> == '1' then UNDEFINED;
		if (bit(instr,0) == 0)
		{
			/* neither changes the program flow
			// bits 24 and 8 == 0 => VRSHRN
			if ((bit(instr, 24) == 0) && (bit(instr, 8) == 0))
			{
				// VRSHRN
			}
			else
			{
				// VQRSHR{U}N
			}
			*/
			retval = set_addr_lin();
		}
		// else UNDEFINED
		break;
	case arm_mux_vshll_i_vmovl:
		// VSHLL(imm!=size,imm),VMOVL
		// if Vd<0> == '1' then UNDEFINED;
		if (bit(instr, 12) == 0)
		{
			// if imm3 == '000' then SEE "Related encodings";
			// if imm3 != '001' && imm3 != '010' && imm3 != '100' then SEE VSHLL;
			switch (bitrng(instr, 21, 19))
			{
			case 0:
				// UNDEFINED
				break;
			case 1:
			case 2:
			case 4:
				// VMOVL
				// doesn't change the program flow
				retval = set_addr_lin();
				break;
			default:
				// VSHLL
				// doesn't change the program flow
				retval = set_addr_lin();
				break;
			}
		}
		// else UNDEFINED
		break;
	case arm_mux_vorr_i_vmov_i:
		// VORR,VMOV,VSHR (imm)
		// bit 7=0 and bits 21-20=0 0 => not VSHR
		// bit 8=0 or bits 11-10=1 1 => VMOV
		// bit 5=0 and bit 8=1 and bits 11-10 != 1 1 => VORR
		// if Q == '1' && Vd<0> == '1' then UNDEFINED
		// Q = bit 6, Vd = bits 15-12
		if ((bit(instr, 6) == 1) && (bit(instr, 12) == 0))
		{
			if ((bit(instr,7) == 0) && (bitrng(instr, 21, 19) == 0))
			{
				// VORR/VMOV
				// if Q == '1' && Vd<0> == '1' then UNDEFINED
				// Q = bit 6, Vd = bits 15-12
				if ((bit(instr, 5) == 0) && (bit(instr, 8) == 1)
						&& (bitrng(instr, 11, 10) != 3))
				{
					// VORR
					// doesn't change the program flow
					retval = set_addr_lin();
				}
				else if ((bit(instr, 8) == 0) || (bitrng(instr, 11, 10) == 3))
				{
					// VMOV
					// doesn't change the program flow
					retval = set_addr_lin();
				}
				// else UNDEFINED
			}
			else
			{
				// VSHR
				// if Q == '1' && (Vd<0> == '1' || Vm<0> == '1') then UNDEFINED
				// Q = bit 6, Vd = bits 15-12, Vm = bits 3-0
				if ((bit(instr, 6) == 1) && (bit(instr, 0) == 0))
				{
					// doesn't change the program flow
					retval = set_addr_lin();
				}
				// else UNDEFINED
			}
		}
		// else UNDEFINED
		break;
	case arm_mux_lsl_i_mov:
	case arm_mux_lsl_i_mov_pc:
		// LSL(imm),MOV
		if (bitrng(instr, 11, 7) == 0)
		{
			// MOV reg
			if (extra == arm_mux_lsl_i_mov)
			{
				// MOV reg
			}
			else
			{
				// 'flags'-bit set?
				if (bit(instr, 20) == 1)
				{
					// return from exception
					// if CurrentModeIsHyp() then UNDEFINED;
					// elsif CurrentModeIsUserOrSystem() then UNPREDICTABLE
					// copy SPSR to CPSR (many state-related restrictions)
					// put result to PC
				}
				else
				{
					// MOV PC
				}
			}
		}
		else
		{
			// LSL imm
			if (extra == arm_mux_lsl_i_mov)
			{
				// LSL imm
			}
			else
			{
				// 'flags'-bit set?
				if (bit(instr, 20) == 1)
				{
					// return from exception
				}
				else
				{
					// LSL imm PC
				}
			}
		}
		break;
	case arm_mux_ror_i_rrx:
		// ROR(imm),RRX
		// The PC-variants are not multiplexed
		if (bitrng(instr, 11, 7) == 0)
		{
			// RRX reg
		}
		else
		{
			// ROR imm
		}
		break;
	case arm_mux_vmovn_q:
		// VQMOV{U}N,VMOVN
		// VQMOV{U}N: if op (bits 7-6) == '00' then SEE VMOVN;
		// 		if size == '11' || Vm<0> == '1' then UNDEFINED;
		// VMOVN: if size (bits 19-18) == '11' then UNDEFINED;
		// 		if Vm<0> (bit 0)== '1' then UNDEFINED;
		if ((bitrng(instr, 19, 18) != 3) && (bit(instr, 0) == 0))
		{
			/* neither changes the program flow
			if (bitrng(instr, 7, 6) == 0)
			{
				// VMOVN
			}
			else
			{
				// VQMOV{U}N
			}
			*/
			retval = set_addr_lin();
		}
		// else UNDEFINED
		break;
	case arm_mux_msr_r_pr:
		// MSR (reg) priv
		// user-mode: write_nzcvq = (mask<1> == '1'); write_g = (mask<0> == '1');
		// if mask (bits 19,18) == 0 then UNPREDICTABLE;
		// if R = 1, UNPREDICTABLE
		// other modes:
		// if mask (bits 19-16) == 0 then UNPREDICTABLE;
		// if n == 15 then UNPREDICTABLE
		// R = bit 22

		if (bitrng(rpi2_reg_context.reg.cpsr, 4, 0) == 16) // user mode
		{
			retval = set_addr_lin();
			if (bit(instr, 22) == 1) // SPSR
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			else if (bitrng(instr, 19, 18) == 0)
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			else if (bitrng(instr, 3, 0) == 15)
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			// else set APSR-bits
		}
		else if (bitrng(rpi2_reg_context.reg.cpsr, 4, 0) == 31) // system mode
		{
			retval = set_addr_lin();
			// mode-to-be
			tmp1 = rpi2_reg_context.storage[bitrng(instr, 3, 0)] & 0x1f;
			if ((tmp1 != 16) && (tmp1 != 31)) // user/system
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			else if (bitrng(instr, 19, 16) == 0) // mask = 0
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			else if (bitrng(instr, 3, 0) == 15)
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			// else set CPSR/SPSR
		}
		else
		{
			// TODO: add all mode restrictions
			retval = set_addr_lin();
			if (bitrng(instr, 19, 16) == 0) // mask = 0
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			else if (bitrng(instr, 3, 0) == 15)
			{
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			// else set CPSR/SPSR
		}
		break;
	case arm_mux_mrs_r_pr:
		// MRS (reg) priv
		// Rd=bits 11-8, if d == 15 then UNPREDICTABLE
		// user mode: copies APSR into Rd
		// MRS that accesses the SPSR is UNPREDICTABLE if executed in
		//   User or System mode
		// An MRS that is executed in User mode and accesses the CPSR
		// returns an UNKNOWN value for the CPSR.{E, A, I, F, M} fields.
		// R = bit 22
		// R[d] = CPSR AND '11111000 11111111 00000011 11011111';
		retval = set_addr_lin();
		tmp1 = bitrng(rpi2_reg_context.reg.cpsr, 4, 0) & 0x1f;
		if (tmp1 == 16) // user mode
		{
			// Rd = PC
			if (bitrng(instr, 11, 8) == 15)
			{
				if (bit(instr, 22) == 0) // CPSR
				{
					tmp2 = rpi2_reg_context.reg.cpsr & 0xf80f0000;
					retval.address = tmp2;
					retval.flag = INSTR_ADDR_ARM;
				}
				// UNPREDICTABLE anyway due to Rd = PC
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			// else the program flow is not changed
		}
		else if (tmp1 == 31) // system mode
		{
			// Rd = PC
			if (bitrng(instr, 11, 8) == 15)
			{
				if (bit(instr, 22) == 0) // CPSR
				{
					tmp2 = rpi2_reg_context.reg.cpsr & 0xf8ff03df;
					retval.address = tmp2;
					retval.flag = INSTR_ADDR_ARM;
				}
				// UNPREDICTABLE anyway due to Rd = PC
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			// else the program flow is not changed
		}
		else
		{
			// Rd = PC
			if (bitrng(instr, 11, 8) == 15)
			{
				if (bit(instr, 22) == 0) // CPSR
				{
					tmp2 = rpi2_reg_context.reg.cpsr & 0xf8ff03df;
					retval.address = tmp2;
					retval.flag = INSTR_ADDR_ARM;
				}
				else // SPSR
				{
					// TODO: store SPSR too in exception and get it here
					tmp2 = rpi2_reg_context.reg.cpsr;
					retval.address = tmp2;
					retval.flag = INSTR_ADDR_ARM;
				}
				// UNPREDICTABLE anyway due to Rd = PC
				retval = INSTR_ADDR_IMPL_DEP(retval);
			}
			// else the program flow is not changed
		}
		break;
	case arm_mux_msr_i_pr_hints:
		// MSR(imm),NOP,YIELD
		retval = set_addr_lin();
		if (bitrng(instr, 19, 16) != 0) // msr mask
		{
			// MSR
			if (bitrng(rpi2_reg_context.reg.cpsr, 4, 0) == 16) // user mode
			{
				if (bit(instr, 22) == 1) // SPSR
				{
					retval = INSTR_ADDR_IMPL_DEP(retval);
				}
				// else set APSR-bits
			}
			else if (bitrng(rpi2_reg_context.reg.cpsr, 4, 0) == 31) // system mode
			{
				// mode-to-be
				tmp1 = rpi2_reg_context.storage[bitrng(instr, 3, 0)] & 0x1f;
				if ((tmp1 != 16) && (tmp1 != 31)) // user/system
				{
					retval = INSTR_ADDR_IMPL_DEP(retval);
				}
				if (bit(instr, 22) == 1) // SPSR
				{
					retval = INSTR_ADDR_IMPL_DEP(retval);
				}
				// else set CPSR/SPSR
			}
			// else set CPSR/SPSR - privileged mode
			// the program flow is not changed
		}
		else
		{
			// hints - bits 7-0: hint opcode
			switch (bitrng(instr, 7, 0))
			{
			case 0: // NOP
			case 1: // YIELD
			case 2: // WFE
			case 3: // WFI
			case 4: // SEV
				// the program flow is not changed
				break;
			default:
				if ((bitrng(instr, 7, 0) & 0xf0) != 0xf0)
				{
					retval = set_undef_addr();
				}
				// else DBG - the program flow is not changed
				break;
			}
		}
		break;
	case arm_mux_vorr_vmov_nm:
		// VORR,VMOV (reg) - same Rn, Rm
		// VORR: if N (bit 7) == M (bit 5) && Vn == Vm then SEE VMOV (register);
		// if Q == '1' && (Vd<0> == '1' || Vn<0> == '1' || Vm<0> == '1') then UNDEFINED;
		// Vd = bits 15-12, Vn = bits 19-16, Vm = bits 3-0
		// VMOV: if !Consistent(M) || !Consistent(Vm) then SEE VORR (register);
		// if Q (bit 6)== '1' && (Vd<0> == '1' || Vm<0> == '1') then UNDEFINED;
		// Vm = bits 19-16 and 3-0, M = bit 7 and bit 5
		if ((bit(instr,16) == 0) && (bit(instr,12) == 0) && (bit(instr,0) ==0))
		{
			/* neither changes the program flow
			if ((bit(instr, 7)== bit(instr, 5))
					&& (bitrng(instr, 19, 16) == bitrng(instr, 3, 0)))
			{
				// VMOV
			}
			else
			{
				// VORR
			}
			*/
			retval = set_addr_lin();
		}
		// else UNDEFINED
		break;
	case arm_mux_vst_type:
		// VST1-4 (multiple element structures) - type
		// if Rm == 15, [<Rn>{:<align>}]
		// else if Rm == 13, [<Rn>{:<align>}]!; Rn = Rn + transfer_size
		// else [<Rn>{:<align>}], <Rm>; Rn = Rn + Rm
		// size = bits 7,6; align = bits 5,4
		// Rn = bits 19-16, Rm = bits 3-0
		// if Rn = 15, UNPREDICTABLE
		switch (bitrng(instr, 11, 8))
		{
		case 2:
			// VST1
			// regs = 4;
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			// (bitrng(instr, 5, 4)? 1: 4 << bitrng(instr, 5, 4))
			// vst1(instr, regs, align);
			// if Rn is PC
			if (bitrng(instr, 19, 16) == 15)
			{
				// if [<Rn>{:<align>}]
				if (bitrng(instr, 3, 0) == 15)
				{
					// Rn doesn't change
					retval = set_addr_lin();
				}
				else
				{
					// vst1(instr, 4, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
					retval = INSTR_ADDR_IMPL_DEP(retval);
				}
			}
			else
			{
				// PC not involved
				retval = set_addr_lin();
			}
			break;
		case 6:
			// VST1
			// regs = 3; if align<1> == '1' then UNDEFINED;
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if (bit(instr, 5) == 0)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst1(instr, 3, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 7:
			// VST1
			// regs = 1; if align<1> == '1' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if (bit(instr, 5) == 0)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst1(instr, 1, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 10:
			// VST1
			// regs = 2; if align == '11' then UNDEFINED;
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if (bitrng(instr, 5, 4) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst1(instr, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 3:
			// VST2
			// regs = 2; inc = 2;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			// vst2(instr, regs, inc, align);
			if (bitrng(instr, 7, 6) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst2(instr, 2, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 8:
			// VST2
			// regs = 1; inc = 1; if align == '11' then UNDEFINED;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if ((bitrng(instr, 5, 4) != 3) && (bitrng(instr, 7, 6) != 3))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst2(instr, 1, 1, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 9:
			// VST2
			// regs = 1; inc = 2; if align == '11' then UNDEFINED;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if ((bitrng(instr, 5, 4) != 3) && (bitrng(instr, 7, 6) != 3))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst2(instr, 1, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 4:
			// VST3
			// inc = 1;
			// if size == '11' || align<1> == '1' then UNDEFINED;
			// alignment = if align<0> == '0' then 1 else 8;
			// vst3(instr, inc, align);
			// vst3(instr, inc, (bit(instr, 4)? 1: 8));
			if ((bitrng(instr, 7, 6) != 3) && (bit(instr, 5) == 0))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst3(instr, 1, (bit(instr, 4)? 1: 8));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 5:
			// VST3
			// inc = 2;
			// if size == '11' || align<1> == '1' then UNDEFINED;
			// alignment = if align<0> == '0' then 1 else 8;
			if ((bitrng(instr, 7, 6) != 3) && (bit(instr, 5) == 0))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst3(instr, 2, (bit(instr, 4)? 1: 8));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 0:
			// VST4
			// inc = 1;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			// size = bits 7,6; align = bits 5,4
			// vst4(instr, inc, align);
			if (bitrng(instr, 7, 6) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst4(instr, 1, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 1:
			// VST4
			// inc = 2;
			// if size == '11' then UNDEFINED
			if (bitrng(instr, 7, 6) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vst4(instr, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		default:
			// UNDEFINED
			break;
		}
		break;
	case arm_mux_vld_type:
		// VLD1-4 (multiple element structures) - type
		// if Rm == 15, [<Rn>{:<align>}]
		// else if Rm == 13, [<Rn>{:<align>}]!; Rn = Rn + transfer_size
		// else [<Rn>{:<align>}], <Rm>; Rn = Rn + Rm
		// size = bits 7,6; align = bits 5,4
		// Rn = bits 19-16, Rm = bits 3-0
		// if Rn = 15, UNPREDICTABLE
		switch (bitrng(instr, 11, 8))
		{
		case 2:
			// VLD1
			// regs = 4;
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			// (bitrng(instr, 5, 4)? 1: 4 << bitrng(instr, 5, 4))
			// vld1(instr, regs, align);
			// if Rn is PC
			if (bitrng(instr, 19, 16) == 15)
			{
				// if [<Rn>{:<align>}]
				if (bitrng(instr, 3, 0) == 15)
				{
					// Rn doesn't change
					retval = set_addr_lin();
				}
				else
				{
					// vld1(instr, 4, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
					retval = INSTR_ADDR_IMPL_DEP(retval);
				}
			}
			else
			{
				// PC not involved
				retval = set_addr_lin();
			}
			break;
		case 6:
			// VLD1
			// regs = 3; if align<1> == '1' then UNDEFINED;
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if (bit(instr, 5) == 0)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld1(instr, 3, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 7:
			// VLD1
			// regs = 1; if align<1> == '1' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if (bit(instr, 5) == 0)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld1(instr, 1, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 10:
			// VLD1
			// regs = 2; if align == '11' then UNDEFINED;
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if (bitrng(instr, 5, 4) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld1(instr, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 3:
			// VLD2
			// regs = 2; inc = 2;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			// vld2(instr, regs, inc, align);
			if (bitrng(instr, 7, 6) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld2(instr, 2, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 8:
			// VLD2
			// regs = 1; inc = 1; if align == '11' then UNDEFINED;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if ((bitrng(instr, 5, 4) != 3) && (bitrng(instr, 7, 6) != 3))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld2(instr, 1, 1, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 9:
			// VLD2
			// regs = 1; inc = 2; if align == '11' then UNDEFINED;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			if ((bitrng(instr, 5, 4) != 3) && (bitrng(instr, 7, 6) != 3))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld2(instr, 1, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 4:
			// VLD3
			// inc = 1;
			// if size == '11' || align<1> == '1' then UNDEFINED;
			// alignment = if align<0> == '0' then 1 else 8;
			// vld3(instr, inc, align);
			if ((bitrng(instr, 7, 6) != 3) && (bit(instr, 5) == 0))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld3(instr, 1, (bit(instr, 4)? 1: 8));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 5:
			// VLD3
			// inc = 2;
			// if size == '11' || align<1> == '1' then UNDEFINED;
			// alignment = if align<0> == '0' then 1 else 8;
			if ((bitrng(instr, 7, 6) != 3) && (bit(instr, 5) == 0))
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld3(instr, 2, (bit(instr, 4)? 1: 8));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 0:
			// VLD4
			// inc = 1;
			// if size == '11' then UNDEFINED
			// alignment = if align == '00' then 1 else 4 << UInt(align);
			// size = bits 7,6; align = bits 5,4
			// vld4(instr, inc, align);
			if (bitrng(instr, 7, 6) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld4(instr, 1, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		case 1:
			// VLD4
			// inc = 2;
			// if size == '11' then UNDEFINED
			if (bitrng(instr, 7, 6) != 3)
			{
				// if Rn is PC
				if (bitrng(instr, 19, 16) == 15)
				{
					// if [<Rn>{:<align>}]
					if (bitrng(instr, 3, 0) == 15)
					{
						// Rn doesn't change
						retval = set_addr_lin();
					}
					else
					{
						// vld4(instr, 2, (bitrng(instr, 5, 4)? 1: (4 << bitrng(instr, 5, 4))));
						retval = INSTR_ADDR_IMPL_DEP(retval);
					}
				}
				else
				{
					// PC not involved
					retval = set_addr_lin();
				}
			}
			// else UNDEFINED
			break;
		default:
			// UNDEFINED
			break;
		}
		break;
	default:
		break;
	}
	return retval;
}

instr_next_addr_t arm_branch(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_coproc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_div(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_mac(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_maca(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_macd(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_pack(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_par(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_sat(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_bit(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_data_std_r(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();
	return retval;
}

instr_next_addr_t arm_core_data_std_pcr(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();
	return retval;
}

instr_next_addr_t arm_core_data_std_sh(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();
	return retval;
}

instr_next_addr_t arm_core_data_std_i(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();
	return retval;
}

instr_next_addr_t arm_core_data_std_ipc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();
	return retval;
}

instr_next_addr_t arm_core_exc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_ldst(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();


	switch (bitrng(instr, 27, 25))
	{
	case 0:
		// TODO: add a couple of handlers to decode table
		break;
	case 2:
		// immediate
		break;
	case 3:
		// register
		break;
	default:
		// shouldn't get here
		break;
	}
	return retval;
}

instr_next_addr_t arm_core_ldstm(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	/*
	 * if bits 24 - 20: B I M W L
	 * B 1=before, 0=after
	 * I 1=increment, 0=decrement
	 * M 1=other mode, 0=current mode
	 * W 1=writeback, 0=no writeback
	 * L 1=load. 0=store
	 */
	return retval;
}

instr_next_addr_t arm_core_ldstrd(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_ldstrex(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_ldstrh(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_ldstrsb(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_ldstsh(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_core_status(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_fp(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_v_bits(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_v_comp(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_v_mac(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_v_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_v_par(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_v_shift(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_vfp_ldst_elem(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_vfp_ldst_ext(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}

instr_next_addr_t arm_vfp_xfer_reg(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	retval = set_undef_addr();

	return retval;
}
