/*
 * start.c
 *
 *  Created on: Mar 29, 2015
 *      Author: jaa
 */
#include <stdint.h>
#include "rpi2.h"


extern int main(uint32_t, uint32_t, uint32_t);

// In case relocation needs to be done here
// #define DO_RELOC_HERE
#ifdef DO_RELOC_HERE
extern char __code_begin;
extern char __text_end;
extern char __bss_end;
extern char __new_org;
#endif

void start1_fun()
{
	uint32_t i;
#ifdef DO_RELOC_HERE
	char *s, *d;
	s = &__code_begin;
	d = &__new_org;
	while (s <= &__bss_end)
	{
		*(d++) = *(s++);
	}
#endif
	// rpi2_init_led(); already done in start.S
	while(1)
	{
		rpi2_led_blink(3000, 500, 3);
		rpi2_delay_loop(5000);
	}
	// here we could set up 'boot parameters'
	(void) main(0, 0, 0);
}
