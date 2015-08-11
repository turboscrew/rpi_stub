/*
 * start.c
 *
 *  Created on: Mar 29, 2015
 *      Author: jaa
 */
#include <stdint.h>

// In case relocation needs to be done here
// #define DO_RELOC_HERE
#ifdef DO_RELOC_HERE
extern char __code_begin;
extern char __text_end;
extern char __bss_end;
extern char __new_org;
#endif

int main(uint32_t, uint32_t, uint32_t);

void start1()
{
#ifdef DO_RELOC_HERE
	char *s, *d;
	s = &__code_begin;
	d = &__new_org;
	while (s <= &__bss_end)
	{
		*(d++) = *(s++);
	}
#endif
	// here we could set up 'boot parameters'

	(void) main(0, 0, 0);
}

