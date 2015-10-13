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
extern int serial_raw_puts(char *str); // used for debugging - may be removed
extern void serial_set_ctrlc(void *handler); // used for debugging
extern char __hivec;

// Another naughty trick - allocates
// exception_info, exception_extra and rpi2_reg_context
// This way definitions and extern-declarations don't clash
#define extern
#include "rpi2.h"
#undef extern

#define RPI_DOUBLE_VECTORS

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

extern void gdb_trap_handler(); // TODO: make callback
extern uint32_t jumptbl;
uint32_t dbg_reg_base;
uint32_t rpi2_dgb_enabled; // flag to pabt that this is IRQ-case
volatile uint32_t rpi2_ctrl_c_flag;
volatile uint32_t rpi2_irq_status;
volatile uint32_t rpi2_irq_flag; // flag to pabt that this is IRQ-case
volatile uint32_t rpi2_exc_reason;
uint32_t rpi2_upper_vec_address;
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
void rpi2_dabt_handler() __attribute__ ((naked));
void rpi2_irq_handler() __attribute__ ((naked));
void rpi2_firq_handler() __attribute__ ((naked));
void rpi2_pabt_handler() __attribute__ ((naked));

// exception re-routing
void rpi2_unhandled_irq() __attribute__ ((naked));
void rpi2_unhandled_pabt() __attribute__ ((naked));
void rpi2_reroute_reset() __attribute__ ((naked));
void rpi2_reroute_exc_und() __attribute__ ((naked));
void rpi2_reroute_exc_svc() __attribute__ ((naked));
void rpi2_reroute_exc_dabt() __attribute__ ((naked));
void rpi2_reroute_exc_aux() __attribute__ ((naked));
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
}

unsigned int rpi2_disable_save_ints()
{
	uint32_t status;
	asm volatile (
			"mrs %[var_reg], cpsr\n\t"
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
 * Other exceptions are handled line non-serial interrupt.
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
			".word rpi2_dabt_handler\n\t"
			".word rpi2_aux_handler @ used in some cases\n\t "
			".word rpi2_unhandled_irq\n\t"
			".word rpi2_firq_handler\n\t"
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
	);
}

void rpi2_set_upper_vectable()
{
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
			".word rpi2_reroute_exc_dabt\n\t"
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
			"str r1, [r0, #4*16]\n\t"
			"str lr, [r0, #4*15]\n\t"

			"@ mode specifics\n\t"
			"mov r2, #0xf @ all valid modes have bit 4 set\n\t"
			"and r1, r2\n\t"

			"@ our own mode?\n\t"
			"mrs r3, cpsr\n\t"
			"and r3, r2\n\t"
			"cmp r1, r3\n\t"
			"bne 10f @ not our own mode\n\t"

			"mov r1, sp\n\t"
			"mov r2, lr\n\t"
			"mrs r3, spsr\n\t"
			"b 9f\n\t"

			"10:\n\t"
			"cmp r1, #0x00 @ USR\n\t"
			"cmpne r1, #0x0f @ SYS\n\t"
			"bne 1f\n\t"
			"@ USR/SYS\n\t"
			"mrs r1, SP_usr\n\t"
			"mrs r2, LR_usr\n\t"
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
			"add r1, r0, #4*8\n\t"
			"stmia r1, {r2 - r6} @ overwrite r8 - 12 with banked regs\n\t"
			"mrs r1, SP_fiq\n\t"
			"mrs r2, LR_fiq\n\t"
			"mrs r3, SPSR_fiq\n\t"
			"b 9f\n\t"
			"2: cmp r1, #0x02 @ IRQ\n\t"
			"bne 3f\n\t"
			"@ IRQ\n\t"
			"mrs r1, SP_irq\n\t"
			"mrs r2, LR_irq\n\t"
			"mrs r3, SPSR_irq\n\t"
			"b 9f\n\t"
			"3: cmp r1, #0x03 @ SVC\n\t"
			"bne 4f\n\t"
			"@ SVC\n\t"
			"mrs r1, SP_svc\n\t"
			"mrs r2, LR_svc\n\t"
			"mrs r3, SPSR_svc\n\t"
			"b 9f\n\t"
			"4: cmp r1, #0x06 @ MON\n\t"
			"bne 5f\n\t"
			"@ MON\n\t"
			"mrs r1, SP_mon\n\t"
			"mrs r2, LR_mon\n\t"
			"mrs r3, SPSR_mon\n\t"
			"b 9f\n\t"
			"5: cmp r1, #0x07 @ ABT\n\t"
			"bne 6f\n\t"
			"@ ABT\n\t"
			"mrs r1, SP_abt\n\t"
			"mrs r2, LR_abt\n\t"
			"mrs r3, SPSR_abt\n\t"
			"b 9f\n\t"
			"6: cmp r1, #0x0a @ HYP\n\t"
			"bne 7f\n\t"
			"@ HYP\n\t"
			"mrs r1, SP_hyp\n\t"
			"mrs r2, ELR_hyp\n\t"
			"mrs r3, SPSR_hyp\n\t"
			"b 9f\n\t"
			"7: cmp r1, #0x0b @ UND\n\t"
			"bne 8f\n\t"
			"@ UND\n\t"
			"mrs r1, SP_und\n\t"
			"mrs r2, LR_und\n\t"
			"mrs r3, SPSR_und\n\t"
			"b 9f\n\t"
			"8: @ ILLEGAL MODE\n\t"
			"udf\n\t"
			"9: \n\t"

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
			"@ mode to return to\n\t"
			"mov r2, #0xf @ all valid modes have bit 4 set\n\t"
			"and r1, r2\n\t"

			"@ our own mode\n\t"
			"mrs r3, cpsr\n\t"
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
			"b 9f\n\t"
			"1: cmp r1,  #0x01 @ FIQ\n\t"
			"bne 2f\n\t"
			"@ FIQ\n\t"
			"msr SP_fiq, r2\n\t"
			"msr LR_fiq, r3\n\t"
			"msr SPSR_fiq, r4\n\t"
			"add r1, r0, #4*8\n\t"
			"ldmia r1, {r2 - r6} @ banked regs\n\t"
			"msr r8_fiq, r2\n\t"
			"msr r9_fiq, r3\n\t"
			"msr r10_fiq, r4\n\t"
			"msr r11_fiq, r5\n\t"
			"msr r12_fiq, r6\n\t"
			"b 9f\n\t"
			"2: cmp r1, #0x02 @ IRQ\n\t"
			"bne 3f\n\t"
			"@ IRQ\n\t"
			"msr SP_irq, r2\n\t"
			"msr LR_irq, r3\n\t"
			"msr SPSR_irq, r4\n\t"
			"b 9f\n\t"
			"3: cmp r1, #0x03 @ SVC\n\t"
			"bne 4f\n\t"
			"@ SVC\n\t"
			"msr SP_svc, r2\n\t"
			"msr LR_svc, r3\n\t"
			"msr SPSR_svc, r4\n\t"
			"b 9f\n\t"
			"4: cmp r1, #0x06 @ MON\n\t"
			"bne 5f\n\t"
			"@ MON\n\t"
			"msr SP_mon, r2\n\t"
			"msr LR_mon, r3\n\t"
			"msr SPSR_mon, r4\n\t"
			"b 9f\n\t"
			"5: cmp r1, #0x07 @ ABT\n\t"
			"bne 6f\n\t"
			"@ ABT\n\t"
			"msr SP_abt, r2\n\t"
			"msr LR_abt, r3\n\t"
			"msr SPSR_abt, r4\n\t"
			"b 9f\n\t"
			"6: cmp r1, #0x0a @ HYP\n\t"
			"bne 7f\n\t"
			"@ HYP\n\t"
			"msr SP_hyp, r2\n\t"
			"msr ELR_hyp, r3\n\t"
			"msr SPSR_hyp, r4\n\t"
			"b 9f\n\t"
			"7: cmp r1, #0x0b @ UND\n\t"
			"bne 8f\n\t"
			"@ UND\n\t"
			"msr SP_und, r2\n\t"
			"msr LR_und, r3\n\t"
			"msr SPSR_und, r4\n\t"
			"b 9f\n\t"
			"8: @ ILLEGAL MODE\n\t"
			"udf\n\t"
			"9: \n\t"

			"ldr r2, [r0, #4*15] @ return address\n\t"
			"ldr r1, [r0] @ r0-value\n\t"
			"push {r1, r2} @ prepare for return\n\t"
			"ldmia r0, {r0 - r12}\n\t"
			"pop {r0, lr}\n\t"
			"dsb\n\t"
			"isb\n\t"
	);
}

// exception handlers

// debug print for naked exception functions
void naked_debug_2(uint32_t src, uint32_t link, uint32_t stack)
{
	char tmp[16];
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

// common trap handler outside exception handlers
void rpi2_trap_handler()
{
#ifdef DEBUG_EXCEPTIONS
	serial_raw_puts("\r\ntrap_hndlr\r\n");
#endif

	// IRQs need to be enabled for serial I/O
	if (rpi2_dgb_enabled)
	{
		rpi2_enable_ints();
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
			"mov pc, #0 @ jump tp low vector\n\t"
	);
}

void rpi2_reroute_exc_und()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif

	asm volatile(
			"mov pc, #4 @ jump tp low vector\n\t"
	);
}

void rpi2_reroute_exc_svc()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #8 @ jump tp low vector\n\t"
	);
}

void rpi2_reroute_exc_dabt()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #16 @ jump tp low vector\n\t"
	);
}

void rpi2_reroute_exc_aux()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #20 @ jump tp low vector\n\t"
	);
}

void rpi2_reroute_exc_fiq()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile(
			"mov pc, #28 @ jump tp low vector\n\t"
	);
}



// The exception handlers
// The exception handlers are split in two parts: the primary and secondary
// The primaries are attributed as 'naked'so that C function prologue
// doesn't disturb the stack before the exception stack frame is pushed
// into the stack, because the prologue would change the register values, and
// we would return from exception before we get to the C function epilogue,
// and we'd return with unbalanced stack.
// Naked functions shouldn't have any C-code in them.
//
// The secondary does the actual work

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
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
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

	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #0 @ RPI2_EXC_RESET\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

			"rst_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_UNDEF
void rpi2_undef_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	static char scratchpad[16]; // scratchpad
	char *p;
	int i;
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
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
	rpi2_dump_context(&rpi2_reg_context);
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
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
			"sub lr, #4 @ on return, skip the instruction\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, und_sp_store\n\t"
			"ldr r1, =rpi2_reg_context\n\t"
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
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

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
	uint32_t tmp;
	char *p;
	static char scratchpad[16]; // scratchpad
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
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
	rpi2_dump_context(&rpi2_reg_context);
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
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, svc_sp_store\n\t"
			"ldr r1, =rpi2_reg_context\n\t"
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
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

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
	uint32_t tmp;
	char *p;
	static char scratchpad[16]; // scratchpad
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
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
	rpi2_dump_context(&rpi2_reg_context);
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
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_svc_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, aux_sp_store\n\t"
			"ldr r1, =rpi2_reg_context\n\t"
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
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

			"aux_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_DABT
void rpi2_dabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	static char scratchpad[16]; // scratchpad
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	// rpi2_reg_context.reg.r15 -= 8; // PC
	rpi2_reg_context.reg.r15 -= 4; // skip bad instruction

	p = "\r\nDABT EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_reg_context);
}
#endif

void rpi2_dabt_handler()
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
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_dabt_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
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
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

			"dabt_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_IRQ
void rpi2_irq_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad

	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);
	// fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	// rpi2_irq_context.reg.r15 -= 4; // PC


#if 0
	// IRQ2 pending
	if (*((volatile uint32_t *)IRC_PENDB) & (1<<19))
	{
		// UART IRQ
		if (*((volatile uint32_t *)IRC_PEND2) & (1<<25))
		{
			//rpi2_led_blink(200, 200, 8);
			//serial_irq();
		}
		else
		{
			// unhandled IRQ
			p = "\r\nIRQ EXCEPTION\r\n";
			do {i = serial_raw_puts(p); p += i;} while (i);

			serial_raw_puts("exc_addr: ");
			util_word_to_hex(scratchpad, exc_addr);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\nSPSR: ");
			util_word_to_hex(scratchpad, exc_cpsr);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\nSP: ");
			util_word_to_hex(scratchpad, stack_frame_addr);
			serial_raw_puts(scratchpad);
			tmp = *((volatile uint32_t *)IRC_PEND2);
			serial_raw_puts("\r\nPEND2: ");
			util_word_to_hex(scratchpad, tmp);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\n");
			rpi2_dump_context(&rpi2_irq_context);
			exception_info = RPI2_EXC_IRQ;
			rpi2_set_exc_reason(RPI2_REASON_HW_EXC);
		}
	}
	else
	{
		// unhandled IRQ
		p = "\r\nIRQ EXCEPTION\r\n";
		do {i = serial_raw_puts(p); p += i;} while (i);

		serial_raw_puts("exc_addr: ");
		util_word_to_hex(scratchpad, exc_addr);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\nSPSR: ");
		util_word_to_hex(scratchpad, exc_cpsr);
		serial_raw_puts(scratchpad);
		tmp = *((volatile uint32_t *)IRC_PENDB);
		serial_raw_puts("\r\nPENDB: ");
		util_word_to_hex(scratchpad, tmp);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
		rpi2_dump_context(&rpi2_irq_context);
		exception_info = RPI2_EXC_IRQ;
		rpi2_set_exc_reason(RPI2_REASON_HW_EXC);

	}
#else
	p = "\r\nIRQ EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSP: ");
	util_word_to_hex(scratchpad, stack_frame_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	tmp = *((volatile uint32_t *)IRC_PENDB);
	serial_raw_puts("\r\nPENDB: ");
	util_word_to_hex(scratchpad, tmp);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	//rpi2_dump_context(&rpi2_irq_context);
#endif

#if 0
	// if crtl-C, dump exception info (for now)
	if (rpi2_ctrl_c_flag)
	{
		// swap flags to stop further interrupts on one ctrl-C
		rpi2_ctrl_c_flag = 0;
		rpi2_irq_flag = 1;
		rpi2_set_exc_reason(RPI2_REASON_SIGINT);
		exception_info = RPI2_EXC_IRQ;

#ifdef DEBUG_REG_CONTEXT
		serial_raw_puts("\r\nCTRL-C\r\n");
		serial_raw_puts("exc_addr: ");
		util_word_to_hex(scratchpad, exc_addr);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\nSPSR: ");
		util_word_to_hex(scratchpad, exc_cpsr);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
		rpi2_dump_context(&rpi2_irq_context);
#endif
	}
#endif
}
#endif

void rpi2_unhandled_irq()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, irq_sp_store\n\t"
			"movw sp, #:lower16:__irq_stack\n\t"
			"movt sp, #:upper16:__irq_stack\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_irq_context\n\t"
			"sub lr, #4 @ fix return address\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, irq_sp_store\n\t"
			"ldr r1, =rpi2_irq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1:\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #6 @ RPI2_EXC_IRQ\n\t"
			"str r1, [r0]\n\t"

			"ldr r0, =rpi2_exc_reason\n\t"
			"mov r1, #10 @ RPI2_REASON_HW_EXC\n\t"
			"str r1, [r0]\n\t"

			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"

			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

			"irq_sp_store: @ close enough for simple pc relative\n\t"
			".int 0\n\t"
	);
}

void rpi2_irq_handler()
{
	asm volatile (
			"str sp, irq_sp_store\n\t"
			"movw sp, #:lower16:__irq_stack\n\t"
			"movt sp, #:upper16:__irq_stack\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"push {r0, r1, lr} @ r1 to keep sp double word aligned\n\t"

			"movw r0, #0xb200 @ IRC_PENDB\n\t"
			"movt r0, #0x3f00\n\t"
			"ldr r1, [r0]\n\t"
			"ands r1, #0x80000 @ bit 19 = UART0\n\t"
			"beq rpi2_other_irqs\n\t"

			"@ UART0 IRQ involved\n\t"
			"bl serial_irq\n\t"
#ifdef DEBUG_IRQ
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_irq_handler2\n\t"
#endif
			"ldr r0, =rpi2_ctrl_c_flag\n\t"
			"ldr r1, [r0]\n\t"
			"cmp r1, #0\n\t"
			"bne 1f @ sigint happened\n\t"
			"@ can't do both ctrl-c and other IRQs so ctrl-c\n\t"
			"@ is of higher priority\n\t"

			"@ any other pending IRQs?\n\t"
			"movw r0, #0xb200 @ IRC_PENDB\n\t"
			"movt r0, #0x3f00\n\t"
			"ldr r1, [r0]\n\t"
			"mvn r2, #0x80000 @ bit 19 = UART0\n\t"
			"ands r1, r2\n\t"
			"bne rpi2_other_irqs\n\t"

			"@ Only UART0 IRQ\n\t"
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, irq_sp_store\n\t"
			"subs pc, lr, #4 @ return\n\t"

			"1: @ SIGINT\n\t"
	);
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"sub lr, #4 @ the return address fix"
			"pop {r0 - r12}\n\t"
			"push {r12}\n\t"
			"ldr r12, =rpi2_irq_context\n\t"
	);
// store processor context
write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 2f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, irq_sp_store\n\t"
			"ldr r1, =rpi2_irq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"2: @ context stored, handle SIGINT\n\t"
			"mov r1, #0\n\t"
			"ldr r0, =rpi2_ctrl_c_flag\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #6 @ RPI2_EXC_IRQ\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_exc_reason\n\t"
			"mov r1, #2 @ RPI2_REASON_SIGINT\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

			"rpi2_other_irqs:\n\t"
	);
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"pop {r0, r1, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, irq_sp_store\n\t"
			"mov pc, #24 @ IRQ, low vector\n\t"

			".ltorg @ literal pool\n\t"
	);
}

#ifdef DEBUG_FIQ
void rpi2_firq_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	rpi2_reg_context.reg.r15 -= 4; // PC

	p = "\r\nFIQ EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_reg_context);
}
#endif

void rpi2_firq_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	// store processor context
	asm volatile (
			"str sp, fiq_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__fiq_stack\n\t"
			"movt sp, #:upper16:__fiq_stack\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_fiq_context\n\t"
	);
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, fiq_sp_store\n\t"
			"ldr r1, =rpi2_fiq_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1: @ context stored\n\t"
	);

	// rpi2_firq_handler2() // - No C in naked function
#ifdef DEBUG_FIQ
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_firq_handler2\n\t"
	);
#endif
	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #7 @ RPI2_EXC_FIQ\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr lr, =rpi2_trap\n\t"
			"mrs r0, spsr\n\t"
			"orr r0, #8 @ interrupt mask\n\t"
			"msr spsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"subs pc, lr, #0\n\t"

			"fiq_sp_store:\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_pabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	int i;
	uint32_t *p;
	uint32_t exc_cpsr;
	uint32_t dbg_state;
	uint32_t ifsr;
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
			:[var_reg] "=r" (exc_cpsr) ::
	);

	if (rpi2_irq_flag) // leave the flag for gdb_trap_handler()
	{
		switch (exception_info)
		{
		case RPI2_EXC_RESET:
			ctx = &rpi2_svc_context;
			rpi2_irq_flag = 0;
			break;
		case RPI2_EXC_UNDEF:
			ctx = &rpi2_svc_context;
			rpi2_irq_flag = 0;
			break;
		case RPI2_EXC_SVC:
			ctx = &rpi2_svc_context;
			rpi2_irq_flag = 0;
			break;
		case RPI2_EXC_PABT:
			ctx = &rpi2_pabt_context;
			rpi2_irq_flag = 0;
			break;
		case RPI2_EXC_DABT:
			ctx = &rpi2_dabt_context;
			rpi2_irq_flag = 0;
			break;
		case RPI2_EXC_AUX:
			ctx = &rpi2_svc_context;
			rpi2_irq_flag = 0;
			break;
		case RPI2_EXC_IRQ:
			ctx = &rpi2_irq_context;
			break;
		case RPI2_EXC_FIQ:
			ctx = &rpi2_fiq_context;
			rpi2_irq_flag = 0;
			break;
		default:
			ctx = &rpi2_reg_context;
			rpi2_irq_flag = 0;
			break;
		}
		// copy irq context to reg context for returning
		// to original program that was interrupted by CTRL-C
		for (i=0; i<18; i++)
		{
			rpi2_reg_context.storage[i] = ctx->storage[i];
		}
		//rpi2_reg_context.reg.cpsr &= ~(1<<7); // clear irq mask
		// serial_raw_puts("\r\nIRQ_PABT\r\n");
	}

	// fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	// rpi2_reg_context.reg.r15 -= 4; // PC


#if 0
	// check debug state
	// MRC p14, 0, <Rt>, c0, c1, 0 // DBGDSCRint
	asm volatile
	(
			"mrc p14, 0, %[var_reg], c0, c1, 0 @ DBGDSCRint\n\t"
			:[var_reg] "=r" (dbg_state) ::
	);
#else
	// all bits are not reliable if read via CP14 interface
	p = (uint32_t *)(dbg_reg_base + 0x88); // DBGDSCRext
	dbg_state = *p;
#endif

	// MRC p15, 0, <Rt>, c5, c0, 1 // Read IFSR (exception cause)
	asm volatile
	(
			"mrc p15, 0, %[var_reg], c5, c0, 1 @ IFSR\n\t"
			:[var_reg] "=r" (ifsr) ::
	);

#ifdef DEBUG_PABT
	// raw puts, because we are here with interrupts disabled
	pp = "\r\nPABT EXCEPTION\r\n";
	do {i = serial_raw_puts(pp); pp += i;} while (i);

	serial_raw_puts("exc_addr: ");
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
	serial_raw_puts("\r\n");
	rpi2_dump_context(&rpi2_reg_context);
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
	}
	*/

	p = (uint32_t *)(dbg_reg_base + 0x88); // DBGDSCRext
	dbg_state = *p;
	/*
	while (!(dbg_state & 0x2)) // while not 'restarted'
	{
		dbg_state = *p;
	}
	*/
	for (i=0; i<100000; i++)
	{
		dbg_state = *p;
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
	//exception_info = RPI2_EXC_PABT;

	if (ifsr & 0x40f) // BKPT
	{
#ifdef DEBUG_PABT
		serial_raw_puts("\r\nBKPT\r\n");
		serial_raw_puts("ret_addr: ");
		util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
#endif

#if 0
		if (exc_cpsr & (1<<5)) // Thumb
		{
			exception_extra = RPI2_TRAP_THUMB;
		}
		else
		{

			exception_extra = RPI2_TRAP_ARM;
			p = (uint32_t *)(exc_addr);
			if ((*p) == RPI2_INITIAL_BKPT) // our gdb-entering trap
			{
				exception_extra = RPI2_TRAP_INITIAL;
			}
			//rpi2_reg_context.reg.r15 += 4; // skip 'bkpt'
		}
#endif
	}
	else
	{
		//rpi2_reg_context.reg.r0 = 0xa5a50002; // debug
		//rpi2_reg_context.reg.r1 = exc_addr; // debug
		// exception_extra = RPI2_TRAP_PABT; // prefetch abort
#ifdef DEBUG_PABT
		pp = "\r\nPrefetch Abort\r\n";
		do {i = serial_raw_puts(pp); pp += i;} while (i);
#endif
	}
	rpi2_trap_handler();
	// serial_raw_puts("\r\nPABT ret\r\n");
	// rpi2_dump_context(&rpi2_reg_context);
	// TODO: check if the exception causing instruction needs to be skipped
}

#ifdef DEBUG_REG_CONTEXT
void rpi2_print_dbg(uint32_t xlr, uint32_t xcpsr, uint32_t xspsr)
{
	static char scratchpad[16];

	serial_raw_puts("\r\npabt_ret");
	serial_raw_puts("\r\nlr: ");
	util_word_to_hex(scratchpad, xlr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\ncpsr: ");
	util_word_to_hex(scratchpad, xcpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nspsr: ");
	util_word_to_hex(scratchpad, xspsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
}
#endif

#ifdef RPI_DOUBLE_VECTORS
void rpi2_unhandled_pabt()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, pabt_sp_store\n\t"
			"@ switch to our stack\n\t"
			"movw sp, #:lower16:__abrt_stack\n\t"
			"movt sp, #:upper16:__abrt_stack\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_pabt_context\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ fix sp in the context, if needed\n\t"
			"mrs r0, spsr @ interrupted mode\n\t"
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 1f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, pabt_sp_store\n\t"
			"ldr r1, =rpi2_pabt_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"1:\n\t"
	);
	asm volatile (
			"ldr r0, =exception_info\n\t"
			"mov r1, #3 @ RPI2_EXC_PABT\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"mrc p15, 0, r0, c5, c0, 1 @ IFSR\n\t"
			"and r1, r0, #0x400 @ bit 10\n\t"
			"bne 2f\n\t"
			"and r0, #0x0f @ bit 10\n\t"
			"cmp r0, #0x2\n\t"
			"bne 2f\n\t"
			"@ breakpoint\n\t"
			"mov r1, #4 @ RPI2_TRAP_BKPTn\t"
			"ldr r0, =exception_extra\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =rpi2_exc_reason\n\t"
			"mov r1, #12 @ RPI2_REASON_SW_EXC\n\t"
			"str r1, [r0]\n\t"
			"@ fix return address\n\t"
			"ldr r0, =rpi2_pabt_context\n\t"
			"ldr r1, [r0, #15*4]\n\t"
			"sub r1, #4\n\t"
			"str r1, [r0, #15*4]\n\t"
			"b 3f\n\t"

			"2: \n\t"
			"ldr r0, =exception_extra\n\t"
			"mov r1, #5 @ RPI2_TRAP_BUS\n\t"
			"ldr r0, =rpi2_exc_reason\n\t"
			"mov r1, #10 @ RPI2_REASON_HW_EXC\n\t"
			"str r1, [r0]\n\t"

			"3:\n\t"
	);
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_pabt_handler2\n\t"
	);

	asm volatile (
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();

	asm volatile (
			"subs pc, lr, #0 @ skip bad instruction\n\t"
			".ltorg @ literal pool\n\t"
	);
}
#endif


void rpi2_pabt_handler()
{
#ifdef DEBUG_EXCEPTIONS
	naked_debug();
#endif
	asm volatile (
			"str sp, pabt_sp_store\n\t"
			"movw sp, #:lower16:__abrt_stack\n\t"
			"movt sp, #:upper16:__abrt_stack\n\t"
			"push {r0 - r12}\n\t"
			"mrs r0, cpsr\n\t"
			"push {r0, lr}\n\t"

			"ldr r0, =rpi2_irq_flag\n\t"
			"ldr r0, [r0]\n\t"
			"cmp r0, #0\n\t"
			"bne 3f @ irq flag set\n\t"

			"mrc p15, 0, r0, c5, c0, 1 @ IFSR\n\t"
			"ands r1, r0, #0x400 @ bit 10\n\t"
			"bne 1f\n\t"
			"and r0, #0x0f\n\t"
			"cmp r0, #0x2\n\t"
			"bne 1f\n\t"

			"@ breakpoint\n\t"
			"mov r3, #4 @ RPI2_TRAP_BKPT\n\t"
			"sub r0, lr, #4 @ exception address\n\t"
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
			"pop {r0, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r12}\n\t"
			"ldr sp, pabt_sp_store\n\t"
			"mov pc, #12 @ low PABT vector address\n\t"

			"2: @ ours\n\t"
			"ldr r0, =rpi2_irq_flag\n\t"
			"mov r1, #1\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, =exception_extra\n\t"
			"str r3, [r0]\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #3 @ RPI2_EXC_PABT\n\t"
			"str r1, [r0]\n\t"
			"3: @ irqflag\n\t"
#ifdef DEBUG_EXCEPTIONS
	);
	naked_debug();
	asm volatile (
#endif
			"pop {r0, lr}\n\t"
			"msr cpsr_fsxc, r0\n\t"
			"dsb\n\t"
			"isb\n\t"
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
			"mov r1, #0xf @ all valid modes have bit 4 set\n\t"
			"and r0, r1\n\t"
			"mrs r2, cpsr @ our mode\n\t"
			"and r2, r1\n\t"
			"@ our own mode?\n\t"
			"cmp r0, r2\n\t"
			"bne 3f @ not our own mode, no fix needed\n\t"

			"@ our mode\n\t"
			"ldr r0, pabt_sp_store\n\t"
			"ldr r1, =rpi2_pabt_context\n\t"
			"str r0, [r1, #13*4]\n\t"

			"3:\n\t"
	);

	// rpi2_pabt_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"bl rpi2_pabt_handler2\n\t"
	);

	asm volatile (
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();
	asm volatile (
#ifdef DEBUG_REG_CONTEXT
			"push {r0, r1, r2, lr}\n\t"
			"mov r0, lr\n\t"
			"mrs r1, cpsr\n\t"
			"mrs r2, spsr\n\t"
			"dsb\n\t"
			"isb\n\t"
			"bl rpi2_print_dbg\n\t"
			"pop {r0, r1, r2, lr}\n\t"
#endif
			"subs pc, lr, #0\n\t"
			"pabt_sp_store:\n\t"
			".int 0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_trap()
{
	/* Use BKPT */
#ifdef DEBUG_EXCEPTIONS
	serial_raw_puts("\r\nrpi2_trap\r\n");
#endif
	asm("bkpt #0x7ffe\n\t");
}

void rpi2_gdb_trap()
{
#ifdef DEBUG_EXCEPTIONS
	serial_raw_puts("\r\ngdb_trap\r\n");
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
		*(location_t++) = RPI2_USER_BKPT_THUMB; // BKPT #be
	}
	else
	{
		*(location_a) = RPI2_USER_BKPT_ARM; // BKPT
	}
	return;
}

void rpi2_pend_trap()
{
	/* set pending flag for SIGINT */
	rpi2_ctrl_c_flag = 1; // for now
}

unsigned int rpi2_get_irq_flag()
{
	return (unsigned int) rpi2_irq_flag;
}

void rpi2_set_irq_flag(unsigned int val)
{
	rpi2_irq_flag = (uint32_t)val;
}

unsigned int rpi2_get_exc_reason()
{
	return (unsigned int) rpi2_exc_reason;
}

void rpi2_set_exc_reason(unsigned int val)
{
	rpi2_exc_reason = (uint32_t)val;
}

// for dumping info about debug HW
void rpi2_check_debug()
{
	int i;
	uint32_t tmp1, tmp2, tmp3;
	uint32_t *ptmp1, *ptmp2;
	static char scratchpad[16]; // scratchpad

	asm volatile (
			"mrc p14, 0, %[retreg], c1, c0, 0 @ get DBGDRAR\n\t"
			: [retreg] "=r" (tmp1)::
	);
	asm volatile (
			"mrc p14, 0, %[retreg], c2, c0, 0 @ get DBGDSAR\n\t"
			: [retreg] "=r" (tmp2)::
	);

	serial_raw_puts("\r\nDBGDRAR: ");
	util_word_to_hex(scratchpad, tmp1);
	serial_raw_puts(scratchpad);
	serial_raw_puts(" DBGDSAR: ");
	util_word_to_hex(scratchpad, tmp2);
	serial_raw_puts(scratchpad);

	tmp2 &= 0xfffff000;
	tmp3 = *((uint32_t *) (tmp2 + 0xfb4)); // DBGLSR
	serial_raw_puts(" DBGLSR: ");
	util_word_to_hex(scratchpad, tmp3);
	serial_raw_puts(scratchpad);
	*((uint32_t *) (tmp2 + 0xfb0)) = 0xC5ACCE55; // DBGLAR
	asm volatile (
			"dsb\n\t"
			"isb\n\t"
	);
	tmp3 = *((uint32_t *) (tmp2 + 0xfb4)); // DBGLSR
	serial_raw_puts(" DBGLSR2: ");
	util_word_to_hex(scratchpad, tmp3);
	serial_raw_puts(scratchpad);

	tmp1 &= 0xfffff000; // masked DBGDRAR (ROM table address)
	ptmp1 = (uint32_t *)tmp1; // ptmp1: pointer to component entries
	for (i=0; i<(0xf00/4); i++)
	{
		tmp2 = *(ptmp1++);
		// print component entry
		serial_raw_puts("\r\ncomp: ");
		util_word_to_hex(scratchpad, i);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" : ");
		util_word_to_hex(scratchpad, tmp2);
		serial_raw_puts(scratchpad);
		if (tmp2 == 0) break; // end of component entries

		tmp2 &= 0xfffff000; // masked component offset

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

		ptmp2 = (uint32_t *)(tmp1 + tmp2 + 0xfe0);
		tmp3 = ((*(ptmp2++)) & 0xff);
		tmp3 |= (((*(ptmp2++)) & 0xff) << 8);
		tmp3 |= (((*(ptmp2++)) & 0xff) << 16);
		tmp3 |= (((*(ptmp2++)) & 0xff) << 24);
		serial_raw_puts("\r\nPIDR: ");
		util_word_to_hex(scratchpad, tmp3);
		serial_raw_puts(scratchpad);
	}
	serial_raw_puts("\r\n");

	/* remember to check the lock: DBGLSR, DBGLAR */

}

void rpi2_set_vectors()
{
	unsigned int cpsr_store;
#ifdef DEBUG_EXCEPTIONS
	uint32_t tmp;
#endif
	cpsr_store = rpi2_disable_save_ints();

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
			::[reg] "r" (tmp):
	);
	util_word_to_hex(dbg_tmp_buff, (unsigned int) tmp);
	serial_raw_puts(dbg_tmp_buff);
	serial_raw_puts("\r\n");
#endif

	rpi2_restore_ints(cpsr_store);
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
	rpi2_irq_flag = 0;
	rpi2_ctrl_c_flag = 0;
	rpi2_dgb_enabled = 0; // disable use of gdb trap handler

#if 0
	serial_set_ctrlc((void *)rpi2_pend_trap);
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
#if 1
	asm volatile (
			"mrc p14, 0, %[retreg], c1, c0, 0 @ get DBGDRAR\n\t"
			: [retreg] "=r" (tmp1)::
	);

	// calculate debug registers base address from debug ROM address (DBGRAR)
	tmp1 &= 0xfffff000; // debug ROM address
	tmp3 = (uint32_t *) tmp1; // to pointer
	tmp2 = *tmp3; // get first debug peripheral entry
	tmp2 &= 0xfffff000; // get the base address offset (from debug ROM)
	dbg_reg_base = (tmp1 + tmp2);
#endif
	/* All interrupt vectors to point to dummy interrupt handler? */
	/* cp15, cn=1: v-bit(bit 13)=0 (low vectors), ve-bit(bit 24)=0 (fixed vectors) */
	/* MRC/MCR{<cond>} p15, 0, <Rd>, <CRn>, <CRm>{, <opcode2>} */
	/*
	 * mvn.w r0, #0
	 * bic.w r0, #1, lsl 24
	 * bic.w r0, #2, lsl 12
	 * mrc p15, 0, r1, 1, 0, 0
	 * and.w r1, r0
	 * mcr p15, 0, r1, 1, 0, 0
	 */
	/* make sure that debug is disabled */
	/* cp14, DBGDSCR: MDBGen(bit[15])=0, HDBGen(bit[14])=0 */
	/* - For now - to disable debug mode */
	/* and make BKPT to cause Prefetch Abort exception */
	/*
	 * mvn.w r0, #0
	 * bic.w r0, #3, lsl 14
	 * mrc p14, 0, r1, 0, 1, 0
	 * and.w r1, r0
	 * mcr p14, 0, r1, 0, 1, 0
	 */
	rpi2_set_vectors();
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

void rpi2_delay_loop(unsigned int loop_count)
{
	int i,j;
	for (i=0; i< loop_count; i++)
	{
		// delay 1 ms
		for (j=0; j<700; j++)
//		for (j=0; j<1400; j++)
		{
			asm volatile("nop\n\t");
		}
	}
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
