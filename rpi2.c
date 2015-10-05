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

// TODO: separate context storages for each exception/core + one for gdb-stub?
// maybe a context storage stack or array? Only unhandled exceptions goto gdb-stub
// TODO: pseudo-vector for IRQs other than UART0? Only unhandled IRQs goto gdb-stub
// TODO: Both IRQ and PABT can be shared

extern void serial_irq(); // this shouldn't be public, so it's not in serial.h
extern int serial_raw_puts(char *str); // used for debugging - may be removed
extern void serial_set_ctrlc(void *handler); // used for debugging
//extern void gdb_trap_handler(); // This should be replaced with a callback
// extern void main(uint32_t r0, uint32_t r1, uint32_t r2);
extern void loader_main();

// Another naughty trick - allocates
// exception_info, exception_extra and rpi2_reg_context
// This way definitions and extern-declarations don't clash
#define extern
#include "rpi2.h"
#undef extern

// moved to rpi2.h
//#define DEBUG_CTRLC

//#define DEBUG_UNDEF
//#define DEBUG_SVC
//#define DEBUG_AUX
//#define DEBUG_DABT
//#define DEBUG_IRQ
//#define DEBUG_FIQ
//#define DEBUG_PABT

// debug for context handling
//#define DEBUG_REG_CONTEXT


#define TRAP_INSTR_T 0xbe00
#define TRAP_INSTR_A 0xe1200070

extern void gdb_trap_handler(); // TODO: make callback
extern uint32_t jumptbl;
uint32_t dbg_reg_base;
volatile uint32_t rpi2_ctrl_c_flag;
volatile uint32_t rpi2_sigint_flag;
volatile rpi2_reg_context_t rpi2_irq_context;

extern char __fiq_stack;
extern char __usrsys_stack;
extern char __svc_stack;
extern char __irq_stack;
extern char __mon_stack;
extern char __hyp_stack;
extern char __und_stack;
extern char __abrt_stack;

// exception handlers
void rpi2_undef_handler() __attribute__ ((naked));
void rpi2_svc_handler() __attribute__ ((naked));
void rpi2_aux_handler() __attribute__ ((naked));
void rpi2_dabt_handler() __attribute__ ((naked));
void rpi2_irq_handler() __attribute__ ((naked));
void rpi2_firq_handler() __attribute__ ((naked));
void rpi2_pabt_handler() __attribute__ ((naked));
void rpi2_ctrlc_trap() __attribute__ ((naked));

/* delay() borrowed from OSDev.org */
static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : : [count]"r"(count) : "cc");
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

void rpi2_set_vectable()
{
	asm volatile (
			".globl jumptbl\n\t"
			"push {r0, r1, r2, r3}\n\t"
			"cpsid aif @ Disable interrupts\n\t"
			"b irqset @ Skip the vector table to be copied\n\t"
			"jumpinstr:\n\t"
			"ldr pc, jumptbl    @ reset\n\t"
			"ldr pc, jumptbl+4  @ undef\n\t"
			"ldr pc, jumptbl+8  @ SVC\n\t"
			"ldr pc, jumptbl+12 @ prefetch\n\t"
			"ldr pc, jumptbl+16 @ data\n\t"
			"ldr pc, jumptbl+20 @ aux\n\t"
			"ldr pc, jumptbl+24 @ IRQ\n\t"
			"ldr pc, jumptbl+28 @ FIRQ\n\t"
			"jumptbl: @ 8 addresses\n\t"
			".word loader_main\n\t"
			".word rpi2_undef_handler\n\t"
			".word rpi2_svc_handler\n\t"
			".word rpi2_pabt_handler\n\t"
			".word rpi2_dabt_handler\n\t"
			".word rpi2_aux_handler @ in some cases\n\t "
			".word rpi2_irq_handler\n\t"
			".word rpi2_firq_handler\n\t"
			"irqset:\n\t"
			"@ Copy the vectors\n\t"
			"ldr r0, =jumpinstr\n\t"
			"mov r1, #0\n\t"
			"ldr r2, =irqset\n\t"
			"irqset_loop:\n\t"
			"ldr r3, [r0], #4 @ word (auto-increment)\n\t"
			"str r3, [r1], #4\n\t"
			"cmp r2, r0 @ r2 - r0\n\t"
			"bhi irqset_loop\n\t"
			"b 1f\n\t"
			".ltorg @ literal pool\n\t"
			"1:\n\t"
			"cpsie aif @ Enable interrupts\n\t"
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

// common trap handler outside exception handlers
void rpi2_trap_handler()
{
	// IRQs need to be enabled for serial I/O
	asm volatile ("cpsie i\n\t");
	gdb_trap_handler();
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

void rpi2_undef_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	static char scratchpad[16]; // scratchpad
#ifdef DEBUG_UNDEF
	static char scratchpad[16]; // scratchpad
	char *p;
	int i;
#endif
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// for fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	rpi2_reg_context.reg.r15 -= 4; // PC

#ifdef DEBUG_UNDEF
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
#endif

	exception_info = RPI2_EXC_UNDEF;
	rpi2_trap_handler();
}

void rpi2_undef_handler()
{
	// store processor context
	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_reg_context\n\t"
	);
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"
	);

	// rpi2_undef_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"bl rpi2_undef_handler2\n\t"
			"pop {r0 - r3}\n\t"
	);

	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();
	asm volatile (
			"subs pc, lr, #0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_svc_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;

#ifdef DEBUG_SVC
	int i;
	uint32_t tmp;
	char *p;
	static char scratchpad[16]; // scratchpad
#endif
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

#ifdef DEBUG_SVC
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
#endif

	exception_info = RPI2_EXC_SVC;
	rpi2_trap_handler();
}


void rpi2_svc_handler()
{
	// store processor context
	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_reg_context\n\t"
	);
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"

	);

	// rpi2_svc_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"bl rpi2_svc_handler2\n\t"
			"pop {r0 - r3}\n\t"
	);


	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	read_context();

	// can be turned on for processor context
	// save/restore debugging
	asm volatile (
			"subs pc, lr, #0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_aux_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
#ifdef DEBUG_AUX
	int i;
	uint32_t tmp;
	char *p;
	static char scratchpad[16]; // scratchpad
#endif
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

#ifdef DEBUG_AUX
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
#else
	exception_info = RPI2_EXC_AUX;
	rpi2_trap_handler();
#endif

}

void rpi2_aux_handler()
{
	// store processor context
	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_reg_context\n\t"
	);
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"
	);

	// rpi2_aux_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"bl rpi2_aux_handler2\n\t"
			"pop {r0 - r3}\n\t"
	);

	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();
	asm volatile (
			"subs pc, lr, #0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_dabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
	static char scratchpad[16]; // scratchpad
#ifdef DEBUG_DABT
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad
#endif
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	rpi2_reg_context.reg.r15 -= 8; // PC

#ifdef DEBUG_DABT
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
#endif

	exception_info = RPI2_EXC_DABT;
	rpi2_trap_handler();
}

void rpi2_dabt_handler()
{
	// store processor context
	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_reg_context\n\t"
	);
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"
	);

	// rpi2_dabt_handler() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"bl rpi2_dabt_handler2\n\t"
			"pop {r0 - r3}\n\t"
	);

	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();
	asm volatile (
			"subs pc, lr, #0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

// trapping from IRQ-mode to pabt
void rpi2_ctrlc_trap()
{
	asm volatile (
			"bkpt #0xe\n\t"
	);
}

void rpi2_irq_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
#ifdef DEBUG_IRQ
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad
#else
#ifdef DEBUG_CTRLC
	static char scratchpad[16]; // scratchpad
#endif
#endif

	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);
	// fix for the exception return address
	// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
	// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
	// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
	rpi2_irq_context.reg.r15 -= 4; // PC

	// IRQ2 pending
	if (*((volatile uint32_t *)IRC_PENDB) & (1<<19))
	{
		// UART IRQ
		if (*((volatile uint32_t *)IRC_PEND2) & (1<<25))
		{
			//rpi2_led_blink(200, 200, 8);
			serial_irq();
		}
		else
		{
			// unhandled IRQ
#ifdef DEBUG_IRQ
			p = "\r\nIRQ EXCEPTION\r\n";
			do {i = serial_raw_puts(p); p += i;} while (i);

			serial_raw_puts("exc_addr: ");
			util_word_to_hex(scratchpad, exc_addr);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\nSPSR: ");
			util_word_to_hex(scratchpad, exc_cpsr);
			serial_raw_puts(scratchpad);
			tmp = *((volatile uint32_t *)IRC_PEND2);
			serial_raw_puts("\r\nPEND2: ");
			util_word_to_hex(scratchpad, tmp);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\n");
			rpi2_dump_context(&rpi2_irq_context);
#else
			exception_info = RPI2_EXC_IRQ;
			rpi2_trap_handler();
#endif
		}
	}
	else
	{
		// unhandled IRQ
#ifdef DEBUG_IRQ
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
#else
		exception_info = RPI2_EXC_IRQ;
		rpi2_trap_handler();
#endif
	}

#ifdef DEBUG_CTRLC
	// if crtl-C, dump exception info (for now)
	if (rpi2_ctrl_c_flag)
	{
		// swap flags to stop further interrupts on one ctrl-C
		rpi2_ctrl_c_flag = 0;
		rpi2_sigint_flag = 1;
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

void rpi2_irq_handler()
{
	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_irq_context\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"
	);

	// rpi2_irq_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"bl rpi2_irq_handler2\n\t"
			"pop {r0 - r3}\n\t"
	);

	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
			"ldr r0, =rpi2_irq_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();
	asm volatile (
			"@ if sigint-flag, return to ctrl-c trap instead \n\t"
			"@ of the interrupted program\n\t"
			"push {r0, r1}\n\t"
			"ldr r0, =rpi2_sigint_flag\n\t"
			"ldr r1, [r0]\n\t"
			"cmp r1, #0\n\t"
			"bne 1f @ sigint happened\n\t"
			"pop {r0, r1}\n\t"
			"subs pc, lr, #0\n\t"
			"1:\n\t"
			"ldr r0, =rpi2_ctrlc_trap\n\t"
			"mov lr, r0\n\t"
			"pop {r0, r1}\n\t"
			"subs pc, lr, #0\n\t"
			".ltorg @ literal pool\n\t"
	);
}


void rpi2_firq_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
#ifdef DEBUG_FIQ
	uint32_t tmp;
	int i;
	char *p;
	static char scratchpad[16]; // scratchpad
#endif
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

#ifdef DEBUG_FIQ
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
#else
	exception_info = RPI2_EXC_FIQ;
	rpi2_trap_handler();
#endif
}

void rpi2_firq_handler()
{
	// store processor context
	asm volatile (
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_reg_context\n\t"
	);
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"
	);

	// rpi2_firq_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"bl rpi2_firq_handler2\n\t"
			"pop {r0 - r3}\n\t"
	);

	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
			"ldr r0, =rpi2_reg_context @ for read_context\n\t"
	);

	// restore processor context
	read_context();
	asm volatile (
			"subs pc, lr, #0\n\t"
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_pabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	int i;
	uint32_t *p, *p2;
	uint32_t exc_cpsr;
	uint32_t dbg_state;
	uint32_t ifsr;
	static char scratchpad[16]; // scratchpad

#ifdef DEBUG_PABT
	uint32_t tmp;
	char *pp;
	static char scratchpad[16]; // scratchpad
#endif
	asm volatile
	(
			"mrs %[var_reg], spsr\n\t"
			:[var_reg] "=r" (exc_cpsr) ::
	);

	// if crtl-C trap
	if (rpi2_sigint_flag) // leave the flag for gdb_trap_handler()
	{
		// copy irq context to reg context for returning
		// to original program that was interrupted by CTRL-C
		p = (uint32_t *)(&rpi2_irq_context);
		p2 = (uint32_t *)(&rpi2_reg_context);
		for (i=0; i<(sizeof(rpi2_reg_context_t)/4); i++)
		{
			*(p2++) = *(p++);
		}
		// serial_raw_puts("\r\nIRQ_PABT\r\n");
	}
	else
	{
		// fix for the exception return address
		// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
		// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
		// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
		rpi2_reg_context.reg.r15 -= 4; // PC
	}

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
	exception_info = RPI2_EXC_PABT;

	if (ifsr & 0x40f) // BKPT
	{
		if (exc_cpsr & (1<<5)) // Thumb
		{
			exception_extra = RPI2_TRAP_THUMB;
		}
		else
		{
			exception_extra = RPI2_TRAP_ARM;
			p = (uint32_t *)(exc_addr - 4);
			if (((*p) & 0x0f) == 0x0f) // our gdb-entering trap
			{
				exception_extra = RPI2_TRAP_INITIAL;
			}
			//rpi2_reg_context.reg.r15 += 4; // skip 'bkpt'
		}
	}
	else
	{
		//rpi2_reg_context.reg.r0 = 0xa5a50002; // debug
		//rpi2_reg_context.reg.r1 = exc_addr; // debug
		exception_extra = RPI2_TRAP_PABT; // prefetch abort
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

void rpi2_pabt_handler()
{
	asm volatile (
			"dsb\n\t"
			"isb\n\t"
			"push {r12} @ popped in write_context\n\t"
			"ldr r12, =rpi2_reg_context\n\t"
	);
	// store processor context
	write_context();

	asm volatile (
			"@ align SP\n\t"
			"mov r0, sp\n\t"
			"and r1, r0, #7\n\t"
			"sub r0, r1\n\t"
			"mov sp, r0\n\t"
			"push {r0,r1} @ stack correction\n\t"
	);

	// rpi2_pabt_handler2() // - No C in naked function
	asm volatile (
			"mov r0, sp\n\t"
			"mov r1, lr\n\t"
			"push {r0 - r3}\n\t"
			"dsb\n\t"
			"isb\n\t"
			"bl rpi2_pabt_handler2\n\t"
			"dsb\n\t"
			"isb\n\t"
			"pop {r0 - r3}\n\t"
	);

	asm volatile (
			"@ restore stack correction\n\t"
			"pop {r0, r1}\n\t"
			"add r0, r1\n\t"
			"mov sp, r0\n\t"
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
			".ltorg @ literal pool\n\t"
	);
}

void rpi2_set_vector(int excnum, void *handler)
{
	/*
	 * If debug-mode is disabled, BKPT causes
	 * Prefetch Abort exception - vector table address 0x0c
	 */
	uint32_t *vector = (uint32_t *)32; // beginning of the jumptbl
	vector[excnum] = (uint32_t)handler;
	return;
}

void rpi2_trap()
{
	/* Use BKPT */
	asm("bkpt #0xf\n\t");
}

/* set BKPT to given address */
void rpi2_set_trap(void *address, int kind)
{
	uint16_t *location_t = (uint16_t *)address;
	uint32_t *location_a = (uint32_t *)address;

	/* poke BKPT instruction */
	if (kind == RPI2_TRAP_THUMB)
	{
		*(location_t++) = 0xBEBE; // BKPT
	}
	else
	{
		*(location_a) = 0xE1200070; // BKPT
	}
	return;
}

void rpi2_pend_trap()
{
	/* set pending flag for Prefetch Abort exception */
	// TODO: pend Prefetch Abort
	rpi2_ctrl_c_flag = 1; // for now
}

unsigned int rpi2_get_sigint_flag()
{
	return (unsigned int) rpi2_sigint_flag;
}

void rpi2_set_sigint_flag(unsigned int val)
{
	rpi2_sigint_flag = (uint32_t)val;
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

void rpi2_init()
{
	int i;
	uint32_t *p;
	uint32_t tmp1, tmp2;
	uint32_t *tmp3;
	exception_info = RPI2_EXC_NONE;
	exception_extra = 0;
	rpi2_sigint_flag = 0;
	rpi2_ctrl_c_flag = 0;
#ifdef DEBUG_CTRLC
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

	rpi2_set_vectable(); // write exception vectors
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
