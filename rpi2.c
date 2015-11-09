/*
rpi2.c

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

#include <stdint.h>
#include "util.h"

extern void serial_irq(); // this shouldn't be public, so it's not in serial.h
extern int serial_raw_puts(char *str); // used for debugging
// extern void serial_enable_ctrlc(); // used for debugging
extern volatile uint32_t gdb_dyn_debug;
extern char __hivec;
// for logging via 'O'-packets
extern void gdb_send_text_packet(char *msg, unsigned int msglen);
// Another naughty trick - allocates
// exception_info, exception_extra and rpi2_reg_context
// This way definitions and extern-declarations don't clash
#define extern
#include "rpi2.h"
#undef extern

#define DEBUG_REG_CONTEXT

#define LOW_VECTOR_ADDRESS_RST 0
#define LOW_VECTOR_ADDRESS_UND 4
#define LOW_VECTOR_ADDRESS_SVC 8
#define LOW_VECTOR_ADDRESS_PABT 12
#define LOW_VECTOR_ADDRESS_DABT 16
#define LOW_VECTOR_ADDRESS_AUX 20
#define LOW_VECTOR_ADDRESS_IRQ 24
#define LOW_VECTOR_ADDRESS_FIQ 28

#define TRAP_INSTR_T 0xbe00
#define TRAP_INSTR_A 0xe1200070

// for mailbox
#define MBOX_BUFF_SIZE (1024 + 16)
volatile __attribute__ ((aligned (16))) uint8_t rpi2_mbox_buff[MBOX_BUFF_SIZE];

// MMU-stuff

/* this is for reference only - actually using bit fields would be dangerous here
typedef struct
{
	uint32_t base_address:12;
	uint32_t ns:1;
	uint32_t sect_type:1;
	uint32_t ng:1;
	uint32_t s:1;
	uint32_t apx:1;
	uint32_t tex:3;
	uint32_t ap:2;
	uint32_t p:1;
	uint32_t domain:4;
	uint32_t xn:1;
	uint32_t c:1;
	uint32_t b:1;
	uint32_t entry_type:1;
	uint32_t xn:1;
} xlat1_entry_sect;
*/

// for now
#define MMU_SECT_ATTR_NORMAL 0x00090c0a
#define MMU_SECT_ATTR_DEV 0x00090c06

// TTBCR_N:
// if N = 0, TTBR0 maps all addresses and the table size is 16 kB
// the table (16 kB) can be addressed with 14 bits (0 - 0x3fff)
// then the rest (upper part) of the address is put into the TTBR0
// as the upper part of the table entry address, and the table needs to be
// 16 kB aligned. If N=1, TTBR0 maps half of the addresses, table size is
// 8 kB, table offset is 13 bits and the rest is put in TTBR0. The table
// has to be 8 kB aligned, and so on.
// one entry maps 1 MB block, so mapping 4GB memory takes 4096 (4k) entries
// 4 bytes each -> 16 kB
// these definitions are made for possibly splitting the mappings between
// TTBR0 and TTBR1 in the future
#define TTBCR_N 0
#define TBL_OFFSET_BITS (14 - TTBCR_N)
#define TBL_ALIGNMENT (1 << TBL_OFFSET_BITS)
#define TBL_WORD_SIZE (TBL_ALIGNMENT >>2 )
#define MMU_VAL_TTBR0(addr, attr) ((addr & ((~0)<< TBL_OFFSET_BITS)) | attr)
#define MMU_SECT_ENTRY(addr_meg, attr) ((addr_meg << 20) | attr)
volatile __attribute__ ((aligned (TBL_ALIGNMENT))) uint32_t master_xlat_tbl[TBL_WORD_SIZE];
extern void gdb_trap_handler();
extern uint32_t jumptbl;
uint32_t dbg_reg_base;
uint32_t dbg_rom_base;

volatile uint32_t rpi2_dgb_enabled; // flag to pabt that this is IRQ-case
volatile uint32_t rpi2_debuggee_running;
volatile uint32_t rpi2_ctrl_c_flag;
volatile uint32_t rpi2_irq_status;
volatile uint32_t rpi2_sigint_flag; // flag: this is SIGINT-case
volatile uint32_t rpi2_exc_reason;
uint32_t rpi2_upper_vec_address;
volatile uint32_t rpi2_pabt_reroute;

// exception handling stuff
// at the moment, SVC-stuff are used for all sychronous exceptions:
// reset, und, svc, aux
volatile rpi2_reg_context_t rpi2_irq_context;
volatile rpi2_reg_context_t rpi2_fiq_context;
volatile rpi2_reg_context_t rpi2_dabt_context;
volatile rpi2_reg_context_t rpi2_pabt_context;
volatile rpi2_reg_context_t rpi2_svc_context;

// for debugging
#define DBG_TMP_SZ 16
#define DBG_MSG_SZ 256
char dbg_tmp_buff[DBG_TMP_SZ];
char dbg_msg_buff[DBG_MSG_SZ];

extern char __fiq_stack;
extern char __usrsys_stack;
extern char __svc_stack;
extern char __irq_stack;
extern char __mon_stack;
extern char __hyp_stack;
extern char __und_stack;
extern char __abrt_stack;

// exception handlers
void rpi2_reset_handler() __attribute__ ((naked));
void rpi2_undef_handler() __attribute__ ((naked));
void rpi2_svc_handler() __attribute__ ((naked));
void rpi2_aux_handler() __attribute__ ((naked));
void rpi2_unhandled_dabt() __attribute__ ((naked));
void rpi2_irq_handler() __attribute__ ((naked));
void rpi2_fiq_handler() __attribute__ ((naked));
void rpi2_pabt_handler() __attribute__ ((naked));

// exception re-routing
void rpi2_unhandled_irq() __attribute__ ((naked));
void rpi2_unhandled_fiq() __attribute__ ((naked));
void rpi2_unhandled_pabt() __attribute__ ((naked));
void rpi2_reroute_reset() __attribute__ ((naked));
void rpi2_reroute_exc_und() __attribute__ ((naked));
void rpi2_reroute_exc_svc() __attribute__ ((naked));
void rpi2_reroute_exc_dabt() __attribute__ ((naked));
void rpi2_reroute_exc_aux() __attribute__ ((naked));
void rpi2_reroute_exc_irq() __attribute__ ((naked));
void rpi2_reroute_exc_fiq() __attribute__ ((naked));

/* delay() borrowed from OSDev.org */
static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : : [count]"r"(count) : "cc");
}

void set_gdb_enabled(unsigned int state)
{
	rpi2_dgb_enabled = state;
	rpi2_flush_address((unsigned int) &rpi2_dgb_enabled);
}

unsigned int rpi2_disable_save_ints()
{
	uint32_t status;
	asm volatile (
			"mrs %[var_reg], cpsr\n\t"
			"dsb\n\t"
			"isb\n\t"
			"cpsid aif\n\t"
			"dsb\n\t"
			"isb\n\t"
			:[var_reg] "=r" (status)::
	);
	return status;
}

void rpi2_restore_ints(unsigned int status)
{
	asm volatile (
			"msr cpsr_fsxc, %[var_reg]\n\t"
			"dsb\n\t"
			"isb\n\t"
			::[var_reg] "r" (status):
	);
}

void rpi2_disable_excs()
{
	asm volatile (
			"cpsid aif\n\t"
			"dsb\n\t"
			"isb\n\t"
	);

}

void rpi2_enable_excs()
{
	asm volatile (
			"cpsie aif\n\t"
			"dsb\n\t"
			"isb\n\t"
	);
}

void rpi2_disable_ints()
{
	asm volatile (
			"cpsid i\n\t"
			"dsb\n\t"
			"isb\n\t"
	);

}

void rpi2_enable_ints()
{
	asm volatile (
			"cpsie i\n\t"
			"dsb\n\t"
			"isb\n\t"
	);
}

/*
 * Exceptions are handled as follows:
 * First exception handler stores the processor context and fixes the return address.
 * PABT exception is checked for BKPT or real PABT. In case of BKPT, the
 * debugger is entered normally. Otherwise the debugger is entered as unhandled exception.
 * IRQ exception is checked whether it's a serial interrupt. If it is,
 * it's handled like a normal interrupt, otherwise the mode is set to the
 * debugger's and a jump to debugger is made.
 * Other exceptions are handled like non-serial interrupt.
 * On return, the exception handler loads the processor context and returns from
 * the exception. No offset is added to the return address, so the return address
 * needs to be fixed at the entry.
 *
 * TODO: maybe the immediate part of BKPT-instruction can be easily used for
 * telling apart different ways to enter the debugger: ARM/Thumb, direct with
 * context save or unhandled interrupt with context already saved?
 * (exception -> handler, context save -> bkpt -> debugger)
 *
 * BKPT is a bit dangerous, because the debugger LR may be destroyed by PABT.
 * (PABT is asynchronous exception to the same mode as BKPT - ABT)
 * For now, we keep PABT disabled.
 */

/* A fixed vector table - copied to 0x0 */

void rpi2_set_lower_vectable()
{
	asm volatile (
			"@.globl jumptbl\n\t"
			"push {r0, r1, r2, r3}\n\t"
			"b irqset_low @ Skip the vector table to be copied\n\t"
			"jumpinstr_low:\n\t"
			"ldr pc, jumptbl_low    @ reset\n\t"
			"ldr pc, jumptbl_low+4  @ undef\n\t"
			"ldr pc, jumptbl_low+8  @ SVC\n\t"
			"ldr pc, jumptbl_low+12 @ prefetch\n\t"
			"ldr pc, jumptbl_low+16 @ data\n\t"
			"ldr pc, jumptbl_low+20 @ aux\n\t"
			"ldr pc, jumptbl_low+24 @ IRQ\n\t"
			"ldr pc, jumptbl_low+28 @ FIRQ\n\t"
			"jumptbl_low: @ 8 addresses\n\t"
	);
	asm volatile (
			".word rpi2_reset_handler\n\t"
			".word rpi2_undef_handler\n\t"
			".word rpi2_svc_handler\n\t"
			".word rpi2_unhandled_pabt\n\t"
			".word rpi2_unhandled_dabt\n\t"
			".word rpi2_aux_handler @ used in some cases\n\t "
			".word rpi2_unhandled_irq\n\t"
			".word rpi2_unhandled_fiq\n\t"
	);
	asm volatile (
			"irqset_low:\n\t"
			"@ Copy the vectors\n\t"
			"ldr r0, =jumpinstr_low\n\t"
			"mov r1, #0\n\t"
			"ldr r2, =irqset_low\n\t"
			"1:\n\t"
			"ldr r3, [r0], #4 @ word (auto-increment)\n\t"
			"str r3, [r1], #4\n\t"
			"cmp r2, r0 @ r2 - r0\n\t"
			"bhi 1b\n\t"
			"b 2f\n\t"
			".ltorg @ literal pool\n\t"
			"2:\n\t"
			"pop {r0, r1, r2, r3}\n\t"
			"dsb\n\t"
			"isb\n\t"
	);
}

void rpi2_set_upper_vectable()
{
	// TODO: add:
	// if rpi2_uart0_excmode == RPI2_UART0_FIQ
	// rpi2_irq_handler => rpi2_reroute_exc_irq
	// rpi2_reroute_exc_fiq => rpi2_fiq_handler
	asm volatile (
			"@.globl jumptbl\n\t"
			"push {r0, r1, r2, r3}\n\t"
			"b irqset_high @ Skip the vector table to be copied\n\t"
			"jumpinstr_high:\n\t"
			"ldr pc, jumptbl_high    @ reset\n\t"
			"ldr pc, jumptbl_high+4  @ undef\n\t"
			"ldr pc, jumptbl_high+8  @ SVC\n\t"
			"ldr pc, jumptbl_high+12 @ prefetch\n\t"
			"ldr pc, jumptbl_high+16 @ data\n\t"
			"ldr pc, jumptbl_high+20 @ aux\n\t"
			"ldr pc, jumptbl_high+24 @ IRQ\n\t"
			"ldr pc, jumptbl_high+28 @ FIRQ\n\t"
			"jumptbl_high: @ 8 addresses\n\t"
			".word rpi2_reroute_reset\n\t"
			".word rpi2_reroute_exc_und\n\t"
			".word rpi2_reroute_exc_svc\n\t"
			".word rpi2_pabt_handler\n\t"
			".word rpi2_dabt_handler\n\t"
			".word rpi2_reroute_exc_aux @ in some cases\n\t "
			".word rpi2_irq_handler\n\t"
			".word rpi2_reroute_exc_fiq\n\t"
			"irqset_high:\n\t"
			"@ Copy the vectors\n\t"
			"ldr r0, =jumpinstr_high\n\t"
			"ldr r1, =rpi2_upper_vec_address\n\t"
			"ldr r1, [r1]\n\t"
			"ldr r2, =irqset_high\n\t"
			"1:\n\t"
			"ldr r3, [r0], #4 @ word (auto-increment)\n\t"
			"str r3, [r1], #4\n\t"
			"cmp r2, r0 @ r2 - r0\n\t"
			"bhi 1b\n\t"
			"b 2f\n\t"
			".ltorg @ literal pool\n\t"
			"2:\n\t"
			"pop {r0, r1, r2, r3}\n\t"
			"dsb\n\t"
			"isb\n\t"
	);
}

void rpi2_dump_context(volatile rpi2_reg_context_t *context)
{
	char *pp;
	uint32_t i, tmp, tmp2;
	static char scratchpad[16];
	uint32_t mode, sp;
	uint32_t stack_base, stack_size;

	mode = (uint32_t)context->reg.cpsr;
	sp = (uint32_t)context->reg.r13;
	switch (mode & 0xf)
	{
	case 0:
		serial_raw_puts("\r\nUSR MODE ");
		pp = &__usrsys_stack;
		break;
	case 1:
		serial_raw_puts("\r\nFIQ MODE ");
		pp = &__fiq_stack;
		break;
	case 2:
		serial_raw_puts("\r\nIRQ MODE ");
		pp = &__irq_stack;
		break;
	case 3:
		serial_raw_puts("\r\nSVC MODE ");
		pp = &__svc_stack;
		break;
	case 6:
		serial_raw_puts("\r\nMON MODE ");
		pp = &__mon_stack;
		break;
	case 7:
		serial_raw_puts("\r\nABT MODE ");
		pp = &__abrt_stack;
		break;
	case 10:
		serial_raw_puts("\r\nHYP MODE ");
		pp = &__hyp_stack;
		break;
	case 11:
		serial_raw_puts("\r\nUND MODE ");
		pp = &__und_stack;
		break;
	case 15:
		serial_raw_puts("\r\nSYS MODE ");
		pp = &__usrsys_stack;
		break;
	default:
		serial_raw_puts("\r\nBAD MODE ");
		util_word_to_hex(scratchpad, mode & 0x1f);
		serial_raw_puts(scratchpad);
		pp = 0;
		break;
	}
	stack_base = (uint32_t)pp;

	pp = "\r\nREGISTER DUMP\r\n";
	do {i = serial_raw_puts(pp); pp += i;} while (i);
	serial_raw_puts("\r\nregister : ");
	serial_raw_puts("data\r\n");
	for (i=0; i<18; i++)
	{
		util_word_to_hex(scratchpad, i);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" : ");
		util_word_to_hex(scratchpad, context->storage[i]);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
	}

	pp = "\r\nSTACK DUMP\r\n";
	do {i = serial_raw_puts(pp); pp += i;} while (i);

	if (stack_base == 0)
	{
		serial_raw_puts("SP base = ");
		serial_raw_puts("unknown");
	}
	else
	{
		// dump stack
		serial_raw_puts("SP base = ");
		util_word_to_hex(scratchpad, stack_base);
		serial_raw_puts(scratchpad);
	}

	serial_raw_puts(" SP = ");
	util_word_to_hex(scratchpad, sp);
	serial_raw_puts(scratchpad);

	if (stack_base == 0)
	{
		serial_raw_puts("\r\nstack size: ");
		serial_raw_puts("unknown");
		tmp2 = 16;
		stack_size = 0;
	}
	else
	{
		stack_size = (stack_base - sp);
		serial_raw_puts("\r\nstack size: ");
		util_word_to_hex(scratchpad, stack_size);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" bytes");
		tmp2 = stack_size/4; // stack size in words
		// limit dump to 16 words
		if (tmp2 > 16)
		{
			tmp2 = 16;
		}
	}
	if (sp != (sp & ~3))
	{
		sp &= (~3); // ensure alignment
		serial_raw_puts("\r\nSP aligned ");
		serial_raw_puts("to: ");
		util_word_to_hex(scratchpad, sp);
		serial_raw_puts(scratchpad);
		tmp2++; // one more word to include the 'partial word'
	}

	serial_raw_puts("\r\naddress  : ");
	serial_raw_puts("data\r\n");
	for (i=0; i<tmp2; i++)
	{
		tmp = *((uint32_t *)sp);
		util_word_to_hex(scratchpad, sp);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" : ");
		util_word_to_hex(scratchpad, tmp);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
		sp += 4;
	}
}

// save processor context
static inline void write_context()
{
	// NOTE: mrs (banked regs) is unpredictable if accessing
	// regs of our own mode
	asm volatile(
			"@push {r12}\n\t"
			"@ldr r12, =rpi2_reg_context\n\t"
			"stmia r12, {r0 - r11}\n\t"
			"mov r0, r12\n\t"
			"pop {r12}\n\t"
			"str r12, [r0, #4*12]\n\t"

			"mrs r1, spsr\n\t"
			"dsb\n\t"
			"str r1, [r0, #4*16]\n\t"
			"str lr, [r0, #4*15]\n\t"

			"@ mode specifics\n\t"
			"mov r2, #0xf @ all valid modes have bit 4 set\n\t"
			"and r1, r2\n\t"

			"@ our own mode?\n\t"
			"mrs r3, cpsr\n\t"
			"dsb\n\t"
			"and r3, r2\n\t"
			"cmp r1, r3\n\t"
			"bne 10f @ not our own mode\n\t"

			"mov r1, sp\n\t"
			"mov r2, lr\n\t"
			"mrs r3, spsr\n\t"
			"dsb\n\t"
			"b 9f\n\t"

			"10:\n\t"
			"cmp r3, #0x01 @ our mode is FIQ?\n\t"
			"bne 11f\n\t"

			"mrs r8, r8_usr\n\t"
			"mrs r9, r9_usr\n\t"
			"mrs r10, r10_usr\n\t"
			"mrs r11, r11_usr\n\t"
			"mrs r12, r12_usr\n\t"
			"add r7, r0, #4*8\n\t"
			"stmia r7, {r8 - r12} @ overwrite r8 - 12 with banked regs\n\t"

			"11:\n\t"
			"cmp r1, #0x00 @ USR\n\t"
			"cmpne r1, #0x0f @ SYS\n\t"
			"bne 1f\n\t"
			"@ USR/SYS\n\t"
			"mrs r1, SP_usr\n\t"
			"mrs r2, LR_usr\n\t"
			"dsb\n\t"
			"eor r3, r3 @ usr/sys has no spsr\n\t"
			"b 9f\n\t"
			"1: cmp r1,  #0x01 @ FIQ\n\t"
			"bne 2f\n\t"
			"@ FIQ\n\t"
			"mrs r2, r8_fiq\n\t"
			"mrs r3, r9_fiq\n\t"
			"mrs r4, r10_fiq\n\t"
			"mrs r5, r11_fiq\n\t"
			"mrs r6, r12_fiq\n\t"
			"dsb\n\t"
			"add r1, r0, #4*8\n\t"
			"stmia r1, {r2 - r6} @ overwrite r8 - 12 with banked regs\n\t"
			"mrs r1, SP_fiq\n\t"
			"mrs r2, LR_fiq\n\t"
			"mrs r3, SPSR_fiq\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"2: cmp r1, #0x02 @ IRQ\n\t"
			"bne 3f\n\t"
			"@ IRQ\n\t"
			"mrs r1, SP_irq\n\t"
			"mrs r2, LR_irq\n\t"
			"mrs r3, SPSR_irq\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"3: cmp r1, #0x03 @ SVC\n\t"
			"bne 4f\n\t"
			"@ SVC\n\t"
			"mrs r1, SP_svc\n\t"
			"mrs r2, LR_svc\n\t"
			"mrs r3, SPSR_svc\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"4: cmp r1, #0x06 @ MON\n\t"
			"bne 5f\n\t"
			"@ MON\n\t"
			"mrs r1, SP_mon\n\t"
			"mrs r2, LR_mon\n\t"
			"mrs r3, SPSR_mon\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"5: cmp r1, #0x07 @ ABT\n\t"
			"bne 6f\n\t"
			"@ ABT\n\t"
			"mrs r1, SP_abt\n\t"
			"mrs r2, LR_abt\n\t"
			"mrs r3, SPSR_abt\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"6: cmp r1, #0x0a @ HYP\n\t"
			"bne 7f\n\t"
			"@ HYP\n\t"
			"mrs r1, SP_hyp\n\t"
			"mrs r2, ELR_hyp\n\t"
			"mrs r3, SPSR_hyp\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"7: cmp r1, #0x0b @ UND\n\t"
			"bne 8f\n\t"
			"@ UND\n\t"
			"mrs r1, SP_und\n\t"
			"mrs r2, LR_und\n\t"
			"mrs r3, SPSR_und\n\t"
			"dsb\n\t"
			"b 9f\n\t"
			"8: @ ILLEGAL MODE\n\t"
			"udf\n\t"
			"9: \n\t"

			"dsb\n\t"
			"str r1, [r0, #4*13]\n\t"
			"str r2, [r0, #4*14]\n\t"
			"str r3, [r0, #4*17]\n\t"
	);
}

// restore processor context
static inline void read_context()
{
	// NOTE: msr (banked regs) is unpredictable if accessing
	// regs of our own mode
	asm volatile(
			"@ldr r0, =rpi2_reg_context\n\t"
			"@ mode specifics\n\t"

			"ldr r1, [r0, #4*16] @ cpsr\n\t"
			"msr spsr_fsxc, r1\n\t"
			"dsb\n\t"
			"isb\n\t"
			"@ mode to return to\n\t"
			"mov r2, #0xf @ all valid modes have bit 4 set\n\t"
			"and r1, r2\n\t"

			"@ our own mode\n\t"
			"mrs r3, cpsr\n\t"
			"dsb\n\t"
			"and r3, r2\n\t"
			"cmp r1, r3\n\t"
			"bne 10f @ not same mode as ours\n\t"
			"ldr sp, [r0, #4*13] @ sp\n\t"
			"ldr lr, [r0, #4*14] @ lr\n\t"
			"b 9f\n\t"

			"10:\n\t"
			"ldr r2, [r0, #4*13] @ sp\n\t"
			"ldr r3, [r0, #4*14] @ lr\n\t"
			"ldr r4, [r0, #4*17] @ spsr\n\t"
			"cmp r1, #0x00 @ USR\n\t"
			"cmpne r1, #0x0f @ SYS\n\t"
			"bne 1f\n\t"
			"@ USR/SYS\n\t"
			"msr SP_usr, r2\n\t"
			"msr LR_usr, r3\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"1: cmp r1,  #0x01 @ FIQ\n\t"
			"bne 2f\n\t"
			"@ FIQ\n\t"
			"msr SP_fiq, r2\n\t"
			"msr LR_fiq, r3\n\t"
			"msr SPSR_fiq, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"add r1, r0, #4*8\n\t"
			"ldmia r1, {r2 - r6} @ banked regs\n\t"
			"msr r8_fiq, r2\n\t"
			"msr r9_fiq, r3\n\t"
			"msr r10_fiq, r4\n\t"
			"msr r11_fiq, r5\n\t"
			"msr r12_fiq, r6\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"2: cmp r1, #0x02 @ IRQ\n\t"
			"bne 3f\n\t"
			"@ IRQ\n\t"
			"msr SP_irq, r2\n\t"
			"msr LR_irq, r3\n\t"
			"msr SPSR_irq, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"3: cmp r1, #0x03 @ SVC\n\t"
			"bne 4f\n\t"
			"@ SVC\n\t"
			"msr SP_svc, r2\n\t"
			"msr LR_svc, r3\n\t"
			"msr SPSR_svc, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"4: cmp r1, #0x06 @ MON\n\t"
			"bne 5f\n\t"
			"@ MON\n\t"
			"msr SP_mon, r2\n\t"
			"msr LR_mon, r3\n\t"
			"msr SPSR_mon, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"5: cmp r1, #0x07 @ ABT\n\t"
			"bne 6f\n\t"
			"@ ABT\n\t"
			"msr SP_abt, r2\n\t"
			"msr LR_abt, r3\n\t"
			"msr SPSR_abt, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"6: cmp r1, #0x0a @ HYP\n\t"
			"bne 7f\n\t"
			"@ HYP\n\t"
			"msr SP_hyp, r2\n\t"
			"msr ELR_hyp, r3\n\t"
			"msr SPSR_hyp, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"7: cmp r1, #0x0b @ UND\n\t"
			"bne 8f\n\t"
			"@ UND\n\t"
			"msr SP_und, r2\n\t"
			"msr LR_und, r3\n\t"
			"msr SPSR_und, r4\n\t"
			"dsb\n\t"
			"isb\n\t"
			"b 9f\n\t"
			"8: @ ILLEGAL MODE\n\t"
			"udf\n\t"
			"9: \n\t"

			"dsb\n\t"
			"isb\n\t"
			"ldr r2, [r0, #4*15] @ return address\n\t"
			"ldr r1, [r0] @ r0-value\n\t"
			"push {r1, r2} @ prepare for return\n\t"
			"ldmia r0, {r0 - r12}\n\t"
			"pop {r0, lr}\n\t"
	);
}

#ifdef RPI2_DEBUG_TIMER

void rpi2_timer_init()
{
	*((volatile uint32_t *)ARM_TIMER_CTL) = 0x003E0000; // stop
	*((volatile uint32_t *)ARM_TIMER_LOAD) = 1000000-1; // load-value
	*((volatile uint32_t *)ARM_TIMER_RELOAD) = 1000000-1; // reload-value
	*((volatile uint32_t *)ARM_TIMER_DIV) = 0x000000F9; // pre-divisor
	*((volatile uint32_t *)ARM_TIMER_CLI) = 0; // clear interrupts
	*((volatile uint32_t *)IRC_ENB) = 1; // enable at interrupt controller
}

void rpi2_timer_start()
{
	*((volatile uint32_t *)ARM_TIMER_LOAD) = 1000000-1; // load-value
	*((volatile uint32_t *)ARM_TIMER_CLI) = 0; // clear interrupts
	// start timer, enable int, prescaler = 256, 23-bit counter
	*((volatile uint32_t *)ARM_TIMER_CTL) = 0x003E00AA;
}

void rpi2_timer_stop()
{
	*((volatile uint32_t *)ARM_TIMER_CTL) = 0x003E0000; // stop
}

void rpi2_timer_kick()
{
	*((volatile uint32_t *)ARM_TIMER_LOAD) = 1000000-1; // load-value
}


void rpi2_timer_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	static char scratchpad[16];
	char *p;
	int i;
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	p = "\r\nTIMER EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_svc_context);

	while (1); // hang
}

#endif

// exception handlers

// debug print for naked exception functions
void naked_debug_2(uint32_t src, uint32_t link, uint32_t stack)
{
	int len;
	char tmp[16];

	if (rpi2_debuggee_running)
	{

		serial_raw_puts("\r\nnak_dbg");
		serial_raw_puts("\r\nPC: ");
		util_word_to_hex(tmp, src);
		serial_raw_puts(tmp);
		serial_raw_puts(" LR: ");
		util_word_to_hex(tmp, link);
		serial_raw_puts(tmp);
		serial_raw_puts(" SP: ");
		util_word_to_hex(tmp, stack);
		serial_raw_puts(tmp);
		serial_raw_puts("\r\n");
	}
}

// this protects the processor state
static inline void naked_debug()
{
	asm volatile (
			"push {r2}\n\t"
			"mov r2, sp\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"push {r0, lr}\n\t"
			"sub r0, pc, #24\n\t"
			"mov r1, lr\n\t"
			"bl naked_debug_2\n\t"
			"pop {r0, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"pop {r2}\n\t"
	);
}

#if 0
static inline void check_irq_stack()
{
	asm volatile (
			"push {r2}\n\t"
			"mov r2, sp\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"dsb\n\t"
			"push {r0, lr}\n\t"

			"ldr r0, =__irq_stack\n\t"
			"ldr r1, =__svc_stack\n\t"
			"cmp sp, r0 @ > : sp corrupted\n\t"
			"cmpls r1, sp @ > : sp corrupted\n\t"
			"bls 1f\n\t"
			"sub r0, pc, #44\n\t"
			"mov r1, lr\n\t"
			"bl naked_debug_2\n\t"

			"1: \n\t"
			"pop {r0, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"pop {r2}\n\t"
	);
}
#endif

// for through-gdb logging
void rpi2_gdb_log(char *ptr, uint32_t size, uint32_t fun)
{
	int len;
#if 0
	char scratch[9];
#endif
	len = size;
	if (fun == RPI2_TRAP_LOGZ)
	{
		len = util_str_len(ptr);
	}
#if 0
	util_word_to_hex(scratch, fun);
	serial_raw_puts("\r\nlog: fun= ");
	serial_raw_puts(scratch);
	util_word_to_hex(scratch, len);
	serial_raw_puts(" len= ");
	serial_raw_puts(scratch);
	serial_raw_puts("\r\n");
#endif
	gdb_send_text_packet(ptr, (unsigned int) len);
}

// common trap handler outside exception handlers
void rpi2_trap_handler()
{
#ifdef DEBUG_GDB_EXC
	serial_raw_puts("\r\ntrap_hndlr\r\n");
#endif

	if (rpi2_dgb_enabled)
	{
		gdb_trap_handler();
	}
}

// exception handlers for re-routed exceptions
// in these, "pre-handling" can be added
void rpi2_reroute_reset()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif

	asm volatile(
			"mov pc, #0 @ jump to low vector\n\t"
	);
}

void rpi2_reroute_exc_und()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif

	asm volatile(
			"mov pc, #4 @ jump to low vector\n\t"
	);
}

void rpi2_reroute_exc_svc()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #8 @ jump to low vector\n\t"
	);
}

void rpi2_reroute_exc_dabt()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #16 @ jump to low vector\n\t"
	);
}

void rpi2_reroute_exc_aux()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #20 @ jump to low vector\n\t"
	);
}

// in case uart0 is handled by firq instead
void rpi2_reroute_exc_irq()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #24 @ jump to low vector\n\t"
	);
}

void rpi2_reroute_exc_fiq()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #28 @ jump to low vector\n\t"
	);
}



// The exception handlers are attributed as 'naked'so that C function prologue
// doesn't disturb the stack before the exception stack frame is pushed
// into the stack, because the prologue would change the register values, and
// we would return from exception before we get to the C function epilogue,
// and we'd return with unbalanced stack.
// Naked functions shouldn't have any C-code in them.

// There are two exception handlers per exception: the upper vector handlers
// and the (emulated) lower vector handlers. Some upper vector handlers just
// jump to the lower vector, and if the vector is not overwritten by debuggee,
// the vector contains a jump to unhandled exception handler.

// upper level handler for gdb entry
void gdb_exception_handler()
{
	volatile rpi2_reg_context_t *ctx;
	int i;
#ifdef DEBUG_GDB_EXC
	uint32_t exc_cpsr;
	uint32_t tmp;
	char *pp;
	static char scratchpad[16]; // scratchpad
#endif

	switch (exception_info)
	{
	case RPI2_EXC_RESET:
		ctx = &rpi2_svc_context;
		break;
	case RPI2_EXC_UNDEF:
		ctx = &rpi2_svc_context;
		break;
	case RPI2_EXC_SVC:
		ctx = &rpi2_svc_context;
		break;
	case RPI2_EXC_PABT:
		ctx = &rpi2_pabt_context;
		break;
	case RPI2_EXC_DABT:
		ctx = &rpi2_dabt_context;
		break;
	case RPI2_EXC_AUX:
		ctx = &rpi2_svc_context;
		break;
	case RPI2_EXC_IRQ:
		ctx = &rpi2_irq_context;
		break;
	case RPI2_EXC_FIQ:
		ctx = &rpi2_fiq_context;
		break;
	default:
		ctx = &rpi2_svc_context;
		break;
	}
	// copy irq context to reg context for returning
	// to original program that was interrupted by CTRL-C
	for (i=0; i<18; i++)
	{
		rpi2_reg_context.storage[i] = ctx->storage[i];
	}

	if (rpi2_sigint_flag) rpi2_ctrl_c_flag = 0;

	// interrupts need to be enabled for serial I/O?
	// asm volatile ("cpsid aif\n\t");
	asm volatile ("cpsid if\n\t");
	if (rpi2_uart0_excmode == RPI2_UART0_FIQ)
	{
		asm volatile ("cpsie f\n\t");
	}
	else if (rpi2_uart0_excmode == RPI2_UART0_IRQ)
	{
		asm volatile ("cpsie i\n\t");
	}
	// else keep all exceptions masked

#ifdef DEBUG_GDB_EXC
#if 0
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);
#endif
	// raw puts, because we are here with interrupts disabled
	pp = "\r\nGDB EXCEPTION";
	do {i = serial_raw_puts(pp); pp += i;} while (i);
#if 0
	serial_raw_puts("\r\nexc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" SPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
#endif
	serial_raw_puts("\r\nctx: ");
	util_word_to_hex(scratchpad, (unsigned int)ctx);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" info: ");
	util_word_to_hex(scratchpad, (unsigned int)exception_info);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" extra: ");
	util_word_to_hex(scratchpad, (unsigned int)exception_extra);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" sigint: ");
	util_word_to_hex(scratchpad, (unsigned int)rpi2_sigint_flag);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_reg_context);
#endif

	rpi2_trap_handler();

	if (rpi2_keep_ctrlc)
	{
		if (rpi2_uart0_excmode == RPI2_UART0_FIQ)
		{
			// BCM8235 ARM Peripherals ch 7.3
			*((volatile uint32_t *)IRC_DIS2) = (1 << 25);
			*((volatile uint32_t *)IRC_FIQCTRL) = ((1 << 7) | 57);
			rpi2_reg_context.reg.cpsr &= ~(1<<6); // enable fiq
		}
		else
		{
			*((volatile uint32_t *)IRC_EN2) = (1 << 25);
			rpi2_reg_context.reg.cpsr &= ~(1<<7); // enable irq
		}
	}
}

// for debugging
void show_go(uint32_t retaddr)
{
	uint32_t xspsr;
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (xspsr) ::
	);

	serial_raw_puts("\n\t going to: ");
	util_word_to_hex(dbg_msg_buff, (unsigned int)retaddr);
	serial_raw_puts(dbg_msg_buff);
	serial_raw_puts(" spsr: ");
	util_word_to_hex(dbg_msg_buff, (unsigned int)xspsr);
	serial_raw_puts(dbg_msg_buff);
	serial_raw_puts("\r\n");
}

// the gdb "entry"
void rpi2_gdb_exception()
{
	// switch to PABT
	asm volatile (
			"mrs r0, cpsr\n\t"
			"and r0, #0x1f\n\t"
			"cmp r0, #0x17\n\t"
			"beq 1f\n\t"
			"mov r0, #0x17 @ abt-mode\n\t"
			"msr cpsr_c, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"1:\n\t"
			"movw sp, #:lower16:__gdb_stack\n\t"
			"movt sp, #:upper16:__gdb_stack\n\t"
			"bl gdb_exception_handler\n\t"
			"ldr r0, =rpi2_reg_context\n\t"
	);

	read_context();

	asm volatile (
#ifdef DEBUG_GDB_EXC
			"push {r0 - r4, lr}\n\t"
			"mov r0, lr\n\t"
			"bl show_go\n\t"
			"pop {r0 - r4, lr}\n\t"
#endif
			"push {r0, r1}\n\t"
			"ldr r0, =rpi2_use_debug_mode\n\t"
			"ldr r1, [r0]\n\t"
			"cmp r1, #0\n\t"
			"beq 2f\n\t"
			"mrc p14, 0, r0, c0, c2, 2 @ dbgdscr_ext\n\t"
			"dsb\n\t"
			"orr r0, #0x8000 @ MDBGen\n\t"
			"mcr p14, 0, r0, c0, c2, 2 @ dbgdscr_ext\n\t"
			"dsb\n\t"
			"isb\n\t"
			"2: \n\t"
			"pop {r0, r1}\n\t"
			"subs pc, lr, #0\n\t"
	);
}

void rpi2_reset_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, rst_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__svc_stack\n\t"
			"movt sp, #:upper16:__svc_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, rst_sp_store\n\t"
			"ldr r1, =rpi2_svc_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1: @ context stored\n\t"
	);

	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #0 @ RPI2_EXC_RESET\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #0 @ nothing\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"rst_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_UNDEF
void rpi2_undef_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	static char scratchpad[16];
	char *p;
	int i;
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// for fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	//rpi2_reg_context.reg.r15 -= 4; // PC (already done in assembly)

	p = "\r\nUNDEFINED EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_svc_context);
}
#endif

void rpi2_undef_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, und_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__und_stack\n\t"
			"movt sp, #:upper16:__und_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
			"sub lr, #4 @ on return, skip the instruction\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, und_sp_store\n\t"
			"ldr r1, =rpi2_svc_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1: @ context stored\n\t"
	);

	// rpi2_undef_handler2() // - No C in naked function
	asm volatile (
#ifdef DEBUG_UNDEF
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_undef_handler2\n\t"
#endif
			"ldr r0, =exception_info\n\t"
			"mov r1, #1 @ RPI2_EXC_UNDEF\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #0 @ nothing\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"und_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_SVC
void rpi2_svc_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	int i;
	char *p;
	static char scratchpad[16];
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// for fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	// rpi2_reg_context.reg.r15 -= 0; // PC

	p = "\r\nSVC EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_sp: ");
	util_word_to_hex(scratchpad, stack_frame_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nexc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_svc_context);
}
#endif


void rpi2_svc_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, svc_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__svc_stack\n\t"
			"movt sp, #:upper16:__svc_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"dsb\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, svc_sp_store\n\t"
			"ldr r1, =rpi2_svc_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1: @ context stored\n\t"
	);

	// rpi2_svc_handler2() // - No C in naked function
#ifdef DEBUG_SVC
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_svc_handler2\n\t"
	);
#endif

	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #2 @ RPI2_EXC_SVC\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #0 @ nothing\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"svc_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_AUX
void rpi2_aux_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	int i;
	char *p;
	static char scratchpad[16];
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// for fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	//
	// AUX is often used for SW interrupts - other instructions or different
	// situation (configuration), like SMC or ...
	// rpi2_reg_context.reg.r15 -= 0; // PC

	p = "\r\nAUX EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_svc_context);
}
#endif

// TODO: add stack for aux
void rpi2_aux_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, aux_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__svc_stack\n\t"
			"movt sp, #:upper16:__svc_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, aux_sp_store\n\t"
			"ldr r1, =rpi2_svc_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1: @ context stored\n\t"
	);

	// rpi2_aux_handler2() // - No C in naked function
#ifdef DEBUG_AUX
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_aux_handler2\n\t"
	);
#endif

	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #5 @ RPI2_EXC_AUX\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #0 @ nothing\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"aux_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_DABT
void rpi2_dabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	uint32_t dfsr, dfar, dbgdscr;

	static char scratchpad[16];
	int i;
	char *p;

	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// TODO: add DFSR.FS, DFAR
	// MRC p15, 0, <Rt>, c5, c0, 0 ; Read DFSR into Rt
	// MRC p15, 0, <Rt>, c6, c0, 0 ; Read DFAR into Rt
	// LR = faulting instruction address
	asm volatile ("mrc p15, 0, %0, c5, c0, 0\n\t" : "=r" (dfsr) ::);
	asm volatile ("mrc p15, 0, %0, c6, c0, 0\n\t" : "=r" (dfar) ::);
	asm volatile ("mrc p14, 0, %0, c0, c2, 2\n\t" : "=r" (dbgdscr) ::);

	p = "\r\nDABT EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" SPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nDFSR: ");
	util_word_to_hex(scratchpad, dfsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" DFAR: ");
	util_word_to_hex(scratchpad, dfar);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" DBGDSCR: ");
	util_word_to_hex(scratchpad, dbgdscr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_dabt_context);
}
#endif

void rpi2_unhandled_dabt()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, dabt_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__abrt_stack\n\t"
			"movt sp, #:upper16:__abrt_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"sub lr, #8 @ fix return address\n\t"
			"ldr r12, =rpi2_dabt_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, dabt_sp_store\n\t"
			"ldr r1, =rpi2_dabt_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1: @ context stored\n\t"
	);

	// rpi2_dabt_handler() // - No C in naked function
#ifdef DEBUG_DABT
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_dabt_handler2\n\t"
	);
#endif

	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #4 @ RPI2_EXC_DABT\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #10 @ RPI2_REASON_HW_EXC\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"dabt_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_dabt_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, dabt_sp_store2\n\t"
			"dsb\n\t"
			"movw sp, #:lower16:__abrt_stack\n\t"
			"movt sp, #:upper16:__abrt_stack\n\t"
			"dsb\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"dsb\n\t"
			"push {r0, r1, lr} @ to keep stack 8-byte aligned\n\t"

			"@ turn off debug monitor mode\n\t"
			"mrc p14, 0, r0, c0, c2, 2 @ dbgdscr_ext\n\t"
			"dsb\n\t"
			"bic r0, #0x8000 @ MDBGen\n\t"
			"mcr p14, 0, r0, c0, c2, 2 @ dbgdscr_ext\n\t"
			"dsb\n\t"
			"isb\n\t"

			"@ check the exception cause\n\t"
			"mrc p15, 0, r0, c6, c0, 0 @ DFAR\n\t"
			"dsb\n\t"
			"ldr r1, =rpi2_dbg_rec\n\t"
			"str r0, [r1]\n\t"
			"str lr, [r1, #0xc]\n\t"
			"mrc p15, 0, r0, c5, c0, 0 @ DFSR\n\t"
			"dsb\n\t"
			"ldr r1, =rpi2_dbg_rec\n\t"
			"str r0, [r1, #4]\n\t"
			"ands r1, r0, #0x400 @ bit 10\n\t"
			"bne dabt_other\n\t"
			"and r0, #0x0f\n\t"
			"cmp r0, #0x2 @ debug event\n\t"
			"bne dabt_other\n\t"
			"mrc p14, 0, r0, c0, c2, 2 @ read dbgdscr\n\t"
			"dsb\n\t"
			"ldr r1, =rpi2_dbg_rec\n\t"
			"str r0, [r1, #8]\n\t"
			"lsr r0, #2 @ MOE-field\n\t"
			"and r0, #0xf\n\t"
			"cmp r0, #0xa @ synch watchpoint?\n\t"
			"bne dabt_other\n\t"

			"@ watchpoint\n\t"
			"mov r3, #6 @ RPI2_TRAP_WATCH\n\t"
			"ldr r0, =exception_extra\n\t"
			"str r3, [r0]\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #4 @ RPI2_EXC_DABT\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"sub lr, #8 @ fix lr - gdb doesn't do it\n\t"
			"pop {r0 - r12}\n\t"
);

	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_dabt_context\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, dabt_sp_store2\n\t"
			"ldr r1, =rpi2_dabt_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1:\n\t"
	);
#ifdef DEBUG_PDBT
	// rpi2_dabt_handler2() // - No C in naked function
	asm volatile (
			"ldr r0, dabt_sp_store2\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_dabt_handler2\n\t"
	);
#endif
	asm volatile (
			"b rpi2_gdb_exception\n\t"
	);
	asm volatile (
			"dabt_other: @ not ours - re-route\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, dabt_sp_store2\n\t"
			"mov pc, #16 @ low DABT vector address\n\t"
	);
	asm volatile (
			"dabt_sp_store2:\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_IRQ
#define SER_DEBUG
#endif
#ifdef DEBUG_CTRLC
#define SER_DEBUG
#endif
//extern volatile int ser_handle_ctrlc; // used for debugging

// returns code for what to do after return
uint32_t rpi2_serial_handler(uint32_t stack_pointer, uint32_t exc_addr)
{
	uint32_t irq_status;
	uint32_t retval;
	uint32_t exc_cpsr;
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad

	retval = 0; // default: goto low vector

	if (rpi2_uart0_excmode == RPI2_UART0_FIQ)
	{
		serial_irq();
	}
	else
	{
		irq_status =  *((volatile uint32_t *)IRC_PENDB);
		if (irq_status & (1 << 19)) // bit 19 = UART0
		{
			irq_status &= ~(1 << 19); // clear bit 9
			serial_irq();
		}
	}
	if (!rpi2_sigint_flag)
	{
		if (rpi2_ctrl_c_flag)
		{
			// CTRL-C
			// swap flags to prevent multiple handling of the same ctrl-c
			rpi2_ctrl_c_flag = 0;
			rpi2_sigint_flag = 1;
			exception_extra = RPI2_REASON_SIGINT;
			rpi2_exc_reason = RPI2_REASON_SIGINT;
			retval = 2; // don't return - goto gdb
		}
	}

	if (retval == 0)
	{
		if (rpi2_uart0_excmode != RPI2_UART0_FIQ)
		{
			if (irq_status == 0)
			{
				retval = 1; // return to interrupted program
			}
			// else retval = 0
		}
		else
		{
			retval = 1; // can't be other fiq
		}
	}

#ifdef SER_DEBUG
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	if (rpi2_sigint_flag)
//	if (gdb_dyn_debug)
	{
		p = "\r\nSIGINT EXCEPTION\r\n";
		do {i = serial_raw_puts(p); p += i;} while (i);

		serial_raw_puts("exc_addr: ");
		util_word_to_hex(scratchpad, exc_addr);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" SP: ");
		util_word_to_hex(scratchpad, stack_pointer);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" SPSR: ");
		util_word_to_hex(scratchpad, exc_cpsr);
		serial_raw_puts(scratchpad);
		tmp = *((volatile uint32_t *)IRC_PENDB);
		serial_raw_puts("\r\nPENDB: ");
		util_word_to_hex(scratchpad, tmp);
		serial_raw_puts(scratchpad);
		tmp = *((volatile uint32_t *)IRC_PEND1);
		serial_raw_puts(" PEND1: ");
		util_word_to_hex(scratchpad, tmp);
		serial_raw_puts(scratchpad);
		tmp = *((volatile uint32_t *)IRC_PEND2);
		serial_raw_puts(" PEND2: ");
		util_word_to_hex(scratchpad, tmp);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\nctrlc-flag: ");
		util_word_to_hex(scratchpad, (unsigned int) rpi2_ctrl_c_flag);
		serial_raw_puts(scratchpad);
		serial_raw_puts("handle_ctrlc: ");
		util_word_to_hex(scratchpad, (unsigned int) ser_handle_ctrlc);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" irq-flag: ");
		util_word_to_hex(scratchpad, (unsigned int) rpi2_sigint_flag);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
#ifdef DEBUG_CTRLC
		rpi2_dump_context(&rpi2_irq_context);
#endif
	}
#endif
	return retval;
}

void rpi2_unhandled_irq()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, irq_sp_store1\n\t"
			"movw sp, #:lower16:__irq_stack\n\t"
			"movt sp, #:upper16:__irq_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_irq_context\n\t"
			"sub lr, #4 @ fix return address\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, irq_sp_store1\n\t"
			"ldr r1, =rpi2_irq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1:\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #6 @ RPI2_EXC_IRQ\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #10 @ RPI2_REASON_HW_EXC\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"irq_sp_store1: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
	);
}

void rpi2_irq_handler()
{
	asm volatile (
			"str sp, irq_sp_store2\n\t"
			"movw sp, #:lower16:__irq_stack\n\t"
			"movt sp, #:upper16:__irq_stack\n\t"
			"dsb\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"push {r0, r1, lr} @ r1 to keep sp double word aligned\n\t"
			"ldr r0, =rpi2_uart0_excmode\n\t"
			"ldr r0, [r0]\n\t"
			"cmp r0, #1 @ RPI2_UART0_FIQ\n\t"
			"beq 1f @ serial doesn't use irq - other irqs\n\t"

			"ldr r0, irq_sp_store2\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_serial_handler\n\t"
			"cmp r0, #2\n\t"
			"beq 2f @ ctrl-c \n\t"
			"cmp r0, #1\n\t"
			"bne 1f\n\t"

			"@ only serial irq\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, irq_sp_store2\n\t"
			"subs pc, lr, #4\n\t"

			"1: @ other irqs\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, irq_sp_store2\n\t"
			"mov pc, #24 @ jump to irq low vector\n\t"

			"2: @ ctrl-c \n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #6 @ RPI2_EXC_IRQ\n\t"
			"str r1, [r0]\n\t"

			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"sub lr, #4 @ fix return address\n\t"
			"push {r12}\n\t"
			"ldr r12, =rpi2_irq_context\n\t"
	);
// store processor context
write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 3f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, irq_sp_store2\n\t"
			"ldr r1, =rpi2_irq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"3: \n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"irq_sp_store2: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_unhandled_fiq()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, fiq_sp_store1\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__fiq_stack\n\t"
			"movt sp, #:upper16:__fiq_stack\n\t"
			"dsb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"sub lr, #4 @ fix return address\n\t"
			"ldr r12, =rpi2_fiq_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, fiq_sp_store1\n\t"
			"ldr r1, =rpi2_fiq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1:\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #7 @ RPI2_EXC_FIQ\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #10 @ RPI2_REASON_HW_EXC\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"

			"fiq_sp_store1:\n\t"
			".int 0\n\t"
	);
}

void rpi2_fiq_handler()
{
	asm volatile (
			"str sp, fiq_sp_store2\n\t"
			"movw sp, #:lower16:__fiq_stack\n\t"
			"movt sp, #:upper16:__fiq_stack\n\t"
			"dsb\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"push {r0, r1, lr} @ r1 to keep sp double word aligned\n\t"
			"ldr r0, =rpi2_uart0_excmode\n\t"
			"ldr r0, [r0]\n\t"
			"cmp r0, #1 @ RPI2_UART0_FIQ\n\t"
			"bne 1f @ serial doesn't use fiq - other fiq\n\t"

			"ldr r0, fiq_sp_store2\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_serial_handler\n\t"
			"cmp r0, #2\n\t"
			"beq 2f @ ctrl-c \n\t"

			"@ only serial fiq\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, fiq_sp_store2\n\t"
			"subs pc, lr, #4\n\t"

			"1: @ other irqs\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, fiq_sp_store2\n\t"
			"mov pc, #28 @ jump to fiq low vector\n\t"
	);

	asm volatile (
			"2: @ ctrl_c\n\t"
#ifdef DEBUG_EXCEPTIONS
	);
	naked_debug();
	asm volatile (
#endif
			"ldr r0, =exception_info\n\t"
			"mov r1, #7 @ RPI2_EXC_FIQ\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"sub lr, #4 @ fix return address\n\t"
			"push {r12}\n\t"
			"ldr r12, =rpi2_fiq_context\n\t"
	);
// store processor context
write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 3f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, fiq_sp_store2\n\t"
			"ldr r1, =rpi2_fiq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"3: \n\t"
			"ldr r0, =rpi2_gdb_exception\n\t"
			"mov pc, r0\n\t"
			"fiq_sp_store2:\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

// for debugging
void rpi2_print_dbg(uint32_t xlr, uint32_t xcpsr, uint32_t xspsr)
{
	uint32_t xpc;

	asm volatile("mov %[reg], lr\n\t" :[reg] "=r" (xpc)::);
	serial_raw_puts("pc: ");
	util_word_to_hex(dbg_tmp_buff, xpc);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts(" lr: ");
	util_word_to_hex(dbg_tmp_buff, xlr);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts("\r\ncpsr: ");
	util_word_to_hex(dbg_tmp_buff, xcpsr);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts(" spsr: ");
	util_word_to_hex(dbg_tmp_buff, xspsr);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts("\r\n");
}

void rpi2_pabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	int i;
	uint32_t *p;
	uint32_t exc_cpsr;
	uint32_t dbg_state;
	uint32_t ifsr;
	uint32_t our_lr;
	volatile rpi2_reg_context_t *ctx;
#ifdef DEBUG_PABT
	uint32_t tmp;
	char *pp;
	static char scratchpad[16]; // scratchpad
#else
#ifdef DEBUG_REG_CONTEXT
	static char scratchpad[16]; // scratchpad
#endif
#endif
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);
	asm volatile
	(
			"mov %[var_reg], lr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (our_lr) ::
	);

#if 0
	// all bits are not reliable if read via CP14 interface
	p = (uint32_t *)(dbg_reg_base + 0x88); // DBGDSCRext
	dbg_state = *p;
#endif

	// MRC p15, 0, <Rt>, c5, c0, 1 // Read IFSR (exception cause)
	asm volatile
	(
			"mrc p15, 0, %[var_reg], c5, c0, 1 @ IFSR\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (ifsr) ::
	);

#ifdef DEBUG_PABT
	// raw puts, because we are here with interrupts disabled
	pp = "\r\nPABT EXCEPTION\r\n";
	do {i = serial_raw_puts(pp); pp += i;} while (i);
	serial_raw_puts("our_lr: ");
	util_word_to_hex(scratchpad, our_lr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nexc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" SPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\ndbgdscr: ");
	util_word_to_hex(scratchpad, dbg_state);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" IFSR: ");
	util_word_to_hex(scratchpad, ifsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nctx: ");
	util_word_to_hex(scratchpad, (unsigned int)ctx);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" info: ");
	util_word_to_hex(scratchpad, (unsigned int)exception_info);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" extra: ");
	util_word_to_hex(scratchpad, (unsigned int)exception_extra);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" ctrl-c: ");
	util_word_to_hex(scratchpad, (unsigned int)rpi2_sigint_flag);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_pabt_context);
#endif

#if 0
	// if we were in debug-state

	p = (uint32_t *)(dbg_reg_base + 0x90); // DBGDRCR
	*p = 0x06; // clear stickies and request exit from debug state
	asm volatile (
			"dsb\n\t"
			"isb\n\t"
	);

	/*
	DBGLSR - lock status
	DBGLAR - lock access: to release, write 0x5cacce55 in it, any other value locks

	tmp = *((uint32_t *) (dbg_reg_base + 0xfb4)); // DBGLSR
	serial_raw_puts(" DBGLSR: ");
	util_word_to_hex(scratchpad, tmp);
	serial_raw_puts(scratchpad);
	if (tmp & 0x2) // if locked, open lock
	{
		*((uint32_t *) (dbg_reg_base + 0xfb0)) = 0xC5ACCE55; // DBGLAR
		asm volatile (
				"dsb\n\t"
				"isb\n\t"
		);
		tmp = *((uint32_t *) (dbg_reg_base + 0xfb4)); // DBGLSR
		serial_raw_puts(" DBGLSR2: ");
		util_word_to_hex(scratchpad, tmp);
		serial_raw_puts(scratchpad);
	}93
	*/

	p = (uint32_t *)(dbg_reg_base + 0x88); // DBGDSCRext
	dbg_state = *p;
	SYNC;
	/*
	while (!(dbg_state & 0x2)) // while not 'restarted'
	{
		dbg_state = *p;
	}
	*/
	for (i=0; i<100000; i++)
	{
		dbg_state = *p;
		SYNC;
		if (dbg_state & 0x2) break;
	}

	serial_raw_puts("\r\ndbgdscr2: ");
	util_word_to_hex(scratchpad, dbg_state);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\ni: ");
	util_word_to_hex(scratchpad, i);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
#endif

#ifdef DEBUG_PABT
	if (ifsr & 0x40f) // BKPT
	{
		serial_raw_puts("\r\nBKPT\r\n");
		serial_raw_puts("ret_addr: ");
		util_word_to_hex(scratchpad, rpi2_pabt_context.reg.r15);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
	}
	else
	{
		pp = "\r\nPrefetch Abort\r\n";
		do {i = serial_raw_puts(pp); pp += i;} while (i);
	}
#endif


#if 0
	serial_raw_puts("\r\nBKPT\r\n");
	util_word_to_hex(scratchpad, (unsigned int) ser_handle_ctrlc);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" =flag\r\n");
	serial_raw_puts(" SPSR: ");
	util_word_to_hex(scratchpad, rpi2_pabt_context.reg.cpsr);
	serial_raw_puts(scratchpad);
	//rpi2_reg_context.reg.cpsr &= ~(1<<8); // enable irqs (fishy)
#endif
	//rpi2_gdb_exception();
}


void rpi2_unhandled_pabt()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, pabt_sp_store1\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__abrt_stack\n\t"
			"movt sp, #:upper16:__abrt_stack\n\t"
			"dsb\n\t"
			"@sub lr, #4 @ for now we skip the instruction\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_pabt_context\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, pabt_sp_store1\n\t"
			"ldr r1, =rpi2_pabt_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1:\n\t"
	);
	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #3 @ RPI2_EXC_PABT\n\t"
			"str r1, [r0]\n\t"
			"mrc p15, 0, r0, c5, c0, 1 @ IFSR\n\t"
			"dsb\n\t"
			"ands r1, r0, #0x400 @ bit 10\n\t"
			"bne 2f\n\t"
			"and r0, #0x0f @ bit 10\n\t"
			"cmp r0, #0x2 @ debug event\n\t"
			"bne 2f\n\t"

			"@ breakpoint\n\t"
			"mov r1, #4 @ RPI2_TRAP_BKPTn\t"
			"ldr r0, =exception_extra\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_exc_reason\n\t"
			"mov r1, #12 @ RPI2_REASON_SW_EXC\n\t"
			"str r1, [r0]\n\t"
			"b 3f\n\t"
#if 0
			"ldr r0, =dbg_rom_base @ cause DABT\n\t"
			"ldr r0, [r0]\n\t"
			"str r1,[r0]\n\t"
#endif
			"2: \n\t"
#ifdef DEBUG_PABT
	);
	naked_debug();
	asm volatile (
#endif
			"ldr r0, =exception_extra\n\t"
			"mov r1, #5 @ RPI2_TRAP_BUS\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_exc_reason\n\t"
			"mov r1, #10 @ RPI2_REASON_HW_EXC\n\t"
			"str r1, [r0]\n\t"
			"@ fix return address\n\t"
			"ldr r1, [r0, #15*4]\n\t"
			"sub r1, #4 @ fix address to exception address\n\t"
			"str r1, [r0, #15*4]\n\t"
			"ldr r0, =rpi2_pabt_context\n\t"

			"3: \n\t"
	);
#ifdef DEBUG_PABT
	asm volatile (
			"ldr r0, pabt_sp_store1\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_pabt_handler2\n\t"
	);
#endif
	asm volatile (
			"b rpi2_gdb_exception\n\t"

			"pabt_sp_store1:\n\t"
			".int 0\n\t"
	);
}


void rpi2_pabt_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, pabt_sp_store2\n\t"
			"dsb\n\t"
			"movw sp, #:lower16:__abrt_stack\n\t"
			"movt sp, #:upper16:__abrt_stack\n\t"
			"dsb\n\t"
			"push {r0 - r12}\n\t"
			"mov r5, r0 @ possible parameters\n\t"
			"mov r6, r1\n\t"
			"mrs r0, cpsr\n\t"
			"dsb\n\t"
			"push {r0, r1, lr} @ to keep stack 8-byte aligned\n\t"

			"@ turn off debug monitor mode\n\t"
			"mrc p14, 0, r0, c0, c2, 2 @ dbgdscr_ext\n\t"
			"dsb\n\t"
			"bic r0, #0x8000 @ MDBGen\n\t"
			"mcr p14, 0, r0, c0, c2, 2 @ dbgdscr_ext\n\t"
			"dsb\n\t"
			"isb\n\t"

			"@ check the cause of exception\n\t"
			"mrc p15, 0, r0, c5, c0, 1 @ IFSR\n\t"
			"dsb\n\t"
			"ands r1, r0, #0x400 @ bit 10\n\t"
			"bne 1f\n\t"
			"and r0, #0x0f\n\t"
			"cmp r0, #0x2 @ debug event\n\t"
			"bne 1f\n\t"

			"@ breakpoint\n\t"
			"mov r3, #4 @ RPI2_TRAP_BKPT\n\t"
			"sub r0, lr, #4\n\t"
			"ldr r1, [r0] @ get instruction\n\t"
			"movw r0, #0xff7f @ user bkpt arm\n\t"
			"movt r0, #0xe127\n\t"
			"cmp r1, r0\n\t"
			"moveq r3, #1 @ RPI2_TRAP_ARM\n\t"
			"beq 2f @ our bkpt\n\t"
			"movw r0, #0xff7e @ internal bkpt\n\t"
			"movt r0, #0xe127\n\t"
			"cmp r1, r0\n\t"
			"moveq r3, #1 @ RPI2_TRAP_ARM\n\t"
			"beq 2f @ our bkpt\n\t"
			"movw r0, #0xff7c @ logging bkpt\n\t"
			"movt r0, #0xe127\n\t"
			"cmp r1, r0\n\t"
			"moveq r3, #13 @ RPI2_TRAP_LOGZ\n\t"
			"beq bkpt_log @ our bkpt\n\t"
			"movw r0, #0xff7b @ logging bkpt\n\t"
			"movt r0, #0xe127\n\t"
			"cmp r1, r0\n\t"
			"moveq r3, #14 @ RPI2_TRAP_LOGN\n\t"
			"beq bkpt_log @ our bkpt\n\t"
			"movw r0, #0xff7d @ internal bkpt\n\t"
			"movt r0, #0xe127\n\t"
			"cmp r1, r0\n\t"
			"moveq r3, #0xf @ RPI2_TRAP_INITIAL\n\t"
			"beq 2f @ our bkpt\n\t"
			"@ for thumb,the exception offset is just 2\n\t"
			"sub r0, lr, #2 @ exception address\n\t"
			"ldrh r1, [r0] @ get instruction\n\t"
			"movw r0, #0xbebe @ user bkpt thumb\n\t"
			"cmp r1, r0\n\t"
			"moveq r3, #2 @ our bkpt\n\t"
			"beq 2f @ our bkpt\n\t"

			"ldr r0, =exception_extra\n\t"
			"str r3, [r0]\n\t"

			"1: @ not ours - re-route\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, pabt_sp_store2\n\t"
			"mov pc, #12 @ low PABT vector address\n\t"

			"2: @ ours\n\t"
			"ldr r0, =exception_extra\n\t"
			"str r3, [r0]\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #3 @ RPI2_EXC_PABT\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"sub lr, #4\n\t"
			"pop {r0 - r12}\n\t"
);

	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_pabt_context\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"dsb\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 3f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, pabt_sp_store2\n\t"
			"ldr r1, =rpi2_pabt_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"3:\n\t"
	);
#ifdef DEBUG_PABT
	// rpi2_pabt_handler2() // - No C in naked function
	asm volatile (
			"ldr r0, pabt_sp_store2\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_pabt_handler2\n\t"
	);
#endif
	asm volatile (
			"b rpi2_gdb_exception\n\t"
	);
	asm volatile (
			"bkpt_log: @ logging\n\t"
			"mov r0, r5\n\t"
			"mov r1, r6\n\t"
			"mov r2, r3\n\t"
			"bl rpi2_gdb_log\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, pabt_sp_store2\n\t"
			"subs pc, lr, #0\n\t"
	);
	asm volatile (
			"pabt_sp_store2:\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_trap()
{
#ifdef DEBUG_EXCEPTIONS
	serial_raw_puts("\r\nrpi2_trap\r\n");
#endif
	asm("bkpt #0x7ffe\n\t");
}

void rpi2_gdb_trap()
{
#ifdef DEBUG_EXCEPTIONS
	serial_raw_puts("\r\ninit_trap\r\n");
#endif
	asm("bkpt #0x7ffd\n\t");
}
/* set BKPT to given address */
void rpi2_set_trap(void *address, int kind)
{
	uint16_t *location_t = (uint16_t *)address;
	uint32_t *location_a = (uint32_t *)address;

	/* poke BKPT instruction */
	if (kind == RPI2_TRAP_THUMB)
	{
		*location_t = RPI2_USER_BKPT_THUMB; // BKPT #be
		rpi2_flush_address((unsigned int) location_t);
	}
	else
	{
#ifdef DEBUG_GDB_EXC
		serial_raw_puts("\r\nbkpt at: ");
		util_word_to_hex(dbg_tmp_buff, (unsigned int)location_a);
		serial_raw_puts(dbg_tmp_buff);
		serial_raw_puts("\r\n");
#endif
		*location_a = RPI2_USER_BKPT_ARM; // BKPT
		rpi2_flush_address((unsigned int) location_a);
	}
	SYNC;
	return;
}

void rpi2_pend_trap()
{
	//serial_raw_puts("\r\nctrl-C\r\n");

	/* set pending flag for SIGINT */
	rpi2_ctrl_c_flag = 1; // for now
	SYNC;
}

unsigned int rpi2_get_sigint_flag()
{
	return (unsigned int) rpi2_sigint_flag;
}

void rpi2_set_sigint_flag(unsigned int val)
{
	rpi2_sigint_flag = (uint32_t)val;
}

unsigned int rpi2_get_exc_reason()
{
	return (unsigned int) rpi2_exc_reason;
}

void rpi2_set_exc_reason(unsigned int val)
{
	rpi2_exc_reason = (uint32_t)val;
}

void rpi2_set_watchpoint(unsigned int num, unsigned int addr, unsigned int control)
{
	uint32_t dbgdscr;
	SYNC;

	volatile uint32_t *pval;
	pval = (volatile uint32_t *)(dbg_reg_base + 0x088);

#if 1
	// set debug monitor mode
	asm volatile ("mrc p14, 0, %[val], c0, c2, 2\n\t" :[val] "=r" (dbgdscr) ::);
	dbgdscr |= (1 << 15); // MDBGen
	asm volatile ("mcr p14, 0, %[val], c0, c2, 2\n\t" ::[val] "r" (dbgdscr) :);
	SYNC;
#endif

#if 0
	// if memory-mapped interface works
	//  96-111 0x180-0x1BC DBGWVRm 4 watchpoint values
	// 112-127 0x1C0-0x1FC DBGWCRm 4 watchpoint controls
	uint32_t *pval, *pctrl;
	pval = (volatile uint32_t *)(dbg_reg_base + 0x180);
	pctrl = (volatile uint32_t *)(dbg_reg_base + 0x1c0);
	pval[num] = (uint32_t)addr;
	pctrl[num] = control;
#else
	// have to use cp14
	switch (num)
	{
	case 0:
		// write to DBGWVR0
		asm volatile ("mcr p14, 0, %[val], c0, c0, 6\n\t" ::[val] "r" (addr) :);
		// write to DBGWCR0
		asm volatile ("mcr p14, 0, %[ctl], c0, c0, 7\n\t" ::[ctl] "r" (control) :);
		break;
	case 1:
		// write to DBGWVR1
		asm volatile ("mcr p14, 0, %[val], c0, c1, 6\n\t" ::[val] "r" (addr) :);
		// write to DBGWCR1
		asm volatile ("mcr p14, 0, %[ctl], c0, c1, 7\n\t" ::[ctl] "r" (control) :);
		break;
	case 2:
		// write to DBGWVR2
		asm volatile ("mcr p14, 0, %[val], c0, c2, 6\n\t" ::[val] "r" (addr) :);
		// write to DBGWCR2
		asm volatile ("mcr p14, 0, %[ctl], c0, c2, 7\n\t" ::[ctl] "r" (control) :);
		break;
	case 3:
		// write to DBGWVR3
		asm volatile ("mcr p14, 0, %[val], c0, c3, 6\n\t" ::[val] "r" (addr) :);
		// write to DBGWCR3
		asm volatile ("mcr p14, 0, %[ctl], c0, c3, 7\n\t" ::[ctl] "r" (control) :);
		break;
	default:
		// do nothing;
		break;
	}
#endif
	SYNC;

#if 1
	// unset debug monitor mode
	asm volatile ("mrc p14, 0, %[val], c0, c2, 2\n\t" :[val] "=r" (dbgdscr) ::);
	dbgdscr |= ~(1 << 15); // MDBGen
	asm volatile ("mcr p14, 0, %[val], c0, c2, 2\n\t" ::[val] "r" (dbgdscr) :);
	SYNC;
#endif
}

void rpi2_unset_watchpoint(unsigned int num)
{
	uint32_t dbgdscr;
	SYNC;
#if 1
	// set debug monitor mode
	asm volatile ("mrc p14, 0, %[val], c0, c2, 2\n\t" :[val] "=r" (dbgdscr) ::);
	dbgdscr |= (1 << 15); // MDBGen
	asm volatile ("mcr p14, 0, %[val], c0, c2, 2\n\t" ::[val] "r" (dbgdscr) :);
	SYNC;
#endif

#if 0
	// if memory-mapped interface works
	uint32_t *pval, *pctrl;

	//pval = (uint32_t *)(dbg_reg_base + 0x180);
	pctrl = (uint32_t *)(dbg_reg_base + 0x1c0);
	pctrl[num] = 0;
#else
	switch (num)
	{
	case 0:
		// write to DBGWCR0
		asm volatile ("mcr p14, 0, %[ctl], c0, c0, 7\n\t" ::[ctl] "r" (0) :);
		break;
	case 1:
		// write to DBGWCR1
		asm volatile ("mcr p14, 0, %[ctl], c0, c1, 7\n\t" ::[ctl] "r" (0) :);
		break;
	case 2:
		// write to DBGWCR2
		asm volatile ("mcr p14, 0, %[ctl], c0, c2, 7\n\t" ::[ctl] "r" (0) :);
		break;
	case 3:
		// write to DBGWCR3
		asm volatile ("mcr p14, 0, %[ctl], c0, c3, 7\n\t" ::[ctl] "r" (0) :);
		break;
	default:
		// do nothing;
		break;
	}
#endif
	SYNC;
#if 1
	// unset debug monitor mode
	asm volatile ("mrc p14, 0, %[val], c0, c2, 2\n\t" :[val] "=r" (dbgdscr) ::);
	dbgdscr |= ~(1 << 15); // MDBGen
	asm volatile ("mcr p14, 0, %[val], c0, c2, 2\n\t" ::[val] "r" (dbgdscr) :);
	SYNC;
#endif
}

// for dumping info about debug HW
void rpi2_check_debug()
{
	int i;
	uint32_t tmp1, tmp2, tmp3, tmp4;
	uint32_t rom_addr, own_debug;
	uint32_t *ptmp1, *ptmp2;
	static char scratchpad[16]; // scratchpad

	asm volatile (
			"mrc p14, 0, %[retreg], c1, c0, 0 @ get DBGDRAR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp1)::
	);
	asm volatile (
			"mrc p14, 0, %[retreg], c2, c0, 0 @ get DBGDSAR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp2)::
	);
	asm volatile (
			"mrc p14, 0, %[retreg], c7, c14, 6 @ get DBGAUTHSTATUS\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp3)::
	);

	serial_raw_puts("\r\nDBGDRAR: ");
	util_word_to_hex(scratchpad, tmp1);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" DBGDSAR: ");
	util_word_to_hex(scratchpad, tmp2);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" DBGAUTH: ");
	util_word_to_hex(scratchpad, tmp3);
	serial_raw_puts(scratchpad);

	tmp1 &= 0xfffff000; // masked DBGDRAR (ROM table address)
	rom_addr = tmp1;
	tmp2 &= 0xfffff000;
	own_debug = tmp2;

	// double lock
	asm volatile (
			"mrc p14, 0, %[retreg], c1, c3, 4 @ get DBGOSDLR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp4)::
	);
	serial_raw_puts("\r\nDBGOSDLR: ");
	util_word_to_hex(scratchpad, tmp4);
	serial_raw_puts(scratchpad);

	if (tmp4 & 1) // double lock on
	{
		tmp3 = 0;
		asm volatile (
				"mcr p14, 0, %[retreg], c1, c1, 4 @ set DBGOSLSR\n\t"
				"dsb\n\t"
				:: [retreg] "r" (tmp3):
		);
		SYNC;
	}

	// OS lock
	asm volatile (
			"mrc p14, 0, %[retreg], c1, c1, 4 @ get DBGOSLSR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp4)::
	);
	serial_raw_puts("\r\nDBGOSLSR: ");
	util_word_to_hex(scratchpad, tmp4);
	serial_raw_puts(scratchpad);
	tmp3 = *((uint32_t *) (own_debug + 0x024)); // DBGECR
	SYNC;
	serial_raw_puts(" DBGECR: ");
	util_word_to_hex(scratchpad, tmp3);
	serial_raw_puts(scratchpad);
	if (tmp4 & 2) // OS lock on
	{
		if (tmp3 & 1) // unlock event enabled
		{
			asm volatile (
					"mrc p14, 0, %[retreg], c1, c5, 4 @ get DBGPRSR\n\t"
					"dsb\n\t"
					: [retreg] "=r" (tmp4)::
			);
			serial_raw_puts(" DBGPRSR: ");
			util_word_to_hex(scratchpad, tmp4);
			serial_raw_puts(scratchpad);
			tmp4 = 1;
			asm volatile (
					"mcr p14, 0, %[retreg], c1, c4, 4 @ set DBGPRCR\n\t"
					:: [retreg] "r" (tmp4):
			);
			SYNC;
			*((uint32_t *) (own_debug + 0x024)) = 0; // DBGECR
			SYNC;
		}
		tmp3 = 0;
		asm volatile (
				"mcr p14, 0, %[retreg], c1, c0, 4 @ set DBGOSLAR\n\t"
				:: [retreg] "r" (tmp3):
		);
		asm volatile (
				"mrc p14, 0, %[retreg], c1, c1, 4 @ get DBGOSLSR\n\t"
				"dsb\n\t"
				: [retreg] "=r" (tmp4)::
		);
		serial_raw_puts(" DBGOSLSR2: ");
		util_word_to_hex(scratchpad, tmp4);
		serial_raw_puts(scratchpad);
		SYNC;
	}

	tmp3 = *((uint32_t *) (own_debug + 0xfb4)); // DBGLSR
	SYNC;
	serial_raw_puts("\r\nDBGLSR: ");
	util_word_to_hex(scratchpad, tmp3);
	serial_raw_puts(scratchpad);
	if (tmp3 & 2) // locked
	{
		*((uint32_t *) (own_debug + 0xfb0)) = 0xC5ACCE55; // DBGLAR
		SYNC;
		tmp3 = *((uint32_t *) (tmp2 + 0xfb4)); // DBGLSR
		SYNC;
		serial_raw_puts(" DBGLSR2: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);
	}
	tmp1 = rom_addr; // masked DBGDRAR (ROM table address)
	ptmp1 = (uint32_t *)tmp1; // ptmp1: pointer to component entries
	for (i=0; i<(0xf00/4); i++)
	{
		tmp2 = *(ptmp1++);
		asm volatile("dsb\n\t");
		// print component entry
		serial_raw_puts("\r\ncomp: ");
		util_word_to_hex(scratchpad, i);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" : ");
		util_word_to_hex(scratchpad, tmp2);
		serial_raw_puts(scratchpad);
		if (tmp2 == 0) break; // end of component entries

		tmp2 &= 0xfffff000; // masked component offset

		ptmp2 = (uint32_t *)(tmp1 + tmp2);
		tmp3 = *ptmp2;
		serial_raw_puts("\r\nDBGDIDR: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);

		ptmp2 = (uint32_t *)(tmp1 + tmp2 + 0xff4);
		tmp3 = ((*ptmp2) & 0xff);
		serial_raw_puts("\r\nCIDR1: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);

		ptmp2 = (uint32_t *)(tmp1 + tmp2 + 0xfd0);
		tmp3 = ((*ptmp2) & 0xff);
		serial_raw_puts("\r\nPIDR5: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);

		ptmp2 = (uint32_t *)(tmp1 + tmp2 + 0xfcc);
		tmp3 = ((*ptmp2) & 0xff);
		serial_raw_puts("\r\nDEVTYPE: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);
		if ((tmp3 & 0xf) == 5)
		{
			serial_raw_puts(" (debug)");
		}
		else if ((tmp3 & 0xf) == 6)
		{
			serial_raw_puts(" (perfmon)");
		}

		ptmp2 = (uint32_t *)(tmp1 + tmp2 + 0xfe0);
		tmp3 = ((*(ptmp2++)) & 0xff);
		asm volatile("dsb\n\t");
		tmp3 |= (((*(ptmp2++)) & 0xff) << 8);
		asm volatile("dsb\n\t");
		tmp3 |= (((*(ptmp2++)) & 0xff) << 16);
		asm volatile("dsb\n\t");
		tmp3 |= (((*(ptmp2++)) & 0xff) << 24);
		asm volatile("dsb\n\t");
		serial_raw_puts("\r\nPIDR: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);
	}
	serial_raw_puts("\r\n");

	/* remember to check the lock: DBGLSR, DBGLAR */

}

unsigned int rpi2_get_clock(unsigned int clock_id)
{
	int i;
	uint32_t request, response;
	request_response_t *arm_clk_req = (request_response_t *)rpi2_mbox_buff;
	arm_clk_req->buff_size = 8*4;
	arm_clk_req->req_resp_code = 0;
	arm_clk_req->tag = 0x00030002; // get clock rate
	arm_clk_req->tag_buff_len = 2*4;
	arm_clk_req->tag_msg_len = 4;
	for (i=0; i<256; i++) arm_clk_req->tag_buff.word_buff[i] = 0;
	arm_clk_req->tag_buff.word_buff[0] = clock_id;
	arm_clk_req->end_tag = 0;

	request = (uint32_t)(arm_clk_req);
	request &= ((~0) << 4);
	request |= 8; // channel for property tags

	SYNC;
	while ( (*((volatile uint32_t *)MBOX0_STATUS)) == MBOX_STATUS_FULL);
	*((volatile uint32_t *)MBOX1_WRITE) = request;
	do {
		SYNC;
		while (*((volatile uint32_t *)MBOX0_STATUS) == MBOX_STATUS_EMPTY);
		response = *((volatile uint32_t *)MBOX0_READ);
	} while ((response & 0xf) != 8);

	// base address = arm_mem_req->tag_buff.word_buff[0];
	// size in bytes = arm_mem_req->tag_buff.word_buff[1];
	if (arm_clk_req->tag_buff.word_buff[0] == clock_id)
		return (arm_clk_req->tag_buff.word_buff[1]);
	else
		return 0; // error
}

unsigned int rpi2_get_arm_ram(unsigned int *start)
{
	int i;
	uint32_t request, response;
	request_response_t *arm_mem_req = (request_response_t *)rpi2_mbox_buff;
	arm_mem_req->buff_size = 8*4;
	arm_mem_req->req_resp_code = 0;
	arm_mem_req->tag = 0x00010005;
	arm_mem_req->tag_buff_len = 2*4;
	arm_mem_req->tag_msg_len = 0;
	for (i=0; i<256; i++) arm_mem_req->tag_buff.word_buff[i] = 0;
	arm_mem_req->end_tag = 0;

	request = (uint32_t)(arm_mem_req);
	request &= ((~0) << 4);
	request |= 8; // channel for property tags

	SYNC;
	while ( (*((volatile uint32_t *)MBOX0_STATUS)) == MBOX_STATUS_FULL);
	*((volatile uint32_t *)MBOX1_WRITE) = request;
	do {
		SYNC;
		while (*((volatile uint32_t *)MBOX0_STATUS) == MBOX_STATUS_EMPTY);
		response = *((volatile uint32_t *)MBOX0_READ);
	} while ((response & 0xf) != 8);

	// base address = arm_mem_req->tag_buff.word_buff[0];
	// size in bytes = arm_mem_req->tag_buff.word_buff[1];
	//arm_mem_req = (request_response_t *)(response & ((~0) << 4));
	*start = (arm_mem_req->tag_buff.word_buff[0]);
	return (arm_mem_req->tag_buff.word_buff[1]);
}

void rpi2_get_cmdline(char *line)
{
	int i;
	uint32_t request, response, linelen;
	request_response_t *arm_mem_req = (request_response_t *)rpi2_mbox_buff;
	arm_mem_req->buff_size = sizeof(rpi2_mbox_buff);
	arm_mem_req->req_resp_code = 0;
	arm_mem_req->tag = 0x00050001;
	arm_mem_req->tag_buff_len = 1024;
	arm_mem_req->tag_msg_len = 0;
	for (i=0; i<256; i++) arm_mem_req->tag_buff.word_buff[i] = 0;
	arm_mem_req->end_tag = 0;

	request = (uint32_t)(arm_mem_req);
	request &= ((~0) << 4);
	request |= 8; // channel for property tags
	SYNC;
	while ( (*((volatile uint32_t *)MBOX0_STATUS)) == MBOX_STATUS_FULL);
	*((volatile uint32_t *)MBOX1_WRITE) = request;
	do {
		SYNC;
		while (*((volatile uint32_t *)MBOX0_STATUS) == MBOX_STATUS_EMPTY);
		response = *((volatile uint32_t *)MBOX0_READ);
	} while ((response & 0xf) != 8);

	//arm_mem_req = (request_response_t *)(response & ((~0) << 4));
	linelen = (arm_mem_req->tag_msg_len) & 0x7fffffff;
	//if (linelen > 1024) linelen = 1024;
	linelen = 1024;
	for (i=0; i<linelen-1; i++) line[i] = arm_mem_req->tag_buff.byte_buff[i];
	line[linelen-1] = '\0';
}

void rpi2_set_vectors()
{
	int i;
	unsigned int v;
#ifdef DEBUG_EXCEPTIONS
	uint32_t tmp;
#endif
	rpi2_upper_vec_address = (uint32_t)&__hivec;
	rpi2_set_upper_vectable();
	// set hivec address
	// MCR p15, 0, <Rt>, c12, c0, 0 ; Write Rt to VBAR
	asm volatile (
			"mcr p15, 0, %[reg], c12, c0, 0\n\t"
			"dsb\n\t"
			"isb\n\t"
			::[reg] "r" (rpi2_upper_vec_address):
	);
	rpi2_set_lower_vectable(); // write exception vectors

	if (rpi2_uart0_excmode == RPI2_UART0_FIQ)
	{
		((uint32_t *)rpi2_upper_vec_address)[14] = (uint32_t)(&rpi2_reroute_exc_irq);
		((uint32_t *)rpi2_upper_vec_address)[15] = (uint32_t)(&rpi2_fiq_handler);
	}

	v = (unsigned int) rpi2_upper_vec_address;
	for (i=0; i< 16; i++)
	{
		rpi2_flush_address(v);
		v += 4;
	}
	v = 0;
	for (i=0; i< 16; i++)
	{
		rpi2_flush_address(v);
		v += 4;
	}

#ifdef DEBUG_EXCEPTIONS
	serial_raw_puts("\r\nset vecs\r\n");
	serial_raw_puts("up vec: ");
	util_word_to_hex(dbg_tmp_buff, (unsigned int) rpi2_upper_vec_address);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts(" up ser: ");
	util_word_to_hex(dbg_tmp_buff, (unsigned int) (* ((uint32_t *)(rpi2_upper_vec_address + 32+24))));
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts(" vbar: ");
	asm volatile (
			"mrc p15, 0, %[reg], c12, c0, 0\n\t"
			"dsb\n\t"
		::[reg] "r" (tmp):
	);
	util_word_to_hex(dbg_tmp_buff, (unsigned int) tmp);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts("\r\n");
#endif

}

// now that we have write-through caches, we don't need to
// clean, but just invalidate
void rpi2_flush_address(unsigned int addr)
{
	if (rpi2_use_mmu)
	{
		asm volatile ("mcr p15, 0, %0, c7, c6, 1\n\t" :: "r" (addr)); // DCIMVAC
		asm volatile ("dsb\n\tisb\n\t" ::: "memory");
		asm volatile ("mcr p15, 0, %0, c7, c5, 1\n\t" :: "r" (addr)); // ICIMVAU
		asm volatile ("dsb\n\tisb\n\t" ::: "memory");
		asm volatile ("mcr p15, 0, %0, c7, c5, 7\n\t" :: "r" (addr)); // BPIMVA
		asm volatile ("dsb\n\tisb\n\t" ::: "memory");
	}
}

void rpi2_invalidate_caches()
{
	uint32_t loc, tmp, i, j, k, cssidr, ctype;
	uint32_t num_sets, num_assoc;
	uint32_t set_offset, way_offset, level_offset;

	if (!rpi2_use_mmu) return;
	// disable caches
	/*
	asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" : "=r" (tmp) :: "memory");
	tmp &= ~(3 << 11); // I-cache and branch predictor
	tmp &= ~4; // D-cache
	asm volatile ("mcr p15, 0, %0, c1, c0, 0\n\t" :: "r" (tmp) : "memory");
	asm volatile ("dsb\n\tisb\n\t" ::: "memory");
	 */
	asm volatile ("mrc p15, 1, %0, c0, c0, 1\n\t" : "=r" (tmp)); // CLIDR
	loc = (tmp >> 24) & 0x7;
	for (i = 0; i <= loc; i++) // for each cache level until LoC
	{
		ctype = tmp >> (i*3);
		if (ctype & 0x6) // if data or unified cache
		{
			// select cache to check
			tmp = (i<<1) | 0; // InD = data/unified
			asm volatile ("mcr p15, 2, %0, c0, c0, 0\n\t" :: "r" (tmp)); // CSSELR
			asm volatile ("dsb\n\tisb\n\t");
			// get cache info
			asm volatile ("mrc p15, 1, %0, c0, c0, 0\n\t" : "=r" (cssidr)); // CSSIDR
			num_sets = ((cssidr >> 13) & 0x7fff) + 1;
			num_assoc = ((cssidr >> 3) & 0x3ff) + 1;
			level_offset = 1;
			set_offset = (cssidr & 7) + 2; // line_size_bits
			way_offset = 31 - util_num_bits(num_assoc);
			for (j = 0; j < num_sets; j++)
			{
				for (k = 0; k < num_assoc; k++)
				{
					tmp = i << level_offset;
					tmp |= j << set_offset;
					tmp |= k << way_offset;
					asm volatile ("mcr p15, 0, %0, c7, c10, 2\n\t" :: "r" (tmp)); // DCCSW(tmp)
				}
			}
		}
		if (ctype & 0x1) // if instruction cache
		{
			asm volatile ("mcr p15, 0, r0, c7, c5, 0\n\t"); // ICIALLU(ignored);
		}
	}
	// branch predictor
	asm volatile ("mcr p15, 0, r0, c7, c5, 6\n\t"); // BPIALL(ignored);
	asm volatile ("dsb\n\tisb\n\t" ::: "memory");

	// enable caches
	/*
	asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" : "=r" (tmp) :: "memory");
	tmp |= (3 << 11); // I-cache and branch predictor
	tmp |= 4; // D-cache
	asm volatile ("mcr p15, 0, %0, c1, c0, 0\n\t" :: "r" (tmp) : "memory");
	asm volatile ("dsb\n\tisb\n\t" ::: "memory");
	*/
}

// should only be called from rpi2_enable_mmu()
void rpi2_invalidate_tlbs()
{
	uint32_t tmp;

	asm volatile ("mrc p15, 0, %0, c0, c0, 3\n\t" : "=r" (tmp)); // TLBTR
	if (tmp & 1) // 0 = unified
	{
		// invalidate istr TLB
		asm volatile ("mcr p15, 0, r0, c8, c5, 0\n\t"); // ITLBIALL(ignored)
		// invalidate data TLB
		asm volatile ("mcr p15, 0, r0, c8, c6, 0\n\t"); // DTLBIALL(ignored)
	}
	else
	{
		//invalidate unified TLB
		asm volatile ("mcr p15, 0, r0, c8, c7, 0\n\t"); // TLBIALL(ignored)
	}

	asm volatile ("dsb\n\tisb\n\t" ::: "memory");
}

// enable MMU and caches
void rpi2_enable_mmu()
{
	uint32_t tmp;
	uint32_t ramsz;
	uint32_t ramstart;

	ramsz = (uint32_t)rpi2_arm_ramsize;
	ramsz >>= 20; // bytes to megs
	ramstart = (uint32_t)rpi2_arm_ramstart;
	ramstart >>= 20; // bytes to megs
	// disable MMU
	tmp = 0;
	asm volatile ("mcr p15, 0, %0, c1, c0, 0\n\t" :: "r" (tmp) : "memory");
#if 0
	// program RAM
	for (tmp=0; tmp<0x3b0; tmp++)
	{
		master_xlat_tbl[tmp] = MMU_SECT_ENTRY(tmp, MMU_SECT_ATTR_NORMAL);
	}
	// video RAM (?)
	for (tmp=0x3b0; tmp<0x3f0; tmp++)
	{
		master_xlat_tbl[tmp] = MMU_SECT_ENTRY(tmp, MMU_SECT_ATTR_DEV);
	}
#else
	// program RAM
	for (tmp=ramstart; tmp<ramsz; tmp++)
	{
		master_xlat_tbl[tmp] = MMU_SECT_ENTRY(tmp, MMU_SECT_ATTR_NORMAL);
	}
	// video RAM (?)
	for (tmp=ramsz; tmp<0x3f0; tmp++)
	{
		master_xlat_tbl[tmp] = MMU_SECT_ENTRY(tmp, MMU_SECT_ATTR_DEV);
	}
#endif
	// peripherals
	for (tmp=0x3f0; tmp<0xfff; tmp++)
	{
		master_xlat_tbl[tmp] = MMU_SECT_ENTRY(tmp, MMU_SECT_ATTR_DEV);
	}

	// set SMP bit in ACTLR - needed, otherwise caches are disabled
	asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t" : "=r" (tmp));
	tmp |= 1 << 6;
	asm volatile ("mcr p15, 0, %0, c1, c0, 1\n\t" :: "r" (tmp));

	// set domain 0 to client (DACR)
	asm volatile ("mcr p15, 0, %0, c3, c0, 0\n\t" :: "r" (1));

	// always use TTBR0 (TTBCR)
	asm volatile ("mcr p15, 0, %0, c2, c0, 2\n\t" :: "r" (0));

	// set TTBR0 (page table walk inner and outer non-cacheable, non-shareable memory)
	tmp = MMU_VAL_TTBR0((uint32_t)master_xlat_tbl, 0);
	asm volatile ("mcr p15, 0, %0, c2, c0, 0\n\t" :: "r" (tmp));

	//rpi2_invalidate_caches();
	rpi2_invalidate_tlbs();

	// enable MMU, caches and branch prediction in SCTLR
	// asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" : "=r" (tmp));
	// tmp |= 0x1805;
	tmp = 0x1805;
	asm volatile ("mcr p15, 0, %0, c1, c0, 0\n\t" :: "r" (tmp) : "memory");
	asm volatile ("dsb\n\tisb\n\t" ::: "memory");

	rpi2_invalidate_caches();
}

void rpi2_init()
{
	int i;
	uint32_t *p;
	uint32_t tmp1, tmp2;
	uint32_t *tmp3;
	unsigned int cpsr_store;

	exception_info = RPI2_EXC_NONE;
	exception_extra = 0;
	rpi2_pabt_reroute = 0;
	rpi2_sigint_flag = 0;
	rpi2_ctrl_c_flag = 0;
	rpi2_dgb_enabled = 0; // disable use of gdb trap handler
	rpi2_debuggee_running = 0;
	gdb_dyn_debug = 0;
	rpi2_arm_ramsize = rpi2_get_arm_ram(&rpi2_arm_ramstart);
	rpi2_uart_clock = rpi2_get_clock(CLOCK_UART);

#if 0
	serial_enable_ctrlc();
#endif

	/* calculate debug register file base address */
	/*
	 * get DBGDRAR
	 * MRC p14, 0, <Rt>, c1, c0, 0
	 * MRC<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
	 * MCR<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
	 * for debug registers opc1 is always zero(?)
	 * (compare with MRC p14, 0, <Rt>, c2, c0, 0 DBGDSAR)
	 * mask off 12 lsbs to get the base address
	 * get entry for the component (end of ROM table is blank entry: 0x00000000)
	 *   ROM table is always 4kB long, Offset 0xf00 - 0xfcb is always reserved
	 *   Last possible component entry is 0xefc
	 * mask off 12 lsbs of the component entry to get the offset (can be negative)
	 * component base = base address + component offset
	 *   (The Address offset field of a ROM Table entry points to the start of the
	 *   last 4KB block of the address space of the component)
	 * get the Peripheral ID4 Register for the component (component base + 0xfd0)
	 * Extract the 4KB count field, bits [7:4], from the value of the Peripheral ID4 Register
	 * Use the 4KB count value to calculate the start address of the address space for the component
	 */

	asm volatile (
			"mrc p14, 0, %[retreg], c1, c0, 0 @ get DBGDRAR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp1)::
	);

	// calculate debug registers base address from debug ROM address (DBGRAR)
	tmp1 &= 0xfffff000; // debug ROM address
	dbg_rom_base = tmp1;
	tmp3 = (uint32_t *) tmp1; // to pointer
	tmp2 = *tmp3; // get first debug peripheral entry
	asm volatile("dsb\n\t");
	tmp2 &= 0xfffff000; // get the base address offset (from debug ROM)
	dbg_reg_base = (tmp1 + tmp2);

	// dbg register lock
	tmp1 = *((uint32_t *) (dbg_reg_base + 0xfb4)); // DBGLSR
	SYNC;
	if (tmp1 & 2) // locked
	{
		// unlock
		*((uint32_t *) (dbg_reg_base + 0xfb0)) = 0xC5ACCE55; // DBGLAR
		SYNC;
	}

	// double lock
	asm volatile (
			"mrc p14, 0, %[retreg], c1, c3, 4 @ get DBGOSDLR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp1)::
	);

	if (tmp1 & 1) // double lock on
	{
		asm volatile (
				"mcr p14, 0, %[retreg], c1, c1, 4 @ set DBGOSLSR\n\t"
				"dsb\n\t"
				:: [retreg] "r" (0):
		);
		SYNC;
	}

	// OS lock
	asm volatile (
			"mrc p14, 0, %[retreg], c1, c1, 4 @ get DBGOSLSR\n\t"
			"dsb\n\t"
			: [retreg] "=r" (tmp1)::
	);
	SYNC;
	if (tmp1 & 2) // OS lock set
	{
		tmp2 = *((uint32_t *) (dbg_reg_base + 0x024)); // DBGECR
		if (tmp2 & 1) // unlock event enabled
		{
			// disable unlock event
			*((uint32_t *) (dbg_reg_base + 0x024)) = 0; // DBGECR
			SYNC;
		}
		// release OS lock
		asm volatile (
				"mcr p14, 0, %[retreg], c1, c0, 4 @ set DBGOSLAR\n\t"
				:: [retreg] "r" (0):
		);
		SYNC;
	}

	cpsr_store = rpi2_disable_save_ints();
	rpi2_set_vectors();
	if (rpi2_use_mmu)
	{
		rpi2_enable_mmu();
		rpi2_invalidate_caches();
	}
	rpi2_restore_ints(cpsr_store);

#if 1
	// set debug monitor mode
	asm volatile ("mrc p14, 0, %[val], c0, c2, 2\n\t" :[val] "=r" (tmp1) ::);
	tmp1 |= (1 << 15); // MDBGen
	asm volatile ("mcr p14, 0, %[val], c0, c2, 2\n\t" ::[val] "r" (tmp1) :);
	SYNC;
#endif

}

#if 0
/*
 * for restart situations - maybe
 */

void rpi2_init_led()
{
	uint32_t tmp; // scratchpad
	uint32_t tmp2; // scratchpad

	// Setup the GPIO pin 47 (ACT-led)
	// Setup the GPIO pin 47 (ACT-led)

	// Change disable pull up/down & delay for 150 cycles
	*((volatile uint32_t *)GPPUD) = 0x00000000; // pull-down disabled for pin 47
	delay(150);

	// Target pull up/down change to pin 47 & delay for 150 cycles.
	*((volatile uint32_t *)GPPUDCLK1) = (1 << 15);
	delay(150);

	// Remove pull up/down change
	*((volatile uint32_t *)GPPUD) = 0x00000000;

	// Write 0 to GPPUDCLK1 to 'un-target' the pull up/down change
	*((volatile uint32_t *)GPPUDCLK1) = 0x00000000;

	// set led off by default
	*((volatile uint32_t *)GPIO_CLRREG1) = (1 << 15);

	// GPIO 47 to output
	tmp = *((volatile uint32_t *)GPFSEL4);
	tmp &= ~(7 << 21);
	tmp |= (1 << 21);
	/*
	//tmp2 = 7 << 21;
	tmp2 = 0x00e00000;
	tmp &= ~(tmp2); // clear the functions for pin 47
	//tmp2 = 1 << 21;
	tmp2 = 0x00200000;
	tmp |= (tmp2); // pin 47 to output
	*/
	*((volatile uint32_t *)GPFSEL4) = tmp;

	// set led off by default
	//*((volatile uint32_t *)GPIO_CLRREG1) = (1 << 15);
}
#endif

void rpi2_led_off()
{
	*((volatile uint32_t *)GPIO_CLRREG1) = (1<<15);
}

void rpi2_led_on()
{
	*((volatile uint32_t *)GPIO_SETREG1) = (1 << 15);
}

void rpi2_delay_loop(unsigned int delay_ms)
{
	int i,j;
#if 1
	uint32_t tm_start, tm_curr, tm_period, tm_delay;
	volatile uint32_t *tmr;

	if (delay_ms == 0) return;
	tmr = (volatile uint32_t *)SYSTMR_CLO; // default: 1MHZ timer
	tm_period = 0;
	tm_delay = (uint32_t)(delay_ms * 1000);
	tm_start = tm_curr = *tmr;
	while (tm_period < tm_delay)
	{
		tm_curr = *tmr;

		// if tm_curr >= tm_start, tm_period = tm_curr - tm_start
		// if tm_curr < tm_start (timer wraparound)
		// 1<<32 - x = -x (with 32-bit numbers):
		//(1<<32) - (tm_start - tm_curr) = tm_curr - tm_start
		tm_period = (uint32_t)(tm_curr - tm_start);
	}
#else
	for (i=0; i< delay_ms; i++)
	{
		// delay 1 ms
		for (j=0; j<700; j++)
//		for (j=0; j<1400; j++)
		{
			asm volatile("nop\n\t");
		}
	}
#endif
}

void rpi2_led_blink(unsigned int on_ms, unsigned int off_ms, unsigned int count)
{
	uint32_t i,j;

	for (i=0; i<count; i++)
	{
		rpi2_led_on();
		rpi2_delay_loop(on_ms);
		rpi2_led_off();
		rpi2_delay_loop(off_ms);
	}
}
