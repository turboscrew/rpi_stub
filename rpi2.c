/*
 * rpi2.c
 *
 *  Created on: Mar 31, 2015
 *      Author: jaa
 */


#include <stdint.h>
#include "util.h"

// our stack limit (from linker script)
//extern char __usrsys_stack;

extern void serial_irq(); // this shouldn't be public, so it's not in serial.h
extern int serial_raw_puts(char *str); // used for debugging - may be removed
//extern void gdb_trap_handler(); // This should be replaced with a callback
// extern void main(uint32_t r0, uint32_t r1, uint32_t r2);
extern void loader_main();

// Another naughty trick - allocates
// exception_info, exception_extra and rpi2_reg_context
// This way definitions and extern-declarations don't clash
#define extern
#include "rpi2.h"
#undef extern

extern void gdb_trap_handler(); // TODO: make callback
extern uint32_t jumptbl;
uint32_t dbg_reg_base;
volatile uint32_t rpi2_ctrl_c_flag;
#define TRAP_INSTR_T 0xbe00
#define TRAP_INSTR_A 0xe1200070

// exception handlers
void rpi2_undef_handler() __attribute__ ((naked));
void rpi2_svc_handler() __attribute__ ((naked));
void rpi2_aux_handler() __attribute__ ((naked));
void rpi2_dabt_handler() __attribute__ ((naked));
void rpi2_irq_handler() __attribute__ ((naked));
void rpi2_firq_handler() __attribute__ ((naked));
void rpi2_pabt_handler() __attribute__ ((naked));

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

void rpi2_dump_stack(uint32_t mode)
{
	uint32_t p;
	uint32_t lnk;
	char *pp;
	int i, bad;
	uint32_t tmp;
	static char scratchpad[16];

	bad = 0;
	pp = "\r\nSTACK DUMP\r\n";
	do {i = serial_raw_puts(pp); pp += i;} while (i);

	switch (mode & 0xf)
	{
	case 0:
		serial_raw_puts("\r\nUSR MODE: ");
		asm volatile (
				"mrs %[retreg], SP_usr\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_usr\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 1:
		serial_raw_puts("\r\nFIQ MODE: ");
		asm volatile (
				"mrs %[retreg], SP_fiq\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_fiq\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 2:
		serial_raw_puts("\r\nIRQ MODE: ");
		asm volatile (
				"mrs %[retreg], SP_irq\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_irq\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 3:
		serial_raw_puts("\r\nSVC MODE: ");
		asm volatile (
				"mrs %[retreg], SP_svc\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_svc\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 6:
		serial_raw_puts("\r\nMON MODE: ");
		asm volatile (
				"mrs %[retreg], SP_mon\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_mon\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 7:
		serial_raw_puts("\r\nABT MODE: ");
		asm volatile (
				"mrs %[retreg], SP_abt\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_abt\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 10:
		serial_raw_puts("\r\nHYP MODE: ");
		asm volatile (
				"mrs %[retreg], SP_hyp\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], ELR_hyp\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 11:
		serial_raw_puts("\r\nUND MODE: ");
		asm volatile (
				"mrs %[retreg], SP_und\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_und\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	case 15:
		serial_raw_puts("\r\nSYS MODE: ");
		asm volatile (
				"mrs %[retreg], SP_usr\n\t"
				:[retreg] "=r" (p)::
		);
		asm volatile (
				"mrs %[retreg], LR_usr\n\t"
				:[retreg] "=r" (lnk)::
		);
		break;
	default:
		serial_raw_puts("\r\nBAD MODE: ");
		util_word_to_hex(scratchpad, mode & 0x1f);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
		p = 0;
		lnk = 0;
		bad = 1;
		break;
	}
	if (bad == 0)
	{
		// dump stack
		serial_raw_puts("SP = ");
		util_word_to_hex(scratchpad, p);
		serial_raw_puts(scratchpad);
		serial_raw_puts(" LR = ");
		util_word_to_hex(scratchpad, lnk);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
		p &= (~3); // ensure alignment
		for (i=0; i<16; i++)
		{
			tmp = *((uint32_t *)p);
			util_word_to_hex(scratchpad, p);
			serial_raw_puts(scratchpad);
			serial_raw_puts(" : ");
			util_word_to_hex(scratchpad, tmp);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\n");
			p += 4;
		}
	}
}

#if 1
//
// stack frame:
// r0 - r12, lr(raw), spsr, sp
// r0: lr-fix value on entry, sp on return
static inline void push_stackframe()
{
	asm volatile(
			"push {r0 - r12, lr}\n\t"
			"mrs r0, spsr\n\t"
			"mov r1, sp\n\t"
			"push {r0, r1}\n\t"
			"mov r0, sp @ frame\n\t"
	);
}

// pops a frame from stack and returns
// stack frame:
// r0 - r12, lr(fixed), spsr, sp
static inline void pop_stackframe()
{
	asm volatile(
			"pop {r0, r1}\n\t"
			"msr spsr, r0\n\t"
			"mov sp, r1\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #0 @ lr already fixed\n\t"
	);
}

// for fix for the exception return address
// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4
static void fix_ret(uint32_t frm_addr, uint32_t fix_val)
{
	uint32_t tmp;
	tmp = *((uint32_t *)(frm_addr + 8));
	tmp -= fix_val;
	*((uint32_t *)(frm_addr + 8)) = tmp;
}

// version for naked functions
// pointer to frame in r0, fix-value in r1
static inline void fix_lr()
{
	asm volatile(
			"push {r0, r1, r2}\n\t"
			"ldr r2, [r0, #0x8]\n\t"
			"sub r2, r1 @ fix\n\t"
			"str r2, [r0, #0x8]\n\t"
			"pop {r0, r1, r2}\n\t"
	);
}
#endif
#if 0
// TODO: doublecheck
static inline void copy_from_stackframe(uint32_t loc)
{
	asm volatile(
			"push {r0 - r12}\n\t"
			"mov r9, sp\n\t"
			"push {r9, lr}\n\t"

			"mov sp, %[sp_loc]\n\t"
			"ldr lr, =rpi2_reg_context @ r0 -> reg_context\n\t"

			"pop {r4, r5, r6, r7}\n\t"
			"mov sp, r7\n\t"
			"@ msr cpsr, r6 @ cpsr not copied\n\t"
			"str r5, [lr, #16*4] @ spsr\n\t"
			"str r4, [lr, #15*4] @ pc\n\t"
			"pop {r0 - r12}\n\t"
			"stmia lr, {r0 - r12}\n\t"
			"str sp, [lr, #13*4]\n\t"

			"pop {r9, lr}\n\t"
			"mov sp, r9\n\t"
			"pop {r0 - r12}\n\t"
			:[sp_loc] "=r" (loc) ::
	);
}

// gets the banked regs from last mode and overwrites the stored
// registers with them
static inline void store_banked_regs()
{
	asm volatile(
			"push {r0 - r3}\n\t"

			"mov r2, #0  @ clear exception_extra\n\t"
			"ldr r1, =exception_extra\n\t"
			"str r2, [r1]\n\t"

			"@ store our cpsr in r3 for restoring at the end\n\t"
			"mrs r3, cpsr\n\t"

			"@ cpsr with zeroed mode into r2 for easier mode change\n\t"
			"mvn r2, #0x0f @ clean up mode bits (all modes have bit 4 set)\n\t"
			"and r2, r3\n\t"

			"@ check original mode field to find out the old bank\n\t"
			"@ and switch into that mode to get lr and sp\n\t"
			"@ (and for FIQ the banked regs r8 - r12)\n\t"
			"mrs r1, spsr @ get previous mode cpsr\n\t"
			"and r1, #0x0f @ extract mode field (bit 4 always set)\n\t"

			"@ USR mode?\n\t"
			"@ USR and SYS use the same registers, but sys is privileged\n\t"
			"@ from user mode we couldn't get back in our original mode\n\t"
			"@ so, if mode is USR, replace with mode SYS\n\t"
			"cmp r1, #0x00 @ USR?\n\t"
			"bne 1f\n\t"
			"mov r1, #0x0f @ set SYS\n\t"

			"1:\n\t"
			"@ set the mode\n\t"
			"orr r2, r2, r1\n\t"
			"msr cpsr_c, r2 @ \n\t"

			"@ if FIQ-mode, overwrite regs 8-12 by banked regs\n\t"
			"cmp r1, #0x01 @ FIQ?\n\t"
			"bne 2f\n\t"
			"ldr r0, =rpi2_reg_context+8*4 @ r0 -> reg_context.r8\n\t"
			"stmia r0, {r8,r9,r10,r11,r12}\n\t"
			"b 3f @ back to 'mainstream'\n\t"

			"2:\n\t"
			"@ if our own mode, LR was overwritten\n\t"
			"@ by the exception and SP and SPSR are already stored\n\t"
			"cmp r1, #0x07 @ ABT?\n\t"
			"bne 3f\n\t"
			"b 4f @ skip this phase\n\t"

			"3:\n\t"
			"@ Other modes\n\t"
			"ldr r0, =rpi2_reg_context	@ r0 -> reg_context\n\t"
			"mov r1, sp\n\t"
			"mov r2, lr\n\t"
			"str r1, [r0, #13*4]\n\t"
			"str r2, [r0, #14*4]\n\t"

			"4:\n\t"
			"@ back to our original mode\n\t"
			"msr cpsr, r3 @ \n\t"
			"pop {r0 - r3}\n\t"
	);
}

// sets the banked regs from the stored registers and writes them
// into the banked regs of the mode in stored cpsr
static inline void load_banked_regs()
{
	asm volatile(
			"push {r0 - r3}\n\t"

			"ldr r0, =rpi2_reg_context	@ r0 -> reg_context\n\t"

			"@ store our cpsr in r3 for restoring at the end\n\t"
			"mrs r3, cpsr\n\t"

			"@ cpsr with zeroed mode into r2 for easier mode change\n\t"
			"mvn r2, #0x0f @ clean up mode bits (all modes have bit 4 set)\n\t"
			"and r2, r3\n\t"

			"@ check original mode field to find out the old bank\n\t"
			"@ and switch into that mode to get lr and sp\n\t"
			"@ (and for FIQ the banked regs r8 - r12)\n\t"
			"ldr r1, [r0, #16*4] @ get previous mode cpsr\n\t"
			"and r1, #0x0f @ extract mode field (bit 4 always set)\n\t"

			"@ USR mode?\n\t"
			"@ USR and SYS use the same registers, but sys is privileged\n\t"
			"@ from user mode we couldn't get back in our original mode\n\t"
			"@ so, if mode is USR, replace with mode SYS\n\t"
			"cmp r1, #0x00 @ USR?\n\t"
			"bne 1f\n\t"
			"mov r1, #0x0f @ set SYS\n\t"

			"1:\n\t"
			"@ set the mode\n\t"
			"orr r2, r2, r1\n\t"
			"msr cpsr_c, r2 @ \n\t"

			"@ if FIQ-mode, load banked regs 8-12\n\t"
			"cmp r1, #0x01 @ FIQ?\n\t"
			"bne 2f\n\t"
			"ldr r0, =rpi2_reg_context+8*4 @ r0 -> reg_context.r8\n\t"
			"ldmia r0, {r8,r9,r10,r11,r12}\n\t"
			"ldr r0, =rpi2_reg_context	@ restore r0 -> reg_context\n\t"
			"b 3f @ back to 'mainstream'\n\t"

			"2:\n\t"
			"@ if our own mode, LR was overwritten\n\t"
			"@ by the exception and SP and SPSR will be handled in\n\t"
			"@ due course\n\t"
			"cmp r1, #0x07 @ ABT?\n\t"
			"bne 3f\n\t"
			"b 4f @ skip this phase\n\t"

			"3:\n\t"
			"@ Other modes\n\t"
			"ldr r1, [r0, #13*4]\n\t"
			"ldr r2, [r0, #14*4]\n\t"
			"mov r1, sp\n\t"
			"mov r2, lr\n\t"

			"4:\n\t"
			"@ back to our original mode\n\t"
			"msr cpsr, r3 @ \n\t"

			"pop {r0 - r3}\n\t"
	);
}

// TODO: doublecheck
static inline void copy_to_stackframe(uint32_t loc)
{
	asm volatile(
			"push {r0 - r12}\n\t"
			"mov r9, sp\n\t"
			"push {r9, lr}\n\t"

			"mov sp, %[sp_loc]\n\t"
			"ldr lr, =rpi2_reg_context @ r0 -> reg_context\n\t"

			"ldr sp, [lr, #13*4]\n\t"
			"ldmda lr, {r0 - r12}\n\t"
			"push {r0 - r12}\n\t"
			"ldr r4, [lr, #15*4] @ pc\n\t"
			"ldr r5, [lr, #16*4] @ spsr\n\t"
			"mrs r6, cpsr\n\t"
			"mov r7, sp\n\t"
			"push {r4, r5, r6, r7}\n\t"

			"pop {r9, lr}\n\t"
			"mov sp, r9\n\t"
			"pop {r0 - r12}\n\t"
			:[sp_loc] "=r" (loc) ::
	);
}

// for debug
static inline void pop_stackframe_r5()
{
	asm volatile(
			"@cpsid aif\n\t"
			"pop {r4, r6, r7, r8}\n\t"
			"mov sp, r8\n\t"
			"msr cpsr, r7\n\t"
			"msr spsr, r6\n\t"
			"mov lr, r5\n\t"
			"pop {r0 - r12}\n\t"
			"@cpsie aif\n\t"
			"subs pc, lr, #0 @ pc = lr - #0; cpsr = spsr\n\t"
	);
}
#endif

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

#define DEBUG_UNDEF

void rpi2_undef_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
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

	//copy_from_stackframe(stack_frame_addr);
	//store_banked_regs();

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
#endif

	// exception_info = RPI2_EXC_UNDEF;
	// rpi2_trap_handler();
	asm volatile (
			"push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10,lr}\n\t"
			"1:\n\t"
			"mov r0, #0x100\n\t"
			"mov r1, #0x100\n\t"
			"mov r2, #5\n\t"
			"bl debug_blink\n\t"
			"mov r3, #0x2000 @ 2 s pause\n\t"
			"bl debug_wait\n\t"
			"mov r0, #0x1000\n\t"
			"mov r1, #0x1000\n\t"
			"mov r2, #2\n\t"
			"bl debug_blink\n\t"
			"mov r3, #0x5000 @ 5 s pause\n\t"
			"bl debug_wait\n\t"
			"b 1b\n\t"
			"pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, lr}\n\t"

			"@ for now we can't return\n\t"
			"push {r0, r1}\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #1 @ RPI2_EXC_UNDEF\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1}\n\t"
			"bl rpi2_trap_handler\n\t"
	);

	//load_banked_regs();
	//copy_to_stackframe(stack_frame_addr);
}

void rpi2_undef_handler()
{
	//push_stackframe();

	// rpi2_undef_handler2() // - No C in naked function
	asm volatile (
			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_undef_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #0 @ skip the bad instruction\n\t"
	);

	//pop_stackframe();
}

#define DEBUG_SVC
uint32_t rpi2_svc_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
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

	//copy_from_stackframe(stack_frame_addr);
	//store_banked_regs();

#ifdef DEBUG_SVC
	p = "\r\nSVC EXCEPTION\r\n";
	do {i = serial_raw_puts(p); p += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
#endif

	 exception_info = RPI2_EXC_SVC;
	// rpi2_trap_handler();

	//load_banked_regs();
	//copy_to_stackframe(stack_frame_addr);
	// cause SVC-loop
	// fix_ret(stack_frame_addr, 4);
	// return frame address and (fixed) return address
	/*
	asm volatile (
			"mov r4, %[sp_reg]\n\t"
			"mov r5, %[link_reg]\n\t"
			::[sp_reg] "r" (stack_frame_addr), [link_reg] "r" (exc_addr):
	);
	*/

	return stack_frame_addr;
}

void rpi2_svc_handler()
{

	//push_stackframe();

	// rpi2_svc_handler2() // - No C in naked function
	/*
	asm volatile (
			"bl rpi2_svc_handler2\n\t"
	);
	*/

	asm volatile (
			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_svc_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #0\n\t"
	);

	// cause SVC-loop
	//asm volatile("mov r1, #4\n\t");
	//fix_lr()
	//pop_stackframe();
	//pop_stackframe_r5();
}

#define DEBUG_AUX
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

	//copy_from_stackframe(stack_frame_addr);
	//store_banked_regs();

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
#endif

	exception_info = RPI2_EXC_AUX;
	// rpi2_trap_handler();

	//load_banked_regs();
	//copy_to_stackframe(stack_frame_addr);
	// return frame address and (fixed) return address
	/*
	asm volatile (
			"mov r4, %[sp_reg]\n\t"
			"mov r5, %[link_reg]\n\t"
			::[sp_reg] "r" (stack_frame_addr), [link_reg] "r" (exc_addr):
	);
	*/
}

void rpi2_aux_handler()
{
	/*
	asm volatile (
			"push {r0}\n\t"
			"mov r0, #0 @ lr fix value\n\t"
	);
	push_stackframe();
	*/
	// rpi2_svc_handler2() // - No C in naked function
	asm volatile (
			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_aux_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #0\n\t"
	);

	//pop_stackframe();
	//pop_stackframe_r5();

}


#define DEBUG_DABT
void rpi2_dabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
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

	//copy_from_stackframe(stack_frame_addr);
	//store_banked_regs();

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

#endif

	// exception_info = RPI2_EXC_DABT;
	// rpi2_trap_handler();
	asm volatile (

			"push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10}\n\t"
			"1:\n\t"
			"mov r0, #0x100\n\t"
			"mov r1, #0x100\n\t"
			"mov r2, #5\n\t"
			"bl debug_blink\n\t"
			"mov r3, #0x5000 @ 5 s pause\n\t"
			"bl debug_wait\n\t"
			"mov r0, #0x1000\n\t"
			"mov r1, #0x1000\n\t"
			"mov r2, #5\n\t"
			"bl debug_blink\n\t"
			"mov r3, #0x5000 @ 5 s pause\n\t"
			"bl debug_wait\n\t"
			"b 1b\n\t"
			"pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10}\n\t"

			"@ for now we can't return\n\t"
			"push {r0, r1}\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #4 @ RPI2_EXC_DABT\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1}\n\t"
			"bl rpi2_trap_handler\n\t"
	);

	//load_banked_regs();
	//copy_to_stackframe(stack_frame_addr);
}

void rpi2_dabt_handler()
{
/*
	asm volatile (
			"push {r0}\n\t"
			"mov r0, #8 @ lr fix value\n\t"
			"push {r1 - r12, lr}\n\t"
	);
*/
	//push_stackframe();

	// rpi2_dabt_handler2(sp_value) // - No C in naked function
	asm volatile (
			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_dabt_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"

			"subs pc, lr, #8\n\t"
	);

	//pop_stackframe();
}

#define DEBUG_IRQ
void rpi2_irq_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	uint32_t exc_cpsr;
#ifdef DEBUG_IRQ
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
			//copy_from_stackframe(stack_frame_addr);
			//store_banked_regs();

			//exception_info = RPI2_EXC_IRQ;
			//rpi2_trap_handler();
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
#endif
			//load_banked_regs();
			//copy_to_stackframe(stack_frame_addr);
		}
	}
	else
	{
		// unhandled IRQ
		//copy_from_stackframe(stack_frame_addr);
		//store_banked_regs();

		//exception_info = RPI2_EXC_IRQ;
		//rpi2_trap_handler();
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
#endif

		//load_banked_regs();
		//copy_to_stackframe(stack_frame_addr);
	}

#if 1
	// if crtl-C, dump exception info (for now)
	if (rpi2_ctrl_c_flag)
	{
		rpi2_ctrl_c_flag = 0;
		serial_raw_puts("\r\nCTRL-C\r\n");
		serial_raw_puts("exc_addr: ");
		util_word_to_hex(scratchpad, exc_addr);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\nSPSR: ");
		util_word_to_hex(scratchpad, exc_cpsr);
		serial_raw_puts(scratchpad);
		serial_raw_puts("\r\n");
		rpi2_dump_stack(exc_cpsr);
	}
#endif
	// return frame address and (fixed) return address
	asm volatile (
			"mov r4, %[sp_reg]\n\t"
			"mov r5, %[link_reg]\n\t"
			::[sp_reg] "r" (stack_frame_addr), [link_reg] "r" (exc_addr):
	);
}

#if 1 // debug stuff
void rpi2_irq_handler()
{
	asm volatile (
			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_irq_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #4\n\t"
	);

}
#else
void rpi2_irq_handler()
{
	// set parameter for 'push_stackframe'-call
	/*
	asm volatile (
			"push {r0}\n\t"
			"mov r0, #4 @ lr fix value\n\t"
	);
	push_stackframe();
	*/
	// rpi2_irq_handler2() // - No C in naked function
	asm volatile (
			"push {r1 - r12, lr}\n\t"

			"@ sp-value in r0 (by push_stackframe)\n\t"
			"mov r5, lr @ fixed lr\n\t"
			"mov r4, sp @ r4 shouldn't be used by function prologue\n\t"
			"bl rpi2_irq_handler2\n\t"
			"mov sp, r4 @ sp (to saved frame)\n\t"

			"pop {r0 - r12, lr}\n\t"

			"subs pc, lr, #4\n\t"
	);

	//pop_stackframe();
	//pop_stackframe_r5();

}

#endif

#define DEBUG_FIQ
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
	//copy_from_stackframe(stack_frame_addr);
	//store_banked_regs();

	// exception_info = RPI2_EXC_FIQ;
	// rpi2_trap_handler();
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
#endif

	//load_banked_regs();
	//copy_to_stackframe(stack_frame_addr);
}

void rpi2_firq_handler()
{
/*
	asm volatile
	(
			"push {r0}\n\t"
			"mov r0, #4 @ lr fix value\n\t"
	);
*/
	//push_stackframe();

	asm volatile (
			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_firq_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #4\n\t"
	);

	//pop_stackframe();
}

//#define DEBUG_PABT
void rpi2_pabt_handler2(uint32_t stack_frame_addr, uint32_t exc_addr)
{
	int i;
	uint32_t *p;
	uint32_t exc_cpsr;
	uint32_t dbg_state;
	uint32_t ifsr;

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

	//copy_from_stackframe(stack_frame_addr);
	//store_banked_regs();

#ifdef DEBUG_PABT
	// raw puts, because we are here with interrupts disabled
	pp = "\r\nPABT EXCEPTION\r\n";
	do {i = serial_raw_puts(pp); pp += i;} while (i);

	serial_raw_puts("exc_addr: ");
	util_word_to_hex(scratchpad, exc_addr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nSPSR: ");
	util_word_to_hex(scratchpad, exc_cpsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\ndbgdscr: ");
	util_word_to_hex(scratchpad, dbg_state);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\nIFSR: ");
	util_word_to_hex(scratchpad, ifsr);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
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
		}
	}
	else
	{
		exception_extra = RPI2_TRAP_PABT; // prefetch abort
#ifdef DEBUG_PABT
		pp = "\r\nPrefetch Abort\r\n";
		do {i = serial_raw_puts(pp); pp += i;} while (i);
#endif
	}
	rpi2_trap_handler();

	//load_banked_regs();
	//copy_to_stackframe(stack_frame_addr);

	// return frame address and (fixed) return address
	asm volatile (
			"mov r4, %[sp_reg]\n\t"
			"mov r5, %[link_reg]\n\t"
			::[sp_reg] "r" (stack_frame_addr), [link_reg] "r" (exc_addr):
	);
}

void rpi2_pabt_handler()
{
/*
	asm volatile (
			"cpsid if\n\t"
			"push {r0}\n\t"
			"mov r0, #4 @ lr fix value\n\t"
	);
*/
	//push_stackframe();

	// rpi2_pabt_handler2(sp_value);
	asm volatile (

			"push {r0 - r12, lr}\n\t"
			"mov r1, lr\n\t"
			"mov r0, sp\n\t"
			"bl rpi2_pabt_handler2\n\t"
			"pop {r0 - r12, lr}\n\t"
			"subs pc, lr, #0 @ to skip bkpt\n\t"
	);

	//pop_stackframe();
	//pop_stackframe_r5();
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
	rpi2_ctrl_c_flag = 0;
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

	tmp1 &= 0xfffff000;
	tmp3 = (uint32_t *) tmp1;
	tmp2 = *tmp3;
	tmp2 &= 0xfffff000;
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

	rpi2_set_vectable();
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
