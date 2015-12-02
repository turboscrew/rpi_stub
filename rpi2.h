/*
rpi2.h

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

#ifndef RPI2_H_
#define RPI2_H_

// needs to be here to be visible to gdb, rpi2 and serial
#define DEBUG_GDB_EXC
//#define DEBUG_CTRLC

#define DEBUG_UNDEF
//#define DEBUG_SVC
//#define DEBUG_AUX
//#define DEBUG_DABT
#define DEBUG_DABT_UNHANDLED
//#define DEBUG_IRQ
//#define DEBUG_FIQ
#define DEBUG_PABT
#define DEBUG_PABT_UNHANDLED

//#define RPI2_DEBUG_TIMER

#define RPI2_NEON_SUPPORTED
// The peripherals base address
#define PERIPH_BASE 0x3f000000

// system timer low
#define SYSTMR_CLO 0x3f003004

// The GPIO registers base address.
#define GPIO_BASE (PERIPH_BASE + 0x200000)

#define GPFSEL1 (GPIO_BASE + 0x04)	// The GPIO function select (for pins 14,15)
#define GPFSEL4 (GPIO_BASE + 0x10)	// The GPIO function select (for pin 47)
#define GPIO47_SELBITS (21)	// The GPIO function select (for pin 47)
#define GPIO_SETREG0 (GPIO_BASE + 0x1c)	// The GPIO set register (for pin 47)
#define GPIO_SETREG1 (GPIO_BASE + 0x20)	// The GPIO set register (for pin 47)
#define GPIO_CLRREG0 (GPIO_BASE + 0x28)	// The GPIO clear register (for pin 47)
#define GPIO_CLRREG1 (GPIO_BASE + 0x2c)	// The GPIO clear register (for pin 47)
#define GPIO47_MASK (1 << 15)	// The GPIO47 output mask (for pin 47)

#define GPPUD (GPIO_BASE + 0x94)	// Pull up/down for all GPIO pins
#define GPPUDCLK0 (GPIO_BASE + 0x98) // Pull up/down for specific GPIO pin
#define GPPUDCLK1 (GPIO_BASE + 0x9C) // Pull up/down for specific GPIO pin

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

/* ARM timer definitions */
#define ARM_TIMER_BASE	(PERIPH_BASE + 0xB000)
#define ARM_TIMER_LOAD	(ARM_TIMER_BASE + 0x400)
#define ARM_TIMER_VALUE	(ARM_TIMER_BASE + 0x404)
#define ARM_TIMER_CTL	(ARM_TIMER_BASE + 0x408)
#define ARM_TIMER_CLI	(ARM_TIMER_BASE + 0x40C)
#define ARM_TIMER_RIS	(ARM_TIMER_BASE + 0x410)
#define ARM_TIMER_MIS	(ARM_TIMER_BASE + 0x414)
#define ARM_TIMER_RELOAD (ARM_TIMER_BASE + 0x418)
#define ARM_TIMER_DIV	(ARM_TIMER_BASE + 0x41C)
#define ARM_TIMER_CNT	(ARM_TIMER_BASE + 0x420)

/* UART0 definitions */

// The base address for UART.
#define UART0_BASE (PERIPH_BASE + 0x201000)

// The UART registers
#define UART0_DR     (UART0_BASE + 0x00)
#define UART0_RSRECR (UART0_BASE + 0x04)
#define UART0_FR     (UART0_BASE + 0x18)
#define UART0_ILPR   (UART0_BASE + 0x20)
#define UART0_IBRD   (UART0_BASE + 0x24)
#define UART0_FBRD   (UART0_BASE + 0x28)
#define UART0_LCRH   (UART0_BASE + 0x2C)
#define UART0_CR     (UART0_BASE + 0x30)
#define UART0_IFLS   (UART0_BASE + 0x34)
#define UART0_IMSC   (UART0_BASE + 0x38)
#define UART0_RIS    (UART0_BASE + 0x3C)
#define UART0_MIS    (UART0_BASE + 0x40)
#define UART0_ICR    (UART0_BASE + 0x44)
#define UART0_DMACR  (UART0_BASE + 0x48)
#define UART0_ITCR   (UART0_BASE + 0x80)
#define UART0_ITIP   (UART0_BASE + 0x84)
#define UART0_ITOP   (UART0_BASE + 0x88)
#define UART0_TDR    (UART0_BASE + 0x8C)

// definitions for the undocumented mailbox
#define MBOX_BASE (PERIPH_BASE + 0xB800)
// MBOX0: GPU -> ARM
#define MBOX0_READ (MBOX_BASE + 0x80)
#define MBOX0_POLL (MBOX_BASE + 0x90)
#define MBOX0_CONF (MBOX_BASE + 0x9c)
#define MBOX0_STATUS (MBOX_BASE + 0x98)
// MBOX1 ARM -> GPU
#define MBOX1_WRITE (MBOX_BASE + 0xA0)
#define MBOX1_STATUS (MBOX_BASE + 0xB8)

#define MBOX_STATUS_FULL (1 << 31)
#define MBOX_STATUS_EMPTY (1 << 30)

// Clock IDs for mailbox queries

#define CLOCK_RESERVED 0
#define CLOCK_EMMC  1
#define CLOCK_UART  2
#define CLOCK_ARM   3
#define CLOCK_CORE  4
#define CLOCK_V3D   5
#define CLOCK_H264  6
#define CLOCK_ISP   7
#define CLOCK_SDRAM 8
#define CLOCK_PIXEL 9
#define CLOCK_PWM  10

typedef struct
{
	unsigned int buff_size;
	unsigned int req_resp_code;
	unsigned int tag;
	unsigned int tag_buff_len;
	unsigned int tag_msg_len;
	union {
		unsigned char byte_buff[1024];
		unsigned int word_buff[256];
	} tag_buff;
	unsigned int end_tag;
} request_response_t;

/* exception_info values
   exception numbers - these had to be hardcoded in the
   exception handlers' assembly code in rpi2.c */
#define RPI2_EXC_RESET	0
#define RPI2_EXC_UNDEF	1
#define RPI2_EXC_SVC	2
#define RPI2_EXC_PABT	3
#define RPI2_EXC_DABT	4
#define RPI2_EXC_AUX	5
#define RPI2_EXC_IRQ	6
#define RPI2_EXC_FIQ	7
#define RPI2_EXC_TRAP	3
#define RPI2_EXC_NONE	8

#define RPI2_INTERNAL_BKPT 0xe127ff7e
#define RPI2_INITIAL_BKPT 0xe127ff7d
#define RPI2_USER_BKPT_ARM 0xe127ff7f
#define RPI2_USER_BKPT_THUMB 0xbebe

// exception_extra values used in PABT
#define RPI2_EXTRA_NOTHING 0
// our ARM-breakpoint
#define RPI2_TRAP_ARM 1
// our THUMB-breakpoint
#define RPI2_TRAP_THUMB 2
// our internal breakpoint
#define RPI2_TRAP_PABT 3
// user bkpt
#define RPI2_TRAP_BKPT 4
// prefetch abort
#define RPI2_TRAP_BUS 5
// watchpoint
#define RPI2_TRAP_WATCH 6
// logging - null terminated
#define RPI2_TRAP_LOGZ 13
// logging - size in r1
#define RPI2_TRAP_LOGN 14
// our initial breakpoint
#define RPI2_TRAP_INITIAL 15

// for special traps to gdb
#define RPI2_REASON_SIGINT 2
#define RPI2_REASON_HW_EXC 30
#define RPI2_REASON_SW_EXC 31

#define SYNC asm volatile ("dsb\n\tisb\n\t":::"memory")

// exception info
typedef struct
{
	unsigned int far;
	unsigned int fsr;
	unsigned int dbgdscr;
	unsigned int exc_lr;
}debug_exc_rec_t;

extern debug_exc_rec_t rpi2_dbg_rec;

// 8x = SW-caused
extern volatile int exception_info;
extern volatile int exception_extra;

#define RPI2_UART0_POLL 0
#define RPI2_UART0_FIQ 1
#define RPI2_UART0_IRQ 2

extern unsigned int rpi2_arm_ramsize; // ARM ram in megs
extern unsigned int rpi2_arm_ramstart; // ARM ram start address
extern unsigned int rpi2_uart_clock;
extern unsigned int rpi2_neon_used;
extern unsigned int rpi2_neon_enable;

// command line parameters
extern unsigned int rpi2_keep_ctrlc; // ARM ram start address
extern unsigned int rpi2_uart0_excmode;
extern unsigned int rpi2_uart0_baud;
extern unsigned int rpi2_use_mmu;
extern unsigned int rpi2_use_hw_debug;
extern unsigned int rpi2_print_dbg_info;

// register context
// for lr in exception, see pages B1-1172 and B1-1173 of
// ARMv7-A/R ARM issue C (ARM DDI 0406C.c)
typedef union reg_ctx {
	unsigned int storage[18]; // without size it would be flexible array member
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
		unsigned int spsr;
	} reg;
} rpi2_reg_context_t;

extern volatile rpi2_reg_context_t rpi2_reg_context;

typedef struct neon_ctx {
	unsigned long long storage[32]; // 256 bytes
	unsigned int fpscr;
	// must be stored/restored, but not sent to gdb
	unsigned int fpexc;
#if 0
	// not implemented in this chip
	unsigned int fpinst;
	unsigned int fpinst2;
#endif
} rpi2_neon_ctx_t;

extern volatile __attribute__ ((aligned (8))) rpi2_neon_ctx_t rpi2_neon_context;

unsigned int rpi2_disable_save_ints();
void rpi2_restore_ints(unsigned int status);
void rpi2_disable_excs();
void rpi2_enable_excs();
void rpi2_disable_ints();
void rpi2_enable_ints();
void set_gdb_enabled(unsigned int state);
void rpi2_get_cmdline(char *line);

void rpi2_set_vectors();
void rpi2_enable_mmu();
void rpi2_flush_address(unsigned int addr);
void rpi2_invalidate_caches();
void rpi2_trap();
void rpi2_gdb_trap();
void rpi2_init();
void rpi2_set_trap(void *address, int kind);
void rpi2_pend_trap();

void rpi2_timer_kick();
void rpi2_timer_start();
void rpi2_timer_stop();

// access functions
unsigned int rpi2_get_sigint_flag();
void rpi2_set_sigint_flag(unsigned int val);
unsigned int rpi2_get_exc_reason();
void rpi2_set_exc_reason(unsigned int val);

/* for debugging */

void rpi2_unset_watchpoint(unsigned int num);
void rpi2_set_watchpoint(unsigned int num, unsigned int addr, unsigned int range);

// ACT-led: gpio 47, active high
void rpi2_init_led();
void rpi2_led_off();
void rpi2_led_on();
void rpi2_delay_loop(unsigned int delay_ms);
void rpi2_led_blink(unsigned int on_ms, unsigned int off_ms, unsigned int count);

// debug-debug
void rpi2_check_debug();
#endif /* RPI2_H_ */
