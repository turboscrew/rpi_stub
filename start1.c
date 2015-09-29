/*
start1.c

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


extern int main(uint32_t, uint32_t, uint32_t);

// In case relocation needs to be done here
// #define DO_RELOC_HERE
#ifdef DO_RELOC_HERE
extern char __load_start;
extern char __load_end;
extern char __code_begin;
#endif

void start1_fun()
{
	uint32_t i;
#ifdef DO_RELOC_HERE
	char *s, *d;
	s = &__load_start;
	d = &__code_begin;
	while (s <= &__load_end)
	{
		*(d++) = *(s++);
	}
#endif
	// rpi2_init_led(); already done in start.S
	/*
	while(1)
	{
		//rpi2_led_blink(1000, 1000, 3);
		rpi2_delay_loop(5000);
	}
	*/
	//rpi2_led_blink(1000, 100, 5);

	// here we could set up 'boot parameters'
	(void) main(0, 0, 0);
}
