/*
loader.c

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
#include "gdb.h"
#include "io_dev.h"
#include "util.h"

// put the SW into 'echo-mode' instead of starting gdb-stub
//#define SERIAL_TEST

io_device serial_io;

extern volatile gdb_program_rec gdb_debuggee;

// split in 'main' and 'loader_main', because GCC wants to
// put 'main' into a special section - now 'loader_main' can
// be relocated into upper memory more easily 
void loader_main()
{
	int i;
	uint32_t tmp1, tmp2, tmp3, tmp4; // for debugging
	char *msg; // for debugging

	// for debugging
	int len;
	const int dbg_buff_len = 512;
	static char scratchpad[16]; // scratchpad
	static char dbg_buff[512]; // message buffer
	static char scratch2[16];
	/* initialize rpi2 */
	rpi2_init();

	/* initialize serial for debugger */
	serial_init(&serial_io);
	
	// debug-line
	msg = "Finally! Got into main()\r\n";
	i=0;
	//do {i = serial_raw_puts(msg); msg += i;} while (i) ;

	// rpi2_check_debug();
	
	rpi2_led_blink(1000, 1000, 3);
	rpi2_delay_loop(3000);

#if 0		
	//  dump vectors (debug)
	len = 0;
	len = util_str_copy(dbg_buff, "vectors: addr, value\r\n", dbg_buff_len);
	for (i=0; i< 16; i++)
	{
		tmp1 = (i * 4); // address
		tmp2 = *((volatile uint32_t *) (tmp1)); // vector

		util_word_to_hex(scratchpad, tmp1);
		len = util_append_str(dbg_buff, scratchpad, dbg_buff_len);
		len = util_append_str(dbg_buff, ", ", dbg_buff_len);
		util_word_to_hex(scratchpad, tmp2);
		len = util_append_str(dbg_buff, scratchpad, dbg_buff_len);
		len = util_append_str(dbg_buff, "\r\n", dbg_buff_len);
		
	}
	if (len > 0)
	{
		serial_io.put_string(dbg_buff, len+1);
	}
	else
	{
		serial_io.put_string("Too long line 1\r\n", 18);	
	}
#endif

#if 0
	// test SVC exception handling
	msg = "trying SVC\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
	// a little delay for serial output
	rpi2_led_blink(1000, 1000, 3);
	asm volatile ("svc #0\n\t");
	msg = "returned from SVC\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
	asm volatile (
			"mrs %[retreg], cpsr\n\t"
			:[retreg] "=r" (tmp1) ::
	);
	msg = "cpsr = ";
	serial_io.put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, tmp1);
	serial_io.put_string(scratchpad, 9);
	serial_io.put_string("\r\n", 3);
#endif
#if 0
	// test PABT exception handling
	msg = "\r\ntrying BKPT\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
	// a little delay for serial output
	rpi2_led_blink(1000, 1000, 3);
	asm volatile ("bkpt #0\n\t");
	msg = "\r\nreturned from BKPT\r\n";
	//i=0;
	//do {i = serial_raw_puts(msg); msg += i;} while (i);
	serial_io.put_string(msg, util_str_len(msg)+1);
#endif
#if 0
	// test SMC exception handling
	msg = "trying SMC\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
	// a little delay for serial output
	rpi2_led_blink(1000, 1000, 3);
	asm volatile ("smc #0\n\t");
	msg = "returned from SMC\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
#endif
#if 0
	// test HVC exception handling
	msg = "trying HVC\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
	// a little delay for serial output
	rpi2_led_blink(1000, 1000, 3);
	asm volatile ("hvc #0\n\t");
	msg = "returned from HVC\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);
#endif

#ifdef SERIAL_TEST	
	// serial test mode
	msg = "\r\nEntering main loop\r\n";
	serial_io.put_string(msg, util_str_len(msg)+1);

	// echo-loop for testing serial I/O
	while (1)
	{			
		// echo
#if 0
		// test serial_read()
		len = serial_io.read(dbg_buff, 512);
#else
		// test serial_get_char()
		for (tmp1=0; tmp1<512; tmp1++)
		{
			i = serial_io.get_char();
			if (i < 0) break;
			dbg_buff[tmp1] = (char)i;
		}
		len = (int) tmp1;
#endif
		if (len > 0)
		{
			i = 0;
			while (i < len)
			{
#if 0
				// test serial_write()
				tmp1 = serial_io.write(dbg_buff+i, len-i);
				i += tmp1;
				if (i < len) rpi2_led_on(); // buffer full
				else rpi2_led_off();
#else
				// test serial_put_string()
				if ((len-i) < 15) tmp1 = util_cpy_substr(scratchpad, dbg_buff+i, ' ', len+1-i);
				else tmp1 = util_cpy_substr(scratchpad, dbg_buff+i, ' ', 16);
				i += tmp1;				
				if (dbg_buff[i] == ' ')
				{
					scratchpad[tmp1++] = ' ';
					scratchpad[tmp1] = '\0';
					i++; // skip delimiter
				}
				tmp2 = 0;
				while (tmp2 < tmp1)
				{
					tmp3 = serial_io.put_string(scratchpad+tmp2, 16-tmp2);
					tmp2 += tmp3;
					if (tmp3 < tmp1) rpi2_led_on(); // buffer full
					else rpi2_led_off();
				}
#endif
			}
		}
		
		i = (int) serial_get_rx_dropped();
		if (i > 0)
		{
			util_word_to_hex(scratchpad, i);
			scratchpad[8]='\0'; // end-nul
			serial_raw_puts("\r\ndropped: ");
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\n");
		}
	}
#else
	/* initialize debugger */
	gdb_init(&serial_io);

	while (1)
	{
		/* reset debugger */
		gdb_reset();
		/* enter gdb monitor */
		gdb_trap();
		
	}
#endif
}

void main(uint32_t r0, uint32_t r1, uint32_t r2)
{
	/* kernel parameters - for future use? */
	(void) r0;
	(void) r1;
	(void) r2;
	
/*
	int i;
	for(i=0; i<5; i++)
	{
		rpi2_led_blink(300, 300, 3);
		rpi2_delay_loop(1000);
	}
*/	
	loader_main();
}