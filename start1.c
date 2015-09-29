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
