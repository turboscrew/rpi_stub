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
// #define SERIAL_TEST

io_device serial_io;
char cmdline[1024];

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
	const int dbg_buff_len = 1024;
	static char scratchpad[16]; // scratchpad
	static char dbg_buff[1024]; // message buffer
	static char scratch2[16];

#if 0
	rpi2_led_blink(2000, 300, 3);
	rpi2_delay_loop(2000);
#endif	

	/* initialize rpi2 */
	rpi2_init();
	
#if 0	
	rpi2_led_blink(2000, 300, 4);
	rpi2_delay_loop(2000);
#endif

	/* initialize serial for debugger */
	serial_init(&serial_io);

#if 0
	rpi2_led_blink(2000, 300, 5);
	rpi2_delay_loop(2000);
#endif

	/* enable all exceptions */
	rpi2_enable_excs();

#if 0
	rpi2_led_blink(2000, 300, 6);
	rpi2_delay_loop(2000);
#endif

#if 0
	rpi2_get_cmdline(dbg_buff);
	serial_io.put_string(dbg_buff, dbg_buff_len);
#endif

#if 0
	msg = "\r\nrpi2_use_mmu = ";
	serial_io.put_string(msg, util_str_len(msg));
	util_word_to_hex(scratchpad, rpi2_use_mmu);
	serial_io.put_string(scratchpad, 9);
	msg = " rpi2_uart0_excmode = ";
	serial_io.put_string(msg, util_str_len(msg));
	util_word_to_hex(scratchpad, rpi2_uart0_excmode);
	serial_io.put_string(scratchpad, 9);
	msg = "\r\nrpi2_arm_ramsize = ";
	serial_io.put_string(msg, util_str_len(msg));
	util_word_to_hex(scratchpad, rpi2_arm_ramsize);
	serial_io.put_string(scratchpad, 9);
	msg = " rpi2_arm_ramstart = ";
	serial_io.put_string(msg, util_str_len(msg));
	util_word_to_hex(scratchpad, rpi2_arm_ramstart);
	serial_io.put_string(scratchpad, 9);
	serial_io.put_string("\r\n", 3);
	//serial_io.put_string(cmdline, 1024);
	//serial_io.put_string("\r\n", 3);
#endif

#if 0
	// debug-line
	msg = "Got into main()\r\n";
	i=0;
	do {i = serial_raw_puts(msg); msg += i;} while (i) ;
	msg = "serial should work\r\n";
	serial_io.put_string(msg, util_str_len(msg));
#endif

#ifdef SERIAL_TEST
	serial_io.enable_ctrlc();
	// debug-line
	msg = "Serial test\r\n";
	i=0;
	do {i = serial_raw_puts(msg); msg += i;} while (i) ;

	// rpi2_check_debug();
	
	//rpi2_led_blink(1000, 1000, 3);
	//rpi2_delay_loop(2000);
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
	msg = "\r\ncpsr = ";
	serial_raw_puts(msg);
	//serial_io.put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, tmp1);
	serial_raw_puts(scratchpad);
	serial_raw_puts("\r\n");
	//serial_io.put_string(scratchpad, 9);
	//serial_io.put_string("\r\n", 3);
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

	while (1)
	{
		//msg = "setting up gdb\r\n";
		//serial_io.put_string(msg, util_str_len(msg));
#if 0		
		rpi2_check_debug();
#endif		
		/* initialize debugger */
		gdb_init(&serial_io);
		/* reset debugger */
		gdb_reset(0);
		/* enter gdb monitor */
		gdb_trap();	
	}
#endif
}

void main(uint32_t r0, uint32_t r1, uint32_t r2)
{
	/* device tree parameters - for future use? */
	(void) r0;
	(void) r1;
	(void) r2;
	
	int i;
		
#if 0
	int i;
	for(i=0; i<5; i++)
	{
		rpi2_led_blink(300, 300, 5);
		rpi2_delay_loop(1000);
	}
#endif
	rpi2_uart0_excmode = RPI2_UART0_POLL; // default
	rpi2_use_mmu = 0; // default - no mmu
	rpi2_get_cmdline(cmdline);
#if 1
	for (i=0; i< 1024; i++)
	{
		if (cmdline[i] == 'r')
		{
			if (util_cmp_substr("rpi_stub_", cmdline + i) >= util_str_len("rpi_stub_"))
			{
				i += util_str_len("rpi_stub_");
				if (util_cmp_substr("mmu", cmdline + i) >= util_str_len("mmu"))
				{
					i += util_str_len("mmu");
					// set flag for mmu setup
					// rpi_stub_mmu
					rpi2_use_mmu = 1;
				}
				else if (util_cmp_substr("interrupt=", cmdline + i) >= util_str_len("interrupt="))
				{
					i += util_str_len("interrupt=");
					if (util_cmp_substr("irq", cmdline + i) >= util_str_len("irq"))
					{
						i += util_str_len("irq");
						// set flag for UART0 using IRQ
						rpi2_uart0_excmode = RPI2_UART0_IRQ;
					}
					else if (util_cmp_substr("fiq", cmdline + i) >= util_str_len("fiq"))
					{
						i += util_str_len("fiq");
						// set flag for UART0 using FIQ
						rpi2_uart0_excmode = RPI2_UART0_FIQ;
					}
					else if (util_cmp_substr("poll", cmdline + i) >= util_str_len("poll"))
					{
						i += util_str_len("poll");
						// set flag for UART0 using poll
						// rpi_stub_interrupt=poll
						rpi2_uart0_excmode = RPI2_UART0_POLL;
					}
				}
#if 0
				else if (util_cmp_substr("baud=", cmdline + i) >= util_str_len("baud="))
				{
					i += util_str_len("baud=");
					// get baud for serial
				}
#endif
			}
		}
	}
#endif
	loader_main();
}