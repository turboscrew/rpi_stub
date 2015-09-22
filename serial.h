/*
 * serial.h
 *
 *  Created on: Mar 28, 2015
 *      Author: jaa
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include "io_dev.h"

void serial_init(io_device *device);
void serial_start();
void serial_set_ctrlc(void *handler);
int serial_get_ctrl_c();
int serial_get_char();
int serial_put_char(char c);
int serial_get_string(char *st, int n);
int serial_put_string(char *st, int n);
int serial_read(char *buf, int n);
int serial_write(char *buf, int n);

// serial interrupt handler
void enable_uart0_ints();
void disable_uart0_ints();
unsigned int serial_get_rx_dropped();

// waits until transmit fifo is empty and writes the string
// directly in the tx fifo and returns the number of chars
// actually sent
// MAX 16 chars at a time
int serial_raw_puts(char *str);

#endif /* SERIAL_H_ */
