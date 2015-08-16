/*
 * rpi2.h
 *
 *  Created on: Mar 31, 2015
 *      Author: jaa
 */

#ifndef RPI2_H_
#define RPI2_H_

// The peripherals base address
#define PERIPH_BASE 0x3f000000

// The GPIO registers base address.
#define GPIO_BASE (PERIPH_BASE + 0x200000)

#define GPFSEL1 (GPIO_BASE + 0x04)	// The GPIO function select (for pins 14,15)
#define GPPUD (GPIO_BASE + 0x94)	// Pull up/down for all GPIO pins
#define GPPUDCLK0 (GPIO_BASE + 0x98) // Pull up/down for specific GPIO pin

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

/* exception numbers */
#define RPI2_EXC_RESET	0
#define RPI2_EXC_UNDEF	1
#define RPI2_EXC_SVC	2
#define RPI2_EXC_PABT	3
#define RPI2_EXC_DABT	4
#define RPI2_EXC_UNUSED	5
#define RPI2_EXC_IRQ	6
#define RPI2_EXC_FIQ	7
#define RPI2_EXC_TRAP	3
#define RPI2_EXC_NONE	8

#define RPI2_TRAP_ARM 1
#define RPI2_TRAP_THUMB 2

// for reading and writing registers
// also guarantee 32-bit reads and writes
#define PUT32(address, data) \
    asm volatile (\
    "str %src, [%dst] \n\t"\
    :src "=r" (data)\
    :dst "r" (address):\
    )

#define GET32(address, data) \
    asm volatile (\
    "ldr %dst, [%src] \n\t"\
    :dst "=r" (data)\
    :src "r" (address):\
    )

// interrupt disable/enable
#define DISABLE_INTS asm volatile ("cpsid aif\n\t")
#define ENABLE_INTS asm volatile ("cpsie aif\n\t")

// exception info
// 8x = SW-caused
extern volatile int exception_info;
extern volatile int exception_extra;

// register context
// for lr in exception, see pages B1-1172 and B1-1173 of
// ARMv7-A/R ARM issue C (ARM DDI 0406C.c)
extern volatile union {
	unsigned int storage[17]; // without size it would be flexible array member
	struct {
		unsigned int r0;
		unsigned int r1;
		unsigned int r2;
		unsigned int r3;
		unsigned int r4;
		unsigned int r5;
		unsigned int r6;
		unsigned int r7;
		unsigned int r8;
		unsigned int r9;
		unsigned int r10;
		unsigned int r11;
		unsigned int r12;
		unsigned int r13; // SP
		unsigned int r14; // LR
		unsigned int r15; // PC
		unsigned int cpsr;
	} reg;
} rpi2_reg_context;

void rpi2_set_vector(int excnum, void *handler);
void rpi2_trap();
void rpi2_init();
void rpi2_set_trap(void *address, int kind);
void rpi2_pend_trap();

#endif /* RPI2_H_ */
