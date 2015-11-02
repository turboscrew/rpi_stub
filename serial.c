/*
serial.c

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
#include "rpi2.h"
#include "serial.h"

// I/O buffers, we write to tail and read from head
// buffers are post-incrementing
#define SER_RX_BUFF_SIZE 1024
#define SER_TX_BUFF_SIZE 1024

volatile int ser_rx_head;
volatile int ser_rx_tail;
volatile char ser_rx_buff[SER_RX_BUFF_SIZE];

volatile int ser_tx_head;
volatile int ser_tx_tail;
volatile char ser_tx_buff[SER_TX_BUFF_SIZE];

volatile uint32_t ser_rx_dropped_count;
volatile uint32_t ser_rx_ovr_count;

volatile int ser_handle_ctrlc = 0;
void (*ser_ctrlc_handler)();
volatile int ser_ctrl_c = 0;

void serial_irq();
void serial_rx();
void serial_tx();
void serial_poll();

/* delay() borrowed from OSDev.org */
static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : : [count]"r"(count) : "cc");
}

static inline uint32_t disable_save_ints()
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

static inline void restore_ints(uint32_t status)
{
	asm volatile (
			"msr cpsr_fsxc, %[var_reg]\n\t"
			"dsb\n\t"
			"isb\n\t"
			::[var_reg] "r" (status):
	);
}

unsigned int serial_get_rx_dropped()
{
	unsigned int retval;
	uint32_t cpsr_store;

	cpsr_store = disable_save_ints();
	retval = (unsigned int)ser_rx_dropped_count;
	ser_rx_dropped_count = 0;
	restore_ints(cpsr_store);

	return retval;
}

unsigned int serial_get_rx_ovr()
{
	uint32_t retval;
	uint32_t cpsr_store;

	cpsr_store = disable_save_ints();
	retval = ser_rx_ovr_count;
	ser_rx_ovr_count = 0;
	restore_ints(cpsr_store);
	return (unsigned int)retval;
}

int serial_get_ctrl_c()
{
	int retval;
	retval = ser_ctrl_c;
	ser_ctrl_c = 0;
	return retval;
}

void serial_enable_ctrlc()
{
	ser_handle_ctrlc = 1;
	SYNC;
}

void serial_disable_ctrlc()
{
	ser_handle_ctrlc = 0;
	SYNC;
}


void serial_init(io_device *device)
{
	uint32_t tmp; // scratchpad
	uint32_t cpsr_store;

#if 0
	for (tmp=0; tmp < 2; tmp++)
	{
		rpi2_led_blink(100, 100, 3);
		rpi2_delay_loop(500);
	}
	rpi2_delay_loop(2000);
#endif


	// no messing with interrupt
	cpsr_store = disable_save_ints();

#if 0
	for (tmp=0; tmp < 3; tmp++)
	{
		rpi2_led_blink(100, 100, 3);
		rpi2_delay_loop(500);
	}
	rpi2_delay_loop(2000);
#endif

	// Disable uart interrupts
	if (rpi2_uart0_excmode == RPI2_UART0_FIQ)
	{
		*((volatile uint32_t *)IRC_FIQCTRL) &= ~(1 << 7);
	}
	else
	{
		// Writing a 1 to a bit will clear the corresponding IRQ enable bit.
		*((volatile uint32_t *)IRC_DIS2) = (1 << 25);
	}

	// Disable UART0 for configuration - sorry, took the only "real" UART,
	// but with that, uploading is faster and so is debugging
	*((volatile uint32_t *)UART0_CR) = 0x00000000;

	SYNC;

	// Flush FIFOs by disabling them
	*((volatile uint32_t *)UART0_LCRH) = 0;

	// Fill up the device structure
	device->get_char = serial_get_char;
	rpi2_flush_address((unsigned int) &(device->get_char));
	device->get_string = serial_get_string;
	rpi2_flush_address((unsigned int) &(device->get_string));
	device->put_char = serial_put_char;
	rpi2_flush_address((unsigned int) &(device->put_char));
	device->put_string = serial_put_string;
	rpi2_flush_address((unsigned int) &(device->put_string));
	device->read = serial_read;
	rpi2_flush_address((unsigned int) &(device->read));
	device->write = serial_write;
	rpi2_flush_address((unsigned int) &(device->write));
	device->start = serial_start;
	rpi2_flush_address((unsigned int) &(device->start));
	device->enable_ctrlc = serial_enable_ctrlc;
	rpi2_flush_address((unsigned int) &(device->enable_ctrlc));
	device->disable_ctrlc = serial_disable_ctrlc;
	rpi2_flush_address((unsigned int) &(device->enable_ctrlc));

	// Clear ring buffers
	ser_rx_head = 0;
	ser_rx_tail = 0;
	ser_tx_head = 0;
	ser_tx_tail = 0;
	ser_rx_dropped_count = 0;
	ser_rx_ovr_count = 0;

	// init ctrl-C handling
	ser_handle_ctrlc = 0;
	ser_ctrl_c = 0;

#if 0
	for (tmp=0; tmp < 4; tmp++)
	{
		rpi2_led_blink(100, 100, 3);
		rpi2_delay_loop(500);
	}
	rpi2_delay_loop(2000);
#endif
	// Setup the GPIO pin 14 & 15.
	// Change pull up/down to pull-down & delay for 150 cycles
	*((volatile uint32_t *)GPPUD) = 0x00000002; // pull-up for pins 14 & 15
	SYNC;
	delay(150);

	// Target pull up/down change to pin 14,15 & delay for 150 cycles.
	*((volatile uint32_t *)GPPUDCLK0) = ((1 << 14) | (1 << 15));
	SYNC;
	delay(150);

	// Change pull up/down to disabled & delay for 150 cycles
	*((volatile uint32_t *)GPPUD) = 0x00000000; // pull-down for pins 14 & 15

	// Write 0 to GPPUDCLK0 to 'un-target' the pull up/down change
	*((volatile uint32_t *)GPPUDCLK0) = 0x00000000;
	SYNC;

#if 0
	rpi2_led_blink(100, 100, 3);
	rpi2_delay_loop(1000);
#endif

	// GPIO 14,15 to UART0 rx & tx
	tmp = *((volatile uint32_t *)GPFSEL1);
	tmp &= ~(7<<(4 * 3) | 7<<(5 * 3)); // clear the functions for pins 14 and 15
	tmp |= 0x4 << (4 * 3); // pin 14 - the 4th 3-bit group, 4 = alt function 0			tmp = 0x4 << (4 * 3) // pin 14 - the 4th 3-bit group, 4 = alt function 0
	tmp |= 0x4 << (5 * 3); // pin 15 - the 5th 3-bit group
	*((volatile uint32_t *)GPFSEL1) = tmp;


	// Configure UART0

	// Set integer & fractional part of baud rate (115200 baud)
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200

	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1
	// Fractional part register = (0.627 * 64) + 0.5 = 40.6 = ~40
	*((volatile uint32_t *)UART0_IBRD) = 1;
	*((volatile uint32_t *)UART0_FBRD) = 40;

	// Enable FIFO & 8 bit data transmission (1 stop bit, no parity)
	*((volatile uint32_t *)UART0_LCRH) = (1 << 4) | (1 << 5) | (1 << 6);

	// FIFO-interrupt levels: rx 1/2, tx 1/2
	//*((volatile uint32_t *)UART0_IFLS) = (0x2 << 3) | (0x2) ;
	*((volatile uint32_t *)UART0_IFLS) = (0x3 << 3) | (0x1) ;
	SYNC;

	// Enable UART0, receive & transfer part of UART
	*((volatile uint32_t *)UART0_CR) = (1 << 0) | (1 << 8) | (1 << 9);
	SYNC;

	// Clear interrupts
	*((volatile uint32_t *)UART0_ICR) = 0x7ff;
	SYNC;

	// Set UART0 interrupt mask
	// HIGH = enable, tx int enabled at write
	//*((volatile uint32_t *)UART0_IMSC) = 0x7D2;
	*((volatile uint32_t *)UART0_IMSC) = (7 << 4);
	SYNC;

	// Enable uart0 interrupt
	if (rpi2_uart0_excmode == RPI2_UART0_FIQ)
	{
		*((volatile uint32_t *)IRC_FIQCTRL) = ((1 << 7) | 57);
	}
	else
	{
		*((volatile uint32_t *)IRC_EN2) = (1 << 25);
	}
	SYNC;

#if 0
	for (tmp=0; tmp < 5; tmp++)
	{
		rpi2_led_blink(100, 100, 3);
		rpi2_delay_loop(500);
	}
	rpi2_delay_loop(2000);
#endif

	restore_ints(cpsr_store);

#if 0
	for (tmp=0; tmp < 6; tmp++)
	{
		rpi2_led_blink(100, 100, 3);
		rpi2_delay_loop(500);
	}
	rpi2_delay_loop(2000);
#endif

}

void serial_start()
{
	// Serial doesn't need
	return;
}

void enable_uart0_ints()
{
	*((volatile uint32_t *)IRC_EN2) = (1 << 25);
	SYNC;
}

void disable_uart0_ints()
{
	*((volatile uint32_t *)IRC_DIS2) = (1 << 25);
	SYNC;
}
// waits until transmit fifo is empty and writes the string
// directly in the tx fifo and returns the number of chars
// actually sent
// MAX 16 chars at a time
int serial_raw_puts(char *str)
{
	int i;

	SYNC;
	// wait until tx fifo becomes empty
	while (!(*((volatile uint32_t *)UART0_FR) & (1 << 7))) ;

	i=0;
	while (i < 16)
	{
		if (*str == '\0')
		{
			break;
		}

		// Write char in transmitter
		*((volatile uint32_t *)UART0_DR) = (uint32_t)(*(str++));
		i++;

	}
	return i;
}

void serial_start_tx()
{
	uint32_t cpsr_store;

	// no messing with tx interrupt
	cpsr_store = disable_save_ints();
	// re-enable tx interrupts
	*((volatile uint32_t *)UART0_IMSC) |= (1<<5);

	(void) serial_tx();

	// Clear transmit interrupt - probably not needed
	//*((volatile uint32_t *)UART0_ICR) = (1<<5);

	restore_ints(cpsr_store);
}

int serial_tx_free()
{
	if (ser_tx_tail > ser_tx_head)
	{
		return (ser_tx_head + SER_TX_BUFF_SIZE - ser_tx_tail);
	}
	return (ser_tx_head - ser_tx_tail);
}

int serial_rx_used()
{
	if (ser_rx_head > ser_rx_tail)
	{
		return (ser_rx_tail + SER_RX_BUFF_SIZE - ser_rx_head);
	}
	return (ser_rx_tail - ser_rx_head);
}

int serial_get_char()
{
	char ch;

	SYNC;

	if (ser_rx_tail == ser_rx_head)
	{
		serial_poll();
	}
	SYNC;

	// if rx buffer is empty
	if (ser_rx_tail == ser_rx_head)
	{
		// no chars
		return -1;
	}
	else
	{
		// get character from ring buffer
		ch = ser_rx_buff[ser_rx_head++];
		ser_rx_head %= SER_RX_BUFF_SIZE;
	}
	SYNC;
	return (int) ch;
}

int serial_write_char(char c)
{
	if (ser_tx_tail + 1 == ser_tx_head)
	{
		serial_poll();
	}
	SYNC;

	// if tx buffer is full
	if (ser_tx_tail + 1 == ser_tx_head)
	{
		// don't write, return error
		return -1;
	}
	else
	{
		// put character to ring buffer
		ser_tx_buff[ser_tx_tail++] = c;
		ser_tx_tail %= SER_TX_BUFF_SIZE;
	}
	SYNC;
	return 0; // success
}

int serial_put_char(char c)
{
	if(!serial_write_char(c))
	{
		serial_start_tx();
		return 0;
	}
	return -1; // success
}

// n is max string length
int serial_get_string(char *st, char delim, int n)
{
	int m = n; // count

	// Read one by one to let more characters to accumulate
	// This also takes care of negative n
	while (m > 0)
	{
		SYNC;

		if (ser_rx_tail == ser_rx_head)
		{
			serial_poll();
		}
		SYNC;

		// if rx buffer is empty
		if (ser_rx_tail == ser_rx_head)
		{
#if 1
			// quit reading
			break;
#else
			// wait
			continue;
#endif
		}
		else
		{
			// get character from ring buffer
			*st = ser_rx_buff[ser_rx_head++];
			ser_rx_head %= SER_RX_BUFF_SIZE;
			// a character less to read
			m--;
			if (*(st++) == delim) break;
			serial_poll();
		}
	}
	SYNC;
	return n - m; // characters actually got
}

// n is max string length
int serial_put_string(char *st, int n)
{
	int m = 0;
	int retry = 0;
	while (*st != '\0')
	{
		// check against max length
		if (m <= n)
		{
			// -1 means that tx buffer is full
			if (serial_write_char(*st) == -1)
			{
				retry++;
				if (retry > 100) break;
				continue;
				// break;
			}
			else
			{
				retry = 0;
				st++;
				m++;
			}
		}
		else break;
	}

	if (m > 0) serial_start_tx();

	return m;
}

// Read from the ring buffer
int serial_read(char *buf, int n)
{
	int m = n; // count

	// Read one by one to let more characters to accumulate
	// This also takes care of negative n
	while (m > 0)
	{
		SYNC;

		if (ser_rx_tail == ser_rx_head)
		{
			serial_poll();
		}
		SYNC;

		// if rx buffer is empty
		if (ser_rx_tail == ser_rx_head)
		{
			// quit reading
			break;
		}
		else
		{
			// get character from ring buffer
			*(buf++) = ser_rx_buff[ser_rx_head++];
			ser_rx_head %= SER_RX_BUFF_SIZE;
			// a character less to read
			m--;
		}
	}
	SYNC;
	return n - m; // characters actually got
}

// Write to the ring buffer
int serial_write(char *buf, int n)
{
	int m = n; // count
	if (n == 0) return 0;
	// write one by one to let more space to emerge
	while (m > 0)
	{

		if (ser_tx_tail + 1 == ser_tx_head)
		{
			serial_poll();
		}
		SYNC;

		// if tx buffer is full
		if (ser_tx_tail + 1 == ser_tx_head)
		{
			// quit writing
			break;
		}
		else
		{
			// put character to ring buffer
			ser_tx_buff[ser_tx_tail++] = *(buf++);
			ser_tx_tail %= SER_TX_BUFF_SIZE;
			// a character less to write
			m--;
		}
	}
	serial_start_tx();
	return n - m; // characters actually written
}

/* IRQ routines */

// a "pseudo-irq"
void serial_poll()
{
	uint32_t status;
	uint32_t curr_cpsr;
	uint32_t flagbit;

	if (rpi2_uart0_excmode == RPI2_UART0_FIQ) flagbit = 7;
	else flagbit = 8;

	// only do this if UART0 interrupts are disabled
	asm volatile ("mrs %[reg], cpsr\n\t" :[reg] "=r" (curr_cpsr)::);
	if (curr_cpsr & (1 << flagbit))
	{
		status = *((volatile uint32_t *)UART0_MIS);
		while (status & (7 << 4)) // UART0 interrupts
		{
			serial_irq();
			status = *((volatile uint32_t *)UART0_MIS);
		}
		while (ser_tx_tail != ser_tx_head)
		{
			serial_irq();
		}
	}
}

void serial_irq()
{
	uint32_t status;


	status = *((volatile uint32_t *)UART0_MIS);
#if 0
	while (!(status & (7 << 4)))
	{
		status = *((volatile uint32_t *)UART0_MIS);
	}
#endif
	// clear all UART0 interrupts except tx, rx and rx timeout
	// this should happen early enough due to the pipeline and
	// write buffer
	*((volatile uint32_t *)UART0_ICR) = 0x782; //all other
	//*((volatile uint32_t *)UART0_ICR) = 0x7f2; // all official
	//*((volatile uint32_t *)UART0_ICR) = 0x7ff; // all
	SYNC;

	if (status & (1<<4))
	{
		/* Rx interrupt */
		serial_rx();
		// Clear receive interrupt
		//*((volatile uint32_t *)UART0_ICR) = (1<<4);
	}
	if (status & (1<<5))
	{
		/* Tx interrupt */
		serial_tx();
		// Clear transmit interrupt
		//*((volatile uint32_t *)UART0_ICR) = (1<<5);
	}
	if (status & (1<<6))
	{
		/* Rx timeout interrupt */
		serial_rx();
		// Clear rx timeout interrupt
		//*((volatile uint32_t *)UART0_ICR) = (1<<6);
	}
#if 0
	serial_rx();
#endif
}

// Note: rx only reads rx_head, and only rx writes rx_tail
// Even simultaneous read and write shouldn't cause problems
void serial_rx()
{
	uint32_t ch;
	uint32_t uart0_fr;
	char scratchpad[16];

	SYNC;
	// if buffer is full
	if (ser_rx_tail + 1 == ser_rx_head)
	{
		// if receive fifo is full
		uart0_fr = *((volatile uint32_t *)UART0_FR);
		asm volatile("dsb\n\t");
		if (uart0_fr & (1 << 6))
		{
			// if we don't clear the interrupt, we'll be here all the time
			// and no one has a chance to read anything
			// if the first option doesn't work, the second option should
			// just more characters lost
#if 1
			// drop a char and clear the interrupt - if next
			// received character re-assignes the interrupt
			// At least a char-time for someone to read
			ch = *((volatile uint32_t *)UART0_DR);
			asm volatile("dsb\n\t");
			ser_rx_dropped_count++;
			// Clear receive interrupt
			*((volatile uint32_t *)UART0_ICR) = (1<<4);
			asm volatile("dsb\n\t");
			if (ch & 0x800) ser_rx_ovr_count++;
			/* if BRK character (CTRL-C) */
			/* It can't be handled if it doesn't fit into HW FIFO */
			if ((ch & 0xff) == 3)
			{
				/* if CTRL-C handler is in use */
				if (ser_handle_ctrlc)
				{
					ser_ctrl_c = 1;
					SYNC;
					rpi2_pend_trap();
				}
			}

#else
			// while receive fifo generates interrupt
			// drop characters to clear the interrupt
			while (*((volatile uint32_t *)UART0_MIS) & (1<<4))
			{
				// Read char in ch
				ch = *((volatile uint32_t *)UART0_DR);
				asm volatile("dsb\n\t");
				ser_rx_dropped_count++;
				if (ch & 0x800) ser_rx_ovr_count++;
				/* if BRK character (CTRL-C) */
				/* It can't be handled if it doesn't fit into HW FIFO */
				if ((ch & 0xff) == 3)
				{
					/* if CTRL-C handler is in use */
					if (ser_handle_ctrlc)
					{
						ser_ctrl_c = 1;
						SYNC;
						rpi2_pend_trap();
						continue;
					}
				}
			}
#endif
		}
	}
	// While buffer is not full
	while (ser_rx_tail + 1 != ser_rx_head)
	{
		// If receive FIFO is empty
		uart0_fr = *((volatile uint32_t *)UART0_FR);
		asm volatile("dsb\n\t");
		if (uart0_fr & (1 << 4))
		{
			// Quit reading
			break;
		}
		else
		{
			// Read char in ch
			ch = *((volatile uint32_t *)UART0_DR);
			asm volatile("dsb\n\t");
			if (ch & 0x800) ser_rx_ovr_count++;
			/* if BRK character (CTRL-C) */
			/* It can't be handled if it doesn't fit into HW FIFO */
			if ((ch & 0xff) == 3)
			{
				/* if CTRL-C handler is in use */
				if (ser_handle_ctrlc)
				{
					ser_ctrl_c = 1;
					SYNC;
					rpi2_pend_trap();
					continue;
				}
			}
			// Store in ring buffer
			ser_rx_buff[ser_rx_tail++] = (char)(ch & 0xff);
			ser_rx_tail %= SER_RX_BUFF_SIZE;
		}
	}
	SYNC;
}

// Note: tx only reads tx_tail, and only tx writes tx_head
// Even simultaneous read and write shouldn't cause problems
// (Well, serial_start_tx() does too, if interrupts are not happening)
// returns number of chars written, or -1 if buffer was empty on call
// to make it usable also for polling
void serial_tx()
{
	uint32_t ch;
	uint32_t uart0_fr;
	SYNC;
	// While buffer is not empty
	while (ser_tx_tail != ser_tx_head)
	{
		uart0_fr = *((volatile uint32_t *)UART0_FR);
		asm volatile("dsb\n\t");
		// If transmit FIFO is full
		if (uart0_fr & (1 << 5))
		{
			// Quit transmitting
			break;
		}
		else
		{
			// get char from ring buffer
			ch = (uint32_t)ser_tx_buff[ser_tx_head++];
			ser_tx_head %= SER_TX_BUFF_SIZE;
			// Write ch in transmitter
			*((volatile uint32_t *)UART0_DR) = ch;
			asm volatile("dsb\n\t");

		}
	}
	// if buffer is empty
	if (ser_tx_tail == ser_tx_head)
	{
		// to keep continuous tx interrupts from happening
		// when there is no data to send,
		// disable tx interrupts - re-enabled at write
		*((volatile uint32_t *)UART0_IMSC) &= ~(1<<5);
		// Clear transmit interrupt
		//*((volatile uint32_t *)UART0_ICR) = (1<<5);
	}
	SYNC;
}
