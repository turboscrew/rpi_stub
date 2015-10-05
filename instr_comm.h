/*
instr_comm.h

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

#ifndef INSTR_COMM_H_
#define INSTR_COMM_H_

#define INSTR_ADDR_UNDEF 0
// marker: instruction is UNPREDICTABLE
#define INSTR_ADDR_UNPRED (1 << 4)
// marker: ARM instruction is defined
#define INSTR_ADDR_ARM 2
// marker: THUMB instruction is defined
#define INSTR_ADDR_THUMB 3
// marker: instruction is UNDEF

// processor modes
#define INSTR_PMODE_USR 0x10
#define INSTR_PMODE_FIQ 0x11
#define INSTR_PMODE_IRQ 0x12
#define INSTR_PMODE_SVC 0x13
#define INSTR_PMODE_MON 0x16
#define INSTR_PMODE_ABT 0x17
#define INSTR_PMODE_HYP 0x1a
#define INSTR_PMODE_UND 0x1b
#define INSTR_PMODE_SYS 0x1f

// PSR flags (N Z C V)
#define INSTR_FLAG_N 0x8
#define INSTR_FLAG_Z 0x4
#define INSTR_FLAG_C 0x2
#define INSTR_FLAG_V 0x1

// Some instruction condition field masks
#define INSTR_COND_MASK 0xf0000000
#define INSTR_COND_AL 0xe0000000
#define INSTR_COND_NV 0xf0000000

// structure to hold the result of decoding
typedef struct
{
	unsigned int address;
	int flag;
} instr_next_addr_t;

// execute instruction with edited registers
void exec_instr(unsigned int instr, unsigned int r0, unsigned int r1,
		unsigned int r2, unsigned int r3);

// reads banked register of another mode
// mode is processor mode (INSTR_PMODE_XXX) reg is register number (16 = spsr)
unsigned int get_mode_reg(unsigned int mode, unsigned int reg);

// return the value of 'pos'th bit (31 - 0)
unsigned int bit(unsigned int value, int pos);

// return the value of bits in the range
unsigned int bitrng(unsigned int value, int hi, int lo);

// return the value formed by bits selected by the mask
unsigned int bits(unsigned int value, unsigned int mask);

// return UNDEFINED address
instr_next_addr_t set_undef_addr(void);

// mark address as UNPREDICTABLE address
instr_next_addr_t set_unpred_addr(instr_next_addr_t addr);

// return address as valid ARM-mode address
instr_next_addr_t set_arm_addr(unsigned int addr);

// return address as valid Thumb-mode address
instr_next_addr_t set_thumb_addr(unsigned int addr);

// returns sign-extended 32-bit value of bits in the range
int sx32(unsigned int val, int hi, int lo);

// returns zero if instruction condition doesn't match the
// stored CPSR condition flags (makes instruction a NOP)
int will_branch(unsigned int instr);

// get banked register contents
unsigned int get_usr_reg(unsigned int reg);

// find out which extensions are implemented
unsigned int read_ID_PFR1();

// get processor mode
unsigned int get_proc_mode();

// checks if processor mode was any of the suggested
// needless arguments can be set to zero
unsigned int check_proc_mode(unsigned int mode1, unsigned int mode2,
		unsigned int mode3, unsigned int mode4);

// get hypervisor mode lr
unsigned int get_elr_hyp();

// get contents of Hyp Configuration Register
unsigned int get_HCR();

// get contents of Secure Configuration Register
unsigned int get_SCR();

// get contents of Secure Configuration Register
void set_SCR(unsigned int m);

// get contents of Non-Secure Access Control Register
unsigned int get_NSACR();

// returns 1 if secure state, 0 otherwise
unsigned int get_security_state();

// returns 1 if coprocessor is accessible in non-secure state
// if coproc == 16 -> access to FIQ banked regs in non-safe mode
// if coproc = 11 or 10 return 4-bit status
// (cp 11 and cp10 are handled together)
unsigned int check_coproc_access(unsigned int coproc);

#endif /* INSTR_COMM_H_ */
