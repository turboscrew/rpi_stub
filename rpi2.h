/*
 * rpi2.h
 *
 *  Created on: Mar 31, 2015
 *      Author: jaa
 */

#ifndef RPI2_H_
#define RPI2_H_

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

// exception info
// 8x = SW-caused
extern volatile int exception_info;
extern volatile int exception_extra;

// register context
/*
Link values saved on exception entry (ARM-ARM, B1.8.3 Overview of exception entry)
Exception				Preferred return address
Undefined Instruction 	Address of the UNDEFINED instruction
Supervisor Call 		Address of the instruction after the SVC instruction
Prefetch Abort 			Address of aborted instruction fetch
Data Abort 				Address of instruction that generated the abort
IRQ or FIQ 				Address of next instruction to execute
 */
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
