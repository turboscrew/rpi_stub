/*
 * rpi2.c
 *
 *  Created on: Mar 31, 2015
 *      Author: jaa
 */


#include <stdint.h>

// Not pretty, but didn't want to include everything
// TODO: Maybe callbacks later...
extern void serial_irq();
extern void gdb_trap_handler();
// extern void main(uint32_t r0, uint32_t r1, uint32_t r2);
extern void loader_main();

// Another naughty trick - allocates
// exception_info, exception_extra and rpi2_reg_context
// This way definitions and extern-declarations don't clash
#define extern
#include "rpi2.h"
#undef extern

extern uint32_t jumptbl;

#define TRAP_INSTR_T 0xbe00
#define TRAP_INSTR_A 0xe1200070

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

static volatile union reg_ctx irq_ctx;

/* A fixed vector table - copied to 0x0 */
void rpi2_set_vectable()
{
	asm (
			".globl jumptbl\n\t"
			"b irqset @ Skip the vector table to be copied\n\t"
			"jumpinstr:\n\t"
			"ldr pc, jumptbl    @ reset\n\t"
			"ldr pc, jumptbl+4  @ undef\n\t"
			"ldr pc, jumptbl+8  @ SVC\n\t"
			"ldr pc, jumptbl+12 @ prefetch\n\t"
			"ldr pc, jumptbl+16 @ data\n\t"
			"ldr pc, jumptbl+20 @ not used\n\t"
			"ldr pc, jumptbl+24 @ IRQ\n\t"
			"ldr pc, jumptbl+28 @ FIRQ\n\t"
			"jumptbl: @ 8 addresses\n\t"
			".word loader_main\n\t"
			".word rpi2_undef_handler\n\t"
			".word rpi2_svc_handler\n\t"
			".word rpi2_pabt_handler\n\t"
			".word rpi2_dabt_handler\n\t"
			".word loader_main\n\t @ goto reset"
			".word rpi2_irq_handler\n\t"
			".word rpi2_firq_handler\n\t"
			"irqset:\n\t"
			"@ Copy the vectors\n\t"
			"cpsid aif @ Disable interrupts\n\t"
			"ldr r0, jumpinstr\n\t"
			"eor r1, r1 @ zero r1\n\t"
			"ldr r2, irqset\n\t"
			"irqset_loop:\n\t"
			"ldr r3, [r0], #4 @ word (auto-increment)\n\t"
			"str r3, [r1], #4\n\t"
			"cmp r2, r0\n\t"
			"bmi irqset_loop\n\t"
			"cpsie if @ Enable interrupts\n\t"
	);
}

// returns sp, takes lr subtract value
//
// for fix for the exception return address
// see ARM DDI 0406C.c, ARMv7-A/R ARM issue C p. B1-1173
// Table B1-7 Offsets applied to Link value for exceptions taken to PL1 modes
// UNDEF: 4, SVC: 0, PABT: 4, DABT: 8, IRQ: 4, FIQ: 4

static inline uint32_t push_stackframe(uint32_t var)
{
	asm volatile(
			"push {r0 - r12}\n\t"
			"mov r5, %[var_reg]\n\t"
			"mov r4, lr\n\t"
			"sub r4, r5 @ fix lr\n\t"
			"mrs r5, spsr\n\t"
			"mrs r6, cpsr\n\t"
			"mov r7, sp @ fixed if needed\n\t"
			"push {r4, r5, r6, r7}\n\t"
			"mov %[var_reg], sp\n\t"
			:[var_reg] "=r" (var) : :
	);
	return var;
}

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

			"eor r2,r2  @ clear exception_extra\n\t"
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

// pops a frame from stack and returns
static inline void pop_stackframe()
{
	asm volatile(
			"pop {r4, r5, r6, r7}\n\t"
			"mov sp, r7\n\t"
			"msr cpsr, r6\n\t"
			"msr spsr, r5\n\t"
			"mov lr, r4\n\t"
			"pop {r0 - r12}\n\t"
			"subs pc, lr, #0 @ pc = lr - #0; cpsr = spsr\n\t"
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
// Some of them are split in two parts: the primary and secondary and some
// are working as primary and secondary combined
// The primaries are attributed as 'naked'so that C function prologue
// doesn't disturb the stack before the exception stack frame is pushed
// into the stack, because we return from exception before we get to the
// C function epilogue, and we'd return with unbalanced stack.
// Naked functions shouldn't have any actual C-code in them.
//
// The secondary does the actual work

void rpi2_undef_handler() __attribute__ ((naked));
void rpi2_undef_handler()
{
	static volatile uint32_t sp_value;
	sp_value = push_stackframe(4);
	copy_from_stackframe(sp_value);
	store_banked_regs();

	// exception_info = RPI2_EXC_UNDEF;
	// rpi2_trap_handler();
	asm volatile (
			"push {r0, r1}\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #1 @ RPI2_EXC_UNDEF\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1}\n\t"
			"bl rpi2_trap_handler\n\t"
	);

	load_banked_regs();
	copy_to_stackframe(sp_value);
	pop_stackframe();
}

void rpi2_svc_handler() __attribute__ ((naked));
void rpi2_svc_handler()
{
	static volatile uint32_t sp_value;
	sp_value = push_stackframe(0);
	copy_from_stackframe(sp_value);
	store_banked_regs();

	// exception_info = RPI2_EXC_SVC;
	// rpi2_trap_handler();
	asm volatile (
			"push {r0, r1}\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #2 @ RPI2_EXC_SVC\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1}\n\t"
			"bl rpi2_trap_handler\n\t"
	);

	load_banked_regs();
	copy_to_stackframe(sp_value);
	pop_stackframe();
}

void rpi2_dabt_handler() __attribute__ ((naked));
void rpi2_dabt_handler()
{
	static volatile uint32_t sp_value;
	sp_value = push_stackframe(8);
	copy_from_stackframe(sp_value);
	store_banked_regs();

	// exception_info = RPI2_EXC_DABT;
	// rpi2_trap_handler();
	asm volatile (
			"push {r0, r1}\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #4 @ RPI2_EXC_DABT\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1}\n\t"
			"bl rpi2_trap_handler\n\t"
	);

	load_banked_regs();
	copy_to_stackframe(sp_value);
	pop_stackframe();
}

void rpi2_irq_handler2(uint32_t stack_frame_addr)
{

	// IRQ2 pending
	if (*((volatile uint32_t *)IRC_PENDB) & (1<<9))
	{
		// UART IRQ
		if (*((volatile uint32_t *)IRC_PEND2) & (1<<25))
		{
			serial_irq();
		}
		else
		{
			// unhandled IRQ
			copy_from_stackframe(stack_frame_addr);
			store_banked_regs();

			exception_info = RPI2_EXC_IRQ;
			rpi2_trap_handler();

			load_banked_regs();
			copy_to_stackframe(stack_frame_addr);
		}
	}
	else
	{
		// unhandled IRQ
		copy_from_stackframe(stack_frame_addr);
		store_banked_regs();

		exception_info = RPI2_EXC_IRQ;
		rpi2_trap_handler();

		load_banked_regs();
		copy_to_stackframe(stack_frame_addr);
	}
}

void rpi2_irq_handler() __attribute__ ((naked));
void rpi2_irq_handler()
{
	static volatile uint32_t sp_value;
	sp_value = push_stackframe(4);

	// rpi2_irq_handler2(sp_value);
	asm volatile (
			"ldr r0, [%[spv]]\n\t"
			"bl rpi2_irq_handler2\n\t"
			:[spv] "=r"(sp_value) : :
	);

	pop_stackframe();
}

void rpi2_firq_handler() __attribute__ ((naked));
void rpi2_firq_handler()
{
	static volatile uint32_t sp_value;
	sp_value = push_stackframe(4);
	copy_from_stackframe(sp_value);
	store_banked_regs();

	// exception_info = RPI2_EXC_FIQ;
	// rpi2_trap_handler();
	asm volatile (
			"push {r0, r1}\n\t"
			"ldr r0, =exception_info\n\t"
			"mov r1, #7 @ RPI2_EXC_FIQ\n\t"
			"str r1, [r0]\n\t"
			"pop {r0, r1}\n\t"
			"bl rpi2_trap_handler\n\t"
	);

	load_banked_regs();
	copy_to_stackframe(sp_value);
	pop_stackframe();
}

void rpi2_pabt_handler2(uint32_t stack_frame_addr)
{
	uint32_t *p;

	copy_from_stackframe(stack_frame_addr);
	store_banked_regs();

	exception_info = RPI2_EXC_PABT;
	// TODO: check
	// MRC p15, 0, <Rt>, c5, c0, 1 ; Read IFSR into Rt
	// if bit 10=0 and bits 3-0=2 -> breakpoint

	// if pc points to 'bkpt', trap, otherwise fetch abort
	p = (uint32_t *)(&(rpi2_reg_context.reg.r15));
	if (*p == TRAP_INSTR_A)
	{
		exception_extra = 1; // trap (ARM)
	}
	else if (*(uint16_t *)p == TRAP_INSTR_T)
	{
		exception_extra = 2; // trap (THUMB)
	}
	rpi2_trap_handler();

	load_banked_regs();
	copy_to_stackframe(stack_frame_addr);
}

void rpi2_pabt_handler() __attribute__ ((naked));
void rpi2_pabt_handler()
{
	static volatile uint32_t sp_value;
	sp_value = push_stackframe(4);

	// rpi2_pabt_handler2(sp_value);
	asm volatile (
			"ldr r0, [%[spv]]\n\t"
			"bl rpi2_pabt_handler2\n\t"
			:[spv] "=r"(sp_value) : :
	);

	pop_stackframe();
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
	asm("bkpt #0\n\t");
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
}

void rpi2_init()
{
	exception_info = RPI2_EXC_NONE;
	exception_extra = 0;
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
