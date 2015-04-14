/*
 * loader.c
 *
 *  Created on: Mar 27, 2015
 *      Author: jaa
 */

#include <stdint.h>
#include "serial.h"
#include "gdb.h"
#include "io_dev.h"

void main(uint32_t r0, uint32_t r1, uint32_t r2)
{
	/* kernel parameters - for future use? */
	(void) r0;
	(void) r1;
	(void) r2;

	io_device serial_io;

	/* initialize serial for debugger */
	serial_init(&serial_io);

	/* initialize debugger */
	gdb_init(&serial_io);

	while (1)
	{
		/* reset debugger */
		gdb_reset();
		/* enter gdb monitor */
		gdb_trap();		
	}

}
