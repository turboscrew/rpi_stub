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
// Maybe:
// if condition code is not 1 1 1 1 then divide by bits 27 - 25
// under which the groups: 0 0 0, 0 1 1, the rest
// if condition code is 1 1 1 1, divide first by bits 27 - 25 != 0 0 1
// bits 27 - 25 == 0 0 1 and under which by bit 23.
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
				retval = set_unpred_addr(retval);
			}
			else if (bitrng(instr, 19, 18) == 0)
			{
				retval = set_unpred_addr(retval);
			}
			else if (bitrng(instr, 3, 0) == 15)
			{
				retval = set_unpred_addr(retval);
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
				retval = set_unpred_addr(retval);
			}
			else if (bitrng(instr, 19, 16) == 0) // mask = 0
			{
				retval = set_unpred_addr(retval);
			}
			else if (bitrng(instr, 3, 0) == 15)
			{
				retval = set_unpred_addr(retval);
			}
			// else set CPSR/SPSR
		}
		else
		{
			// TODO: add all mode restrictions
			retval = set_addr_lin();
			if (bitrng(instr, 19, 16) == 0) // mask = 0
			{
				retval = set_unpred_addr(retval);
			}
			else if (bitrng(instr, 3, 0) == 15)
			{
				retval = set_unpred_addr(retval);
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
				retval = set_unpred_addr(retval);
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
				retval = set_unpred_addr(retval);
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
				retval = set_unpred_addr(retval);
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
					retval = set_unpred_addr(retval);
				}
				// else set APSR-bits
			}
			else if (bitrng(rpi2_reg_context.reg.cpsr, 4, 0) == 31) // system mode
			{
				// mode-to-be
				tmp1 = rpi2_reg_context.storage[bitrng(instr, 3, 0)] & 0x1f;
				if ((tmp1 != 16) && (tmp1 != 31)) // user/system
				{
					retval = set_unpred_addr(retval);
				}
				if (bit(instr, 22) == 1) // SPSR
				{
					retval = set_unpred_addr(retval);
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
					retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
					retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
						retval = set_unpred_addr(retval);
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
	// The above needs to be done to find out if the instruction is UNDEFINED or UNPREDICTABLE.
	// That's why we check the condition here
	if ((retval.flag & (~INSTR_ADDR_UNPRED)) == INSTR_ADDR_ARM)
	{
		// if the condition doesn't match
		if (!will_branch(instr))
		{
			retval = set_addr_lin();
		}
	}
	return retval;
}

instr_next_addr_t arm_branch(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	int baddr = 0;

	retval = set_undef_addr();

	if (will_branch(instr))
	{
		switch (extra)
		{
		case arm_bra_b_lbl:
		case arm_bra_bl_lbl:
			baddr = (int) rpi2_reg_context.reg.r15; // PC
			baddr += (sx32(instr, 23, 0) << 2);
			retval = set_arm_addr((unsigned int) baddr);
			break;
		case arm_bra_blx_lbl:
			baddr = (int) rpi2_reg_context.reg.r15; // PC
			baddr += (sx32(instr, 23, 0) << 2) | (bit(instr, 24) << 1);
			retval = set_thumb_addr((unsigned int) baddr);
			break;
		case arm_bra_bx_r:
		case arm_bra_blx_r:
		case arm_bra_bxj_r:
			// from Cortex-A7 mpcore trm:
			// "The BXJ instruction behaves as a BX instruction"
			// if bit 0 of the address is '1' -> switch to thumb-state.
			retval.address = rpi2_reg_context.storage[bitrng(instr, 3, 0)];
			if (bit(retval.address, 0))
				retval = set_thumb_addr(retval.address & ((~0) << 1));
			else
				retval = set_arm_addr(retval.address & ((~0) << 2));
			if (bitrng(instr, 3, 0) == 15) // PC
				retval = set_unpred_addr(retval);
			break;
		default:
			// shouldn't get here
			break;
		}
	}
	else
	{
		// No condition match - NOP
		retval = set_addr_lin();
	}
	return retval;
}

instr_next_addr_t arm_coproc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	unsigned int tmp;
	retval = set_undef_addr();
	// coproc 15 = system control, 14 = debug
	// coproc 10, 11 = fp and vector
	// coproc 8, 9, 12, 13 = reserved => UNDEFINED
	// coproc 0 - 7 = vendor-specific => UNPREDICTABLE
	// if Rt or RT2 = PC or SP => UNPREDICTABLE
	// TODO: add checks for valid known coprocessor commands
	tmp = bitrng(instr, 11, 8); // coproc
	if (!((tmp == 8) || (tmp == 9) || (tmp == 12) || (tmp == 13)))
	{
		switch(extra)
		{
		case arm_cop_mcrr2:
		case arm_cop_mcrr:
			// if Rt or Rt2 is PC
			if ((bitrng(instr, 19, 16) == 15) || (bitrng(instr, 15, 12) == 15))
			{
				/*
				if ((bitrng(instr, 19, 16) == 15) && (bitrng(instr, 15, 12) == 15))
				{
					// Rt2 = Rt = PC
					// execute: Rt2 = Rt = R0 if valid

				}
				else
				{
					// execute : Rt2 = R0, Rt = R1 if valid

					// find new PC value
					if (bitrng(instr, 19, 16) == 15)
					{
						// Rt2 = PC
					}
					else
					{
						// Rt = PC
					}
				}
				*/
				// at the moment, assume (falsely) linear, but unpredictable
				retval = set_addr_lin();
				retval = set_unpred_addr(retval);
			}
			else
			{
				retval = set_addr_lin();
			}
			break;
		case arm_cop_mcr2:
		case arm_cop_mcr:
			retval = set_addr_lin();
			tmp = bitrng(instr, 15, 12);
			if (tmp == 15)
			{
				// at the moment, assume (falsely) linear, but unpredictable
				retval = set_unpred_addr(retval);
			}
			else if (tmp == 13)
			{
				retval = set_unpred_addr(retval);
			}
			// else no effect on program flow
			break;
		case arm_cop_ldc2:
		case arm_cop_ldc:
			// if P U D W = 0 0 0 0 => UNDEFINED
			if (bitrng(instr, 24, 21) != 0)
			{
				retval = set_addr_lin();
				retval = set_unpred_addr(retval);
			}
			break;
		case arm_cop_ldc2_pc:
		case arm_cop_ldc_pc:
			// if P U D W = 0 0 0 0 => UNDEFINED
			if (bitrng(instr, 24, 21) != 0)
			{
				retval = set_addr_lin();
				retval = set_unpred_addr(retval);
			}
			break;
		case arm_cop_mrrc2:
		case arm_cop_mrrc:
			retval = set_addr_lin();
			retval = set_unpred_addr(retval);
			break;
		case arm_cop_mrc2:
		case arm_cop_mrc:
			retval = set_addr_lin();
			retval = set_unpred_addr(retval);
			break;
		case arm_cop_stc2:
		case arm_cop_stc:
			// if P U D W = 0 0 0 0 => UNDEFINED
			if (bitrng(instr, 24, 21) != 0)
			{
				retval = set_addr_lin();
				retval = set_unpred_addr(retval);
			}
			break;
		case arm_cop_cdp2:
		case arm_cop_cdp:
			// coproc 101x => fp instr
			retval = set_addr_lin();
			retval = set_unpred_addr(retval);
			break;
		default:
			// shouldn't get here
			break;
		}
	}
	return retval;
}

instr_next_addr_t arm_core_data_div(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	unsigned int tmp1, tmp2, tmp3;
	int stmp1, stmp2, stmp3;
	retval = set_undef_addr();

	tmp1 = bitrng(instr, 19, 16); // Rd
	tmp2 = bitrng(instr, 11, 8);  // Rm
	tmp3 = bitrng(instr, 3, 0);   // Rn
	if (tmp1 == 15) // Rd = PC
	{
		if (extra == arm_div_sdiv)
		{
			stmp2 = (int)rpi2_reg_context.storage[tmp2];
			stmp3 = (int)rpi2_reg_context.storage[tmp3];
			if (stmp2 == 0) // zero divisor
			{
				// division by zero
				// if no division by zero exception, return zero
				retval = set_arm_addr(0);
			}
			else
			{
				// round towards zero
				// check the result sign
				if (bit(stmp3, 31) == bit(stmp2, 31))
				{
					// result is positive
					// round towards zero = trunc
					stmp1 = stmp3/stmp2;
				}
				else
				{
					// result is negative
					stmp1 = (((stmp3 << 1)/stmp2) + 1) >> 1;
				}
				// if no division by zero exception, return zero
				retval = set_arm_addr((unsigned int) stmp1);
			}
		}
		else
		{
			// arm_div_udiv
			tmp2 = rpi2_reg_context.storage[tmp2];
			tmp3 = rpi2_reg_context.storage[tmp3];
			if (tmp2 == 0) // zero divisor
			{
				// division by zero
				// if no division by zero exception, return zero
				retval = set_arm_addr(0);
			}
			else
			{
				// round towards zero = trunc
				tmp1 = tmp3/tmp2;
			}
			retval = set_arm_addr(tmp1);
		}
	}
	else
	{
		// PC not involved
		retval = set_addr_lin();
		if ((tmp2 == 15) || (tmp3 == 15))
			retval = set_unpred_addr(retval); // Why?
	}
	return retval;
}

instr_next_addr_t arm_core_data_mac(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	unsigned int tmp1, tmp2, tmp3, tmp4;
	int stmp1, stmp2, stmp3;
	long long int ltmp;

	retval = set_undef_addr();

	// Rd = bits 19-16, Rm = 11-8, Rn = 3-0, Ra = 15-12
	// if d == 15 || n == 15 || m == 15 || a == 15 then UNPREDICTABLE;
	tmp3 = bitrng(instr, 19, 16);

	if (tmp3 == 15) // if Rd = PC
	{
		tmp1 = bitrng(instr, 11, 8); // Rm
		tmp2 = bitrng(instr, 3, 0); // Rn
		tmp1 = rpi2_reg_context.storage[tmp1];
		tmp2 = rpi2_reg_context.storage[tmp2];
		switch (extra)
		{
		case arm_cmac_mul:
			// multiply Rn and Rm and keep only the low 32 bits
		case arm_cmac_mla:
			// multiply Rn by Rm and add Ra; keep only the low 32 bits
		case arm_cmac_mls:
			// multiply Rn by Rm and subtract from Ra; keep only the low 32 bits
			// The low 32-bits are same whether signed or unsigned multiply
			ltmp = tmp1 * tmp2;
			tmp3 = (unsigned int)(ltmp & 0xffffffffL);
			if (extra == arm_cmac_mla)
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				tmp3 += tmp4;
			}
			else if (extra == arm_cmac_mls)
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				tmp3 = tmp4 - tmp3;
			}
			// else no accumulate
			retval = set_arm_addr(tmp3);
			retval = set_unpred_addr(retval);
			break;
		case arm_cmac_smulw:
			// signed multiply of Rn by 16 bits of Rm
		case arm_cmac_smlaw:
			// signed multiply of Rn by 16 bits of Rm and add Ra
			// only the top 32-bit of the 48-bit product is kept
			if (bit(instr, 6)) // which half of Rm
				stmp1 = (int)(short int)bitrng(tmp1, 31, 16);
			else
				stmp1 = (int)(short int)bitrng(tmp1, 15, 0);

			stmp2 = (int) tmp2;
			ltmp = stmp2 * stmp1;
			stmp3 = (int)((ltmp >> 16) & 0xffffffffL);
			if (extra == arm_cmac_smlaw)
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				stmp3 += (int)tmp4;
			}
			retval = set_arm_addr((unsigned int)stmp3);
			retval = set_unpred_addr(retval);
			break;
		case arm_cmac_smul:
			// signed multiply 16 bits of Rn by 16 bits of Rm
		case arm_cmac_smla:
			// signed multiply 16 bits of Rn by 16 bits of Rm and add Ra
			if (bit(instr, 6)) // which half of Rm
				stmp1 = (int)(short int)bitrng(tmp1, 31, 16);
			else
				stmp1 = (int)(short int)bitrng(tmp1, 15, 0);

			if (bit(instr, 5)) // which half of Rn
				stmp2 = (int)(short int)bitrng(tmp2, 31, 16);
			else
				stmp2 = (int)(short int)bitrng(tmp2, 15, 0);

			stmp3 = stmp2 * stmp1;
			if (extra == arm_cmac_smla)
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				stmp3 += (int)tmp4;
			}
			retval = set_arm_addr((unsigned int)stmp3);
			retval = set_unpred_addr(retval);
			break;
		case arm_cmac_smmul:
			// multiply Rn by Rm and keep only the top 32 bits
		case arm_cmac_smmla:
			// signed multiply Rn by Rm and add only the top 32 bits to Ra
			// exceptionally Ra can be PC without restrictions
			// if Ra == '1111' -> SMMUL
		case arm_cmac_smmls:
			// smmls is like smmla, but subtract from Ra
			ltmp = ((int)tmp1) * ((int)tmp2);
			if (bit(instr, 5)) ltmp += 0x80000000L; // round
			stmp3 = (int)((ltmp >> 32) & 0xffffffffL);
			if (extra == arm_cmac_smmla)
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				stmp3 += (int)tmp4;
			}
			else if (extra == arm_cmac_smmls)
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				stmp3 = (int)tmp4 - stmp3;
			}
			retval = set_arm_addr((unsigned int)stmp3);
			retval = set_unpred_addr(retval);
			break;
		case arm_cmac_smuad:
			// smuad: multiply upper and lower 16 bits (maybe Rm swapped)
			// and sum results
		case arm_cmac_smusd:
			// smusd is like smuad, but subtract (low - high)
		case arm_cmac_smlad:
			// smlad is like smuad, but sum is added to Ra
			// if Ra == '1111' -> SMUAD
		case arm_cmac_smlsd:
			// smlsd is like smusd but the difference is added to Ra
			// if Ra == '1111' -> SMUSD
			// M-bit = 5
			if (bit(instr, 5))
			{
				// swap Rm
				tmp3 = bitrng(tmp1, 31, 16);
				tmp3 |= bitrng(tmp1, 15, 0) << 16;
				tmp1 = tmp3;
			}
			// low 16 bits
			stmp1 = (int)(short int)(tmp1 & 0xffff);
			stmp2 = (int)(short int)(tmp2 & 0xffff);
			stmp3 = stmp1 * stmp2;
			// high 16 bits
			stmp1 = (int)(short int)((tmp1 >> 16) & 0xffff);
			stmp2 = (int)(short int)((tmp2 >> 16) & 0xffff);
			if ((extra == arm_cmac_smuad) || (extra == arm_cmac_smlad))
				stmp3 += stmp1 * stmp2;
			else
				stmp3 -= stmp1 * stmp2;

			if ((extra == arm_cmac_smlsd) || (extra == arm_cmac_smlad))
			{
				tmp4 = bitrng(instr, 15, 12); // Ra
				tmp4 = rpi2_reg_context.storage[tmp4];
				stmp3 += (int)tmp4;
			}
			retval = set_arm_addr((unsigned int) stmp3);
			retval = set_unpred_addr(retval);
			break;
		default:
			// shouldn't get here
			break;
		}
	}
	else
	{
		retval = set_addr_lin();
	}
	return retval;
}

instr_next_addr_t arm_core_data_macd(unsigned int instr, ARM_decode_extra_t extra)
{
	// Rm = bits 11-8, Rn=3-0, RdHi=19-16, RdLo=15-12, R/M/N=bit 5, M=bit 6
	// if dLo == 15 || dHi == 15 || n == 15 || m == 15 then UNPREDICTABLE;
	// if dHi == dLo then UNPREDICTABLE;
	instr_next_addr_t retval;
	unsigned int tmp1, tmp2, tmp3, tmp4;
	int stmp1, stmp2, stmp3;
	long long int ltmp;
	long long utmp;

	retval = set_undef_addr();

	tmp3 = bitrng(instr, 19, 16); // RdHi
	tmp4 = bitrng(instr, 15, 12); // RdLo

	if ((tmp3 == 15) || (tmp4 == 15))// if RdHi = PC or RdLo = PC
	{
		tmp1 = bitrng(instr, 11, 8); // Rm
		tmp2 = bitrng(instr, 3, 0); // Rn
		tmp1 = rpi2_reg_context.storage[tmp1];
		tmp2 = rpi2_reg_context.storage[tmp2];
		switch (extra)
		{
		case arm_cmac_smlal16:
			// signed multiply 16 bits of Rn by 16 bits of Rm and
			// add RdHi and RdLo as two 32-bit values to the result
			// then put the result to RdHi and RdLo as 64-bit value
			if (bit(instr, 6)) // which half of Rm
				stmp1 = (int)(short int)bitrng(tmp1, 31, 16);
			else
				stmp1 = (int)(short int)bitrng(tmp1, 15, 0);

			if (bit(instr, 5)) // which half of Rn
				stmp2 = (int)(short int)bitrng(tmp2, 31, 16);
			else
				stmp2 = (int)(short int)bitrng(tmp2, 15, 0);

			ltmp = (long long int)(((int)tmp1) * ((int)tmp2));
			stmp1 = (int) rpi2_reg_context.storage[tmp3];
			stmp2 = (int) rpi2_reg_context.storage[tmp4];
			ltmp += (long long int)(stmp1 + stmp2);
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((ltmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(ltmp & 0xffffffff);
			break;
		case arm_cmac_smlal:
			// signed multiply Rn by Rm and add RdHi and RdLo as 64-bit value
			// result is 64 bit value in RdHi and RdLo
			ltmp = (long long int)(((int)tmp1) * ((int)tmp2));
			ltmp += ((long long int) tmp3) << 32;
			ltmp += (long long int)((int) tmp4);
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((ltmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(ltmp & 0xffffffff);
			break;
		case arm_cmac_smull:
			// signed multiply Rn by Rm
			// result is 64 bit value in RdHi and RdLo
			ltmp = (long long int)(((int)tmp1) * ((int)tmp2));
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((ltmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(ltmp & 0xffffffff);
			break;
		case arm_cmac_umaal:
			// signed multiply Rn by Rm and
			// add RdHi and RdLo as two 32-bit values to the result
			// then put the result to RdHi and RdLo as 64-bit value
			ltmp = (long long int)(((int)tmp1) * ((int)tmp2));
			stmp1 = (int) rpi2_reg_context.storage[tmp3];
			stmp2 = (int) rpi2_reg_context.storage[tmp4];
			ltmp += (long long int)(stmp1 + stmp2);
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((ltmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(ltmp & 0xffffffff);
		break;
		case arm_cmac_umlal:
			// unsigned multiply Rn by Rm and add RdHi and RdLo as 64-bit value
			// result is 64 bit value in RdHi and RdLo
			utmp = (long long)(tmp1 * tmp2);
			utmp += ((long long)tmp3) << 32;
			utmp += (long long)tmp4;
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((utmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(utmp & 0xffffffff);
			break;
		case arm_cmac_umull:
			// unsigned multiply Rn by Rm
			// result is 64 bit value in RdHi and RdLo
			utmp = (long long)(tmp1 * tmp2);
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((ltmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(ltmp & 0xffffffff);
			break;
		case arm_cmac_smlald:
			// signed multiply upper and lower 16 bits (maybe Rm swapped)
			// and sum the products in RdHi and RdLo
			// result is 64 bit value in RdHi and RdLo
		case arm_cmac_smlsld:
			// smlsld is like smlald, but subtract (low - high)
			if (bit(instr, 5))
			{
				// swap Rm
				tmp3 = bitrng(tmp1, 31, 16);
				tmp3 |= bitrng(tmp1, 15, 0) << 16;
				tmp1 = tmp3;
				tmp3 = bitrng(instr, 19, 16); // restore tmp3
			}

			// low 16 bits
			stmp1 = (int)(short int)(tmp1 & 0xffff);
			stmp2 = (int)(short int)(tmp2 & 0xffff);
			stmp3 = stmp1 * stmp2;
			// high 16 bits
			stmp1 = (int)(short int)((tmp1 >> 16) & 0xffff);
			stmp2 = (int)(short int)((tmp2 >> 16) & 0xffff);
			if (extra == arm_cmac_smlald)
				stmp3 += stmp1 * stmp2;
			else
				stmp3 -= stmp1 * stmp2;
			stmp1 = (int)rpi2_reg_context.storage[tmp3];
			stmp2 = (int)rpi2_reg_context.storage[tmp4];
			ltmp = (long long int)stmp3;
			ltmp += ((long long int)stmp1) << 32;
			ltmp += (long long int)stmp2;
			if (tmp3 == 15) // if PC is RdHi
				tmp4 = (unsigned int)((ltmp >> 32) & 0xffffffff);
			else // PC is RdLo
				tmp4 = (unsigned int)(ltmp & 0xffffffff);
			break;
		default:
			// shouldn't get here
			break;
		}
		retval = set_arm_addr(tmp4);
		retval = set_unpred_addr(retval);
	}
	else
	{
		retval = set_addr_lin();
	}
	return retval;
}

instr_next_addr_t arm_core_data_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	instr_next_addr_t retval;
	unsigned int tmp1, tmp2, tmp3, tmp4;
	unsigned short htmp1, htmp2;

	retval = set_undef_addr();

	switch (extra)
	{
	case arm_cmisc_movw:
		// move 16-bit immediate to Rd (upper word zeroed)
	case arm_cmisc_movt:
		// move 16-bit immediate to top word of Rd
		// Rd = bits 15-12, imm = bits 19-16 and 11-0
		// if d == 15 then UNPREDICTABLE;
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp2 = bits(instr, 0x000f0fff); // immediate
			if (instr == arm_cmisc_movt)
			{
				tmp2 <<= 16;
				tmp1 = rpi2_reg_context.storage[tmp1];
				tmp1 &= 0x0000ffff;
				tmp2 |= tmp1;
			}
			retval = set_arm_addr(tmp2);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_clz:
		// counts leading zeroes in Rm and places the number in Rd
		// Rd = bits 15-12, Rm = bits 3-0
		// if d == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp2 = bitrng(instr, 3, 0); // Rm
			tmp2 = rpi2_reg_context.storage[tmp2];
			// this could be optimized
			for (tmp3=0; tmp3 < 32; tmp3++)
			{
				// if tmp3:th bit (from top) is set
				if (tmp2 & (1 << (31 - tmp3))) break;
			}
			retval = set_arm_addr(tmp3);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_bfc:
		// clears a bit field in Rd (bits 15-12)
		// msb = bits 20-16, lsb = bits 11-7
		// if d == 15 then UNPREDICTABLE;
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp2 = bitrng(instr, 20, 16); // msb
			tmp3 = bitrng(instr, 11, 7); // lsb
			// make the 'clear' mask
			tmp4 = (~0) << (tmp2 - tmp3 + 1);
			tmp4 = (~tmp4) << tmp3;
			tmp4 = ~tmp4;
			// clear bits
			tmp1 = rpi2_reg_context.storage[tmp1];
			tmp1 &= tmp4;
			retval = set_arm_addr(tmp1);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_bfi:
		// inserts (low)bits from Rn into bit field in Rd
		// Rd = bits 15-12, Rm = bits 3-0
		// msb = bits 20-16, lsb = bits 11-7
		// if d == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp2 = bitrng(instr, 20, 16); // msb
			tmp3 = bitrng(instr, 11, 7); // lsb
			// make the 'pick' mask
			tmp4 = (~0) << (tmp2 - tmp3 + 1);
			tmp4 = (~tmp4) << tmp3;
			// clear bits in Rd
			tmp1 = rpi2_reg_context.storage[tmp1];
			tmp1 &= (~tmp4);
			// get bits from Rm
			tmp2 = bitrng(instr, 3, 0); // Rm
			tmp2 = rpi2_reg_context.storage[tmp2];
			tmp2 &= tmp4;
			// insert bits
			tmp1 |= tmp2;
			retval = set_arm_addr(tmp1);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_rbit:
		// reverse Rm bit order
		// if d == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp1 = rpi2_reg_context.storage[tmp1];
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

			retval = set_arm_addr(tmp1);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_rev:
		// reverse Rm byte order
		// if d == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp1 = rpi2_reg_context.storage[tmp1];
			tmp2 = rpi2_reg_context.storage[bitrng(instr, 3, 0)]; // (Rm)
			tmp1 = (tmp2 & 0xff000000)  >> 24; // result lowest
			tmp1 |= (tmp2 & 0x00ff0000) >> 8;
			tmp1 |= (tmp2 & 0x0000ff00) << 8;
			tmp1 |= (tmp2 & 0x000000ff) << 24; // result highest
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;

	case arm_cmisc_rev16:
		// reverse Rm half word byte orders
		// if d == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp1 = rpi2_reg_context.storage[tmp1];
			tmp2 = rpi2_reg_context.storage[bitrng(instr, 3, 0)]; // (Rm)
			tmp1 = (tmp2 & 0xff000000)  >> 8;
			tmp1 |= (tmp2 & 0x00ff0000) << 8; // result highest
			tmp1 |= (tmp2 & 0x0000ff00) >> 8; // result lowest
			tmp1 |= (tmp2 & 0x000000ff) << 8;
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_revsh:
		// Byte-Reverse Signed Halfword and sign-extend
		// if d == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp2 = rpi2_reg_context.storage[bitrng(instr, 3, 0)]; // (Rm)
			if (bit(tmp2, 7)) // the result will be negative
				tmp3 = (~0) << 16;
			else
				tmp3 = 0;
			tmp3 |= (tmp2 & 0x0000ff00) >> 8;
			tmp3 |= (tmp2 & 0x000000ff) << 8;

			retval = set_arm_addr(tmp3);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_sbfx:
	case arm_cmisc_ubfx:
		// Signed Bit Field Extract
		// if d == 15 || n == 15 then UNPREDICTABLE
		// width minus 1 = bits 20-16, lsb = bits 11-7
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp2 = bitrng(instr, 20, 16); // widhtminus1
			tmp3 = bitrng(instr, 11, 7); // lsb
			// make the 'pick' mask
			tmp4 = (~0) << (tmp2 + 1);
			tmp4 = (~tmp4) << tmp3;
			// get bits from Rm
			tmp1 = bitrng(instr, 3, 0); // Rn
			tmp1 = rpi2_reg_context.storage[tmp1];
			tmp1 &= tmp4;
			// make it an unsigned number
			tmp1 >>= tmp3;
			if (extra == arm_cmisc_sbfx)
			{
				// if msb is '1'
				if (bit(tmp1, tmp2))
				{
					// sign-extend to signed
					tmp1 |= (~0)<<(tmp2 + 1);
				}
			}
			retval = set_arm_addr(tmp1);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_sel:
		// select bytes by GE-flags
		// if d == 15 || n == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp1 = rpi2_reg_context.storage[bitrng(instr, 19, 16)]; // (Rn)
			tmp2 = rpi2_reg_context.storage[bitrng(instr, 3, 0)]; // (Rm)
			tmp3 = rpi2_reg_context.reg.cpsr;
			tmp4 = 0;
			// select Rn or Rm by the GE-bits
			tmp4 |= ((bit(tmp3,19)?tmp1:tmp2) & 0xff000000);
			tmp4 |= ((bit(tmp3,18)?tmp1:tmp2) & 0x00ff0000);
			tmp4 |= ((bit(tmp3,17)?tmp1:tmp2) & 0x0000ff00);
			tmp4 |= ((bit(tmp3,16)?tmp1:tmp2) & 0x000000ff);
			retval = set_arm_addr(tmp4);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	case arm_cmisc_usad8:
	case arm_cmisc_usada8:
		// calculate sum of absolute byte differences
		// if d == 15 || n == 15 || m == 15 then UNPREDICTABLE
		tmp1 = bitrng(instr, 15, 12); // Rd
		if (tmp1 == 15) // Rd = PC
		{
			tmp1 = rpi2_reg_context.storage[bitrng(instr, 11, 8)]; // (Rm)
			tmp2 = rpi2_reg_context.storage[bitrng(instr, 3, 0)]; // (Rn)
			// sum of absolute differences
			tmp3 = 0;
			for (tmp4 = 0; tmp4 < 4; tmp4++) // for each byte
			{
				htmp1 = (unsigned short)((tmp1 >> (8*tmp4)) & 0xff);
				htmp2 = (unsigned short)((tmp2 >> (8*tmp4)) & 0xff);
				if (htmp2 < htmp1) htmp1 -= htmp2;
				else htmp1 = htmp2 - htmp1;
				tmp3 += (unsigned int)htmp1;
			}

			if (extra == arm_cmisc_usada8)
			{
				// USADA8 (=USAD8 with Ra)
				tmp1 = rpi2_reg_context.storage[bitrng(instr, 15, 12)]; // (Ra)
				tmp3 += tmp1;
			}
			retval = set_arm_addr(tmp3);
			retval = set_unpred_addr(retval);
		}
		else
		{
			retval = set_addr_lin();
		}
		break;
	default:
		break;
	}
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
