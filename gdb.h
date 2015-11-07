/*
gdb.h

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

#ifndef GDB_H_
#define GDB_H_

#include "io_dev.h"

//#define DEBUG_GDB
//#define GDB_INVALIDATE_CACHES
//#define GDB_DEBUG_LED
//#define GDB_DEBUG_RX
//#define GDB_DEBUG_TX
//#define GDB_DEBUG_RX_LED

// program
typedef struct {
	void *start_addr;
	uint32_t size; // load size
	void *entry;
	void *curr_addr;
	uint8_t status;
} gdb_program_rec;

void gdb_init(io_device *device);
void gdb_trap();
void gdb_reset();
void gdb_trap_handler();
void gdb_send_text_packet(char *msg, unsigned int msglen);

#endif /* GDB_H_ */
