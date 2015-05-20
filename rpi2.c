/*
 * rpi2.c
 *
 *  Created on: Mar 31, 2015
 *      Author: jaa
 */


#include <stdint.h>

// Not pretty, but didn't want to include everything
extern void serial_irq();
extern void gdb_trap_handler();
extern void main(uint32_t r0, uint32_t r1, uint32_t r2);

// Another naughty trick
#define extern
#include "rpi2.h"
#undef extern

extern uint32_t jumptbl;
// The peripherals base address
#define PERIPH_BASE 0x3f000000

// The IRC base address
#define IRC_BASE (PERIPH_BASE + 0xB000)

// The IRC registers
#define IRC_PENDB	(IRC_BASE + 0x200)
#define IRC_PEND1	(IRC_BASE + 0x204)
#define IRC_PEND2	(IRC_BASE + 0x208)
#define IRC_FIQCTRL (IRC_BASE + 0x20C)
#define IRC_ENB		(IRC_BASE + 0x218)
#define IRC_EN1		(IRC_BASE + 0x210)
#define IRC_EN2		(IRC_BASE + 0x214)
#define IRC_DISB	(IRC_BASE + 0x224)
#define IRC_DIS1	(IRC_BASE + 0x21C)
#define IRC_DIS2	(IRC_BASE + 0x220)


#define TRAP_INSTR_T 0xbe00
#define TRAP_INSTR_A 0xe1200070

// ISRs
void rpi2_undef_handler() __attribute__ ((interrupt));
void rpi2_svc_handler() __attribute__ ((interrupt));
void rpi2_dabt_handler() __attribute__ ((interrupt));
void rpi2_irq_handler() __attribute__ ((interrupt));
void rpi2_firq_handler() __attribute__ ((interrupt));

/* A fixed vector table - copied to 0x0 */
void rpi2_set_vectable()
{
	asm (
			".globl jumptbl\n\t"
			"b irqset @ Skip the vector entry to be copied\n\t"
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
			".word 0x1f000000 @ main\n\t"
			".word rpi2_undef_handler\n\t"
			".word rpi2_svc_handler\n\t"
			".word rpi2_pabt_handler\n\t"
			".word rpi2_dabt_handler\n\t"
			".word 0x0\n\t @ goto reset"
			".word rpi2_irq_handler\n\t"
			".word rpi2_firq_handler\n\t"
			"irqset:\n\t"
			"@ Copy the vectors\n\t"
			"cpsid i @ Disable interrupts\n\t"
			"ldr r0, jumpinstr\n\t"
			"eor r1, r1 @ zero r1\n\t"
			"ldr r2, irqset\n\t"
			"irqset_loop:\n\t"
			"ldr r3, [r0], #4 @ word auto-increment\n\t"
			"str r3, [r1], #4\n\t"
			"cmp r2, r0\n\t"
			"bhi irqset_loop\n\t"
			"cpsie i @ Enable interrupts\n\t"
	);
}

// must be first action in an exception handler
static inline void save_regs()
{
	asm volatile(
			"str r0, [sp], #-4 @ push r0 - r0 is needed here\n\t"
			"ldr r0, =rpi2_reg_context @ r0 -> reg_context\n\t"
			"@ save the regs - we'll fix r0 and sp later\n\t"
			"stmia r0, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}\n\t"
			"str r13, [r0, #+13*4] @ SP not allowed in thumb-stmia\n\t"
			"str r14, [r0, #+14*4] @ SP not allowed in thumb-stmia\n\t"
			"@ two more working registers, now that they are already saved\n\t"
			"str r1, [sp], #-4 @ push r1\n\t"
			"str r2, [sp], #-4 @ push r2\n\t"
			"eor r2,r2  @ clear exception_extra\n\t"
			"ldr r1, =exception_extra\n\t"
			"str r2, [r1]\n\t"
			"@ fix stored sp - we did push r0 before saving sp\n\t"
			"ldr r1, [r0, #+14*4] @ saved sp\n\t"
			"add r1, #4 @ r0\n\t"
			"str r1, [r0, #+14*4]\n\t"
			"@ fix r0 which was pushed at the beginning\n\t"
			"ldr r1, [sp, #+12] @ load old r0\n\t"
			"str r1, [r0] @ save r0\n\t"
			"@ save old pc\n\t"
			"str lr, [r0, #+15*4]\n\t"
			"mrs r1, spsr @ save old cpsr\n\t"
			"str r1, [r0, #+16*4]\n\t"
			"@ find out previous mode for getting old SP + lr\n\t"
			"@ for debugging only - no need to restore them later\n\t"
			"mrs r1, cpsr @ store our cpsr to stack\n\t"
			"str r1, [sp], #-4\n\t"
			"@ cpsr with zeroed mode into r2 for easier mode change\n\t"
			"ldr r2, =#0xfffffff0 @ clean up mode bits\n\t"
			"and r2, r1 @ store mode bit masked cpsr in r2\n\t"
			"@ check original mode field to find out the old bank\n\t"
			"@ and switch into that mode to get lr and sp\n\t"
			"@ getting sp and lr and switching back to our mode is common code\n\t"
			"and r1, #0x0f @ extract mode field (bit 4 always set)\n\t"
			"@ handle FIQ mode\n\t"
			"cmp r1, #0x01 @ FIQ?\n\t"
			"bne 1f\n\t"
			"@ handle FIQ mode specific banked registers\n\t"
			"orr r1, r2, #0x01 @ set FIQ mode\n\t"
			"msr cpsr, r1 @ \n\t"
			"@ store banked regs 8 - 12\n\t"
			"ldr r0, =rpi2_reg_context+8*4 @ r0 -> reg_context.r8\n\t"
			"stmia r0, {r8,r9,r10,r11,r12}\n\t"
			"ldr r0, =rpi2_reg_context	@ r0 -> reg_context\n\t"
			"b 8f\n\t"
			"@ handle user and system modes - common sp and lr\n\t"
			"1:\n\t"
			"cmp r1, #0x00 @ user?\n\t"
			"bne 2f\n\t"
			"cmp r1, #0x0f @ system?\n\t"
			"bne 2f\n\t"
			"@ usr and sys use same registers, but sys is privileged\n\t"
			"@ from user mode we couldn't get back\n\t"
			"orr r1, r2, #0x0f @ set system mode\n\t"
			"msr cpsr, r1 @ \n\t"
			"b 8f\n\t"
			"@ handle IRQ and SVC modes - common sp and lr\n\t"
			"2:\n\t"
			"cmp r1, #0x02 @ IRQ?\n\t"
			"b 3f @ IRQ and SVC use same registers\n\t"
			"cmp r1, #0x03 @ SVC??\n\t"
			"b 3f @ IRQ and SVC use same registers\n\t"
			"orr r1, r2, #0x03 @ set SVC mode\n\t"
			"msr cpsr_c, r1 @ \n\t"
			"b 8f\n\t"
			"@ handle monitor mode\n\t"
			"3:\n\t"
			"cmp r1, #0x06 @ monitor?\n\t"
			"bne 4f\n\t"
			"orr r1, r2, #0x06 @ set monitor mode\n\t"
			"msr cpsr_c, r1 @ \n\t"
			"b 8f\n\t"
			"@ handle hyp mode\n\t"
			"4:\n\t"
			"cmp r1, #0x0a @ hyp?\n\t"
			"bne 5f\n\t"
			"orr r1, r2, #0x0a @ set hyp mode\n\t"
			"msr cpsr_c, r1 @ \n\t"
			"b 8f\n\t"
			"@ handle und mode\n\t"
			"5:\n\t"
			"cmp r1, #0x0b @ und?\n\t"
			"bne 6f\n\t"
			"orr r1, r2, #0x0b @ set und mode\n\t"
			"msr cpsr_c, r1 @ \n\t"
			"b 8f\n\t"
			"@ handle abort modes (also our mode)\n\t"
			"6:\n\t"
			"cmp r1, #0x07 @ abort?\n\t"
			"bne 7f\n\t"
			"@ also abort mode\n\t"
			"b 8f\n\t"
			"@ illegal mode\n\t"
			"7:\n\t"
			"@ we've run out of modes - something is very wrong\n\t"
			"ldr r1, =0x7fffff00 @ notify debugger\n\t"
			"str r0, [sp], #-4 @ push r0\n\t"
			"ldr r0, =exception_extra\n\t"
			"str r1, [r0]\n\t"
			"ldr r0, [sp, #+4]! @ pop r0\n\t"
			"@ get sp and lr - common code\n\t"
			"8:\n\t"
			"@ if our mode, skip\n\t"
			"ldr r1, [sp, #+4]\n\t"
			"and r1, #0x0f @ extract mode field\n\t"
			"cmp r1, #0x07 @ abort?\n\t"
			"beq 9f\n\t"
			"@ else save old sp and lr\n\t"
			"mov r1, lr @ save old lr\n\t"
			"str r1, [r0, #+14*4]\n\t"
			"mov r1, sp @ save old sp\n\t"
			"str r1, [r0, #+13*4]\n\t"
			"@ back to our mode\n\t"
			"9:\n\t"
			"@ WHOSE STACK WE PULL FROM? \n\t"
			"ldr r1, [sp, #+4]! @ restore our cpsr\n\t"
			"msr cpsr, r1 @ \n\t"
			"@ restore working registers (just in case)"
			"ldr r2, [sp, #+4]! @ pop r2\n\t"
			"ldr r1, [sp, #+4]! @ pop r1\n\t"
			"ldr r0, [sp, #+4]! @ pop r0\n\t"
	);
}

// must be last action in an exception handler
static inline void load_regs()
{
	asm volatile(
			"@ load common regs\n\t"
			"ldr r0, =rpi2_reg_context\n\t"
			"add r0, #4 @ r1\n\t"
			"@ restoring sp matters in this mode only\n\t"
			"@ in this mode we've already lost lr - overwritten by exception"
			"@ in other modes lr and sp are banked - and safe\n\t"
			"ldmia r0, {r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}\n\t"
			"ldr r13, [r0, #+13*4] @ SP not recommended in ldmia\n\t"
			"@ pushing r0 and r1 alter the loaded sp, but it becomes restored\n\t"
			"@ r1 is needed for pushing r0\n\t"
			"push {r1}\n\t"
			"ldr r0, =rpi2_reg_context\n\t"
			"push {r0}\n\t"
			"@ restore spsr\n\t"
			"ldr r1, [r0, #+16*4]\n\t"
			"msr spsr, r1 @ \n\t"
			"@ load old pc (stored pc to current lr)\n\t"
			"ldr lr, [r0, #+15]\n\t"
			"pop {r0}\n\t"
			"pop {r1}\n\t"
			"subs pc, lr, #0 @ pc = lr - #0; cpsr = spsr\n\t"
	);
}

// exception handlers

// common trap handler outside exception handlers
void rpi2_trap_handler()
{
	// IRQs need to be enabled for serial I/O
	asm volatile (
			"push {r0}\n\t"
			"mrs r0, cpsr\n\t"
			"bic r0, #128 @ enable irqs\n\t"
			"msr cpsr, r0\n\t"
			"pop {r0}\n\t"
	);
	gdb_trap_handler();
}

void rpi2_undef_handler()
{
	save_regs();
	exception_info = RPI2_EXC_UNDEF;
	rpi2_trap_handler();
	load_regs();
}

void rpi2_svc_handler()
{
	save_regs();
	exception_info = RPI2_EXC_SVC;
	rpi2_trap_handler();
	load_regs();
}

void rpi2_dabt_handler()
{
	save_regs();
	// unhandled dabt
	exception_info = RPI2_EXC_DABT;
	rpi2_trap_handler();
	load_regs();
}

void rpi2_irq_handler()
{
	save_regs();
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
			exception_info = RPI2_EXC_IRQ;
			rpi2_trap_handler();
		}
	}
	else
	{
		// unhandled IRQ
		exception_info = RPI2_EXC_IRQ;
		rpi2_trap_handler();
	}
	load_regs();
}

void rpi2_firq_handler()
{
	save_regs();
	// unhandled FIQ
	exception_info = RPI2_EXC_FIQ;
	rpi2_trap_handler();
	load_regs();
}

void rpi2_pabt_handler()
{
	uint32_t *p;
	save_regs();
	exception_info = RPI2_EXC_PABT;
	// if pc points to 'bkpt', trap, otherwise fetch abort
	p = (uint32_t *)(&(rpi2_reg_context.reg.r15));
	if (*p == TRAP_INSTR_A)
	{
		exception_extra |= 1; // trap (ARM)
	}
	else if (*(uint16_t *)p == TRAP_INSTR_T)
	{
		exception_extra |= 2; // trap (THUMB)
	}
	rpi2_trap_handler();
	load_regs();
}

void rpi2_set_vector(int excnum, void *handler)
{
	/*
	 * If debug-mode is disabled, BKPT causes
	 * Prefetch Abort exception - vector table address 0x0c
	 */
	uint32_t *vector = (uint32_t *)jumptbl;
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
	uint16_t *location = (uint16_t *)address;
	/* poke BKPT instruction */
	*(location++) = 0xBE00; // BKPT
	*(location) = 0xBE00; // BKPT
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
