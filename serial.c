/*
 * serial.c
 *
 *  Created on: Mar 28, 2015
 *      Author: jaa
 */
#include <stdint.h>
#include "rpi2.h"
#include "serial.h"

/* UART definitions */

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

// I/O buffers, we write to tail and read from head
// buffers are post-incrementing
#define SER_RX_BUFF_SIZE 1024
#define SER_TX_BUFF_SIZE 1024

volatile int ser_rx_head = 0;
volatile int ser_rx_tail = 0;
volatile char ser_rx_buff[SER_RX_BUFF_SIZE];

volatile int ser_tx_head = 0;
volatile int ser_tx_tail = 0;
volatile char ser_tx_buff[SER_TX_BUFF_SIZE];

volatile int ser_ctrlc;
void (*ser_ctrlc_handler)();
volatile int ser_ctrl_c = 0;

void serial_rx();
void serial_tx();

/* delay() borrowed from OSDev.org */
static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : : [count]"r"(count) : "cc");
}

int serial_get_ctrl_c()
{
	int retval;
	retval = ser_ctrl_c;
	ser_ctrl_c = 0;
	return retval;
}

void serial_set_ctrlc(void *handler)
{
	if (handler)
	{
		/* enable ctrl-C detection */
		ser_ctrlc_handler = (void (*)()) handler;
		ser_ctrlc = 1;
	}
	else
	{
		/* disable ctrl-C detection */
		ser_ctrlc = 0;
		ser_ctrlc_handler = 0;
	}
}

void serial_init(io_device *device)
{
	uint32_t tmp; // scratchpad

	// Disable UART0 for configuration - sorry, took the only "real" UART,
	// but with that, uploading is faster and so is debugging
	*((volatile uint32_t *)UART0_CR) = 0x00000000;

	// Fill up the device structure
	device->get_char = serial_get_char;
	device->get_string = serial_get_string;
	device->put_char = serial_put_char;
	device->put_string = serial_put_string;
	device->read = serial_read;
	device->write = serial_write;
	device->start = serial_start;
	device->set_ctrlc = serial_set_ctrlc;
	device->get_ctrl_c = serial_get_ctrl_c;

	// Clear ring buffers
	ser_rx_head = 0;
	ser_rx_tail = 0;
	ser_tx_head = 0;
	ser_tx_tail = 0;

	// Setup the GPIO pin 14 & 15.

	// Change pull up/down to pull-down & delay for 150 cycles
	*((volatile uint32_t *)GPPUD) = 0x00000000; // pull-down disabled for pins 14 & 15
	delay(150);

	// Target pull up/down change to pin 14,15 & delay for 150 cycles.
	*((volatile uint32_t *)GPPUDCLK0) = ((1 << 14) | (1 << 15));
	delay(150);

	// Change pull up/down to disabled & delay for 150 cycles
	*((volatile uint32_t *)GPPUD) = 0x00000000; // pull-down for pins 14 & 15

	// Write 0 to GPPUDCLK0 to 'un-target' the pull up/down change
	*((volatile uint32_t *)GPPUDCLK0) = 0x00000000;

	// GPIO 14,15 to UART0 rx & tx
	tmp = *((volatile uint32_t *)GPFSEL1);
	tmp &= ~(7<<(4 * 3) | 7<<(5 * 3)); // clear the functions for pins 14 and 15
	tmp |= 0x4 << (4 * 3); // pin 14 - the 4th 3-bit group, 4 = alt function 0			tmp = 0x4 << (4 * 3) // pin 14 - the 4th 3-bit group, 4 = alt function 0
	tmp |= 0x4 << (5 * 3); // pin 15 - the 5th 3-bit group
	*((volatile uint32_t *)GPFSEL1) = tmp;


	// Configure UART0

	// Mask off all UART0 interrupts for now
	*((volatile uint32_t *)UART0_IMSC) = 0x7F2;

	// Set integer & fractional part of baud rate
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200

	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1
	// Fractional part register = (0.627 * 64) + 0.5 = 40.6 = ~40
	*((volatile uint32_t *)UART0_IBRD) = 1;
	*((volatile uint32_t *)UART0_FBRD) = 40;

	// Flush FIFOs by disabling them
	*((volatile uint32_t *)UART0_LCRH) = 0;

	// Enable FIFO & 8 bit data transmission (1 stop bit, no parity)
	*((volatile uint32_t *)UART0_LCRH) = (1 << 4) | (1 << 5) | (1 << 6);

	// FIFO-interrupt levels: rx 1/2, tx 1/2
	*((volatile uint32_t *)UART0_IFLS) = (0x2 << 3) | (0x2) ;

	// Enable UART0, receive & transfer part of UART
	*((volatile uint32_t *)UART0_CR) = (1 << 0) | (1 << 8) | (1 << 9);

	// Clear interrupts
	*((volatile uint32_t *)UART0_ICR) = 0x7ff;

	// Set UART0 interrupt mask (enable almost all)
	*((volatile uint32_t *)UART0_IMSC) = 0x7F0;

	// Enable uart interrupts
	*((volatile uint32_t *)IRC_EN2) = (1 << 25);

}

void serial_start()
{
	// Serial doesn't need
	return;
}

void serial_start_tx()
{
	// If TX FIFO is empty and transmitter is idle
	// (If TX FIFO is not empty, transmitter will become busy)
	if ( (*((volatile uint32_t *)UART0_FR) & (1 << 7))
			&& (*((volatile uint32_t *)UART0_FR) & (1 << 3)) )
	{
		// no messing with tx interrupt
		asm volatile ("cpsid i\n\t");
		serial_tx();
		asm volatile ("cpsie i\n\t");
	}
}

int serial_get_char()
{
	char ch;

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
	return (int) ch;
}

int serial_put_char(char c)
{
	// if tx buffer is full
	if (ser_tx_tail + 1 == ser_tx_head)
	{
		// make sure transmitter is awake
		serial_start_tx();
		// don't write, return error
		return -1;
	}
	else
	{
		// put character to ring buffer
		ser_tx_buff[ser_tx_tail++] = c;
		ser_tx_tail %= SER_TX_BUFF_SIZE;
	}
	serial_start_tx();

	return 0; // success
}

// n is max string length
int serial_get_string(char *st, int n)
{
	return -1; // not implemented
}

// n is max string length
int serial_put_string(char *st, int n)
{
	int m = 0;

	while (*st != '\0')
	{
		// check against max length
		if (m <= n)
		{
			// -1 means that FIFO is full
			if (serial_put_char(*(st++)) == -1)
			{
				break;
			}
			else
			{
				m++;
			}
		}
		else break;
	}
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
	return n - m; // characters actually got
}
// Write to the ring buffer
int serial_write(char *buf, int n)
{
	int m = n; // count

	// If data to be sent
	// (This also takes care of negative n)
	if (m > 0)
	{
		if ( serial_put_char(*(buf++)) )
		{
			return 0; // No chars written
		}
		m--;
	}
	// read one by one to let more space to emerge
	while (m > 0)
	{
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
	return n - m; // characters actually written

}

/* IRQ routines */

void serial_irq()
{
	if (*((volatile uint32_t *)UART0_MIS) & (1<<4))
	{
		/* Rx interrupt */
		serial_rx();
		// Clear receive interrupt
		*((volatile uint32_t *)UART0_ICR) = (1<<4);
	}
	if (*((volatile uint32_t *)UART0_MIS) & (1<<5))
	{
		/* Tx interrupt */
		serial_tx();
		// Clear transmit interrupt
		*((volatile uint32_t *)UART0_ICR) = (1<<5);
	}
	if (*((volatile uint32_t *)UART0_MIS) & (1<<6))
	{
		/* Rx timeout interrupt */
		serial_rx();
		// Clear rx timeout interrupt
		*((volatile uint32_t *)UART0_ICR) = (1<<6);
	}
	/* Clear all UART0 interrupts except tx, rx and rx timeout */
	*((volatile uint32_t *)UART0_ICR) = 0x782;
}

// Note: rx only reads rx_head, and only rx writes rx_tail
// Even simultaneous read and write shouldn't cause problems
void serial_rx()
{
	uint32_t ch;
	// While buffer is not full
	while (ser_rx_tail + 1 != ser_rx_head)
	{
		// If receive FIFO is empty
		if (*((volatile uint32_t *)UART0_FR) & (1 << 4))
		{
			// Quit reading
			break;
		}
		else
		{
			// Read char in ch
			ch = *((volatile uint32_t *)UART0_DR);
			// Store in ring buffer
			ser_rx_buff[ser_rx_tail++] = (char)(ch & 0xff);
			ser_rx_tail %= SER_RX_BUFF_SIZE;
		}
		/* if BRK character (CTRL-C) */
		/* It can't be handled if it doesn't fit into HW FIFO */
		if ((ch & 0xff) == 3)
		{
			/* if CTRL-C handler is in use */
			if (ser_ctrlc)
			{
				ser_ctrl_c = 1;
				ser_ctrlc_handler();
			}
		}
	}
}

// Note: tx only reads tx_tail, and only tx writes tx_head
// Even simultaneous read and write shouldn't cause problems
// (Well, serial_start_tx() does too, if interrupts are not happening)
void serial_tx()
{
	uint32_t ch;
	// While buffer is not empty
	while (ser_tx_tail != ser_tx_head)
	{
		// If transmit FIFO is full
		if (*((volatile uint32_t *)UART0_FR) & (1 << 5))
		{
			// Quit transmitting
			break;
		}
		else
		{
			// Read char from ring buffer
			ch = (uint32_t)ser_tx_buff[ser_tx_head++];
			ser_tx_head %= SER_TX_BUFF_SIZE;
			// Write ch in transmitter
			*((volatile uint32_t *)UART0_DR) = ch;
		}
	}
}
