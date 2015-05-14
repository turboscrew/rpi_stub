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
// mostly by scripts. The outcome is in the file 'ARM_decode_table_data.h'
// The extra-field and decoder-function names are also defined in
// the spreadsheet.
//
// At this stage the function names and extra-field enum-values are
// just copied from the spreadsheet. The function names and the
// extra-values must match to those in the spreadsheet.

#include "ARM_decode_table.h"

// Extra info to help in decoding.
// Especially useful for decoding multoplexed instructions
// (= different instructions that have same mask and data).
// These could have been read from a file generated from the
// spreadsheet, but since there are not very many of them, they
// are just copied here from the spreadsheet.
typedef enum
{
	arm_xtra_none,
	arm_xtra_all_lanes,
	arm_xtra_banked,
	arm_xtra_cmode,
	arm_xtra_excl,
	arm_xtra_fp,
	arm_xtra_hint,
	arm_xtra_imm,
	arm_xtra_imm_cmode,
	arm_xtra_immzero,
	arm_xtra_mode,
	arm_xtra_movw,
	arm_xtra_one_lane,
	arm_xtra_op,
	arm_xtra_priv,
	arm_xtra_priv_hint,
	arm_xtra_ret,
	arm_xtra_same_nm,
	arm_xtra_single,
	arm_xtra_spec,
	arm_xtra_type,
	arm_xtra_unpriv,
	arm_xtra_vldm,
	arm_xtra_vstm
} ARM_decode_extra_t;

// Decoding function prototypes.
// These could have been read from a file generated from the
// spreadsheet, but since there are not very many of them, they
// are just copied here from the spreadsheet.
unsigned int arm_branch(unsigned int instr, ARM_decode_extra_t);
unsigned int arm_coproc(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_div(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_mac(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_misc(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_pack(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_par(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_sat(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_satx(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_shift(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_data_std(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_exc(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_ldst(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_ldstm(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_misc(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_core_status(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_fp(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_mux(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_v_bits(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_v_comp(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_v_mac(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_v_misc(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_v_par(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_v_shift(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_vfp_ldst_elem(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_vfp_ldst_ext(unsigned int instr, ARM_decode_extra_t extra);
unsigned int arm_vfp_xfer_reg(unsigned int instr, ARM_decode_extra_t extra);

// the decoding table type
typedef struct
{
	unsigned int data;
	unsigned int mask;
	ARM_decode_extra_t extra;
	unsigned int (*decoder)(unsigned int, ARM_decode_extra_t);
} ARM_dec_tbl_entry_t;

// the decoding table itself.
// (See the description in the beginningof this file.)
ARM_dec_tbl_entry_t ARM_decode_table[] =
{
#include "ARM_decode_table_data.h"
};

// decoder dispatcher - finds the decoding function and calls it
// TODO: Partition the table seatch (use bits 27 - 25 of the instruction.
int ARM_decoder_dispatch(unsigned int instr)
{
	int instr_count = sizeof(ARM_decode_table);
	unsigned int retval = 0xffffffff;
	int i;

	for (i=0; i<instr_count; i++)
	{
		if ((instr & ARM_decode_table[i].mask) == ARM_decode_table[i].data)
		{
			retval = ARM_decode_table[i].decoder(instr, ARM_decode_table[i].extra);
			break;
		}
	}
	if (retval == 0xffffffff) // instruction not known
	{
		// return UNDEFINED
	}
	return retval;
}

// the decoding functions

// Sub-dispatcher - figures out the multiplexed instruction encodings
// and dispatches further,
unsigned int arm_mux(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_branch(unsigned int instr, ARM_decode_extra_t)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_coproc(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_div(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_mac(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_pack(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_par(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_sat(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_satx(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_shift(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_data_std(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_exc(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_ldst(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_ldstm(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_core_status(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_fp(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_v_bits(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_v_comp(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_v_mac(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_v_misc(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_v_par(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_v_shift(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_vfp_ldst_elem(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_vfp_ldst_ext(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}

unsigned int arm_vfp_xfer_reg(unsigned int instr, ARM_decode_extra_t extra)
{
	unsigned int retval = 0xffffffff;
	return retval;
}
