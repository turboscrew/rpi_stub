/*
 * debug.h
 *
 *  Created on: Mar 28, 2015
 *      Author: jaa
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "io_dev.h"

// program
typedef struct {
	void *start;
	uint32_t size; // load size
	void *entry;
	void *curr_addr;
	uint8_t status;
} gdb_program_rec;

void gdb_init(io_device *device);
void gdb_trap();
void gdb_reset();
void gdb_trap_handler();

#endif /* DEBUG_H_ */
