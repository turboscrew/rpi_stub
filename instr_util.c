/*
 * instr_util.c
 *
 *  Created on: May 27, 2015
 *      Author: jaa
 */

#include "instr_util.h"
#include <stdint.h>

// gets high half word as signed int
int instr_util_shgethi(unsigned int val)
{
	return (int)(short int)((val >> 16) & 0xffff);
}

// gets low half word as signed int
int instr_util_shgetlo(unsigned int val)
{
	return (int)(short int)(val & 0xffff);
}

// sign-extends the lower short in unsigned int to signed integer
int instr_util_signx_short(unsigned int val)
{
	if (val & 0x00008000) // if negative
		val |= 0xffff0000; // set sign extend bits
	else
		val &= 0x0000ffff; // clear sign extend bits
	return (int)val;
}

// sign-extends the lowest byte in unsigned int to signed integer
int instr_util_signx_byte(unsigned int val)
{
	if (val & 0x00000080) // if negative
		val |= 0xffffff00; // set sign extend bits
	else
		val &= 0x000000ff; // clear sign extend bits
	return (int)val;
}

// stuffs two signed integers into high and low half words of an unsigned int
unsigned int instr_util_ustuffs16(int val1, int val2)
{
	unsigned int retval;
	retval = (unsigned int)((val1 << 16) & 0xffff0000);
	retval |= (unsigned int)(val2 & 0xffff);
	return retval;
}

// stuffs two unsigned integers into high and low half words of an unsigned int
unsigned int instr_util_ustuffu16(unsigned int val1, unsigned int val2)
{
	unsigned int retval;
	retval = ((val1 << 16) & 0xffff0000);
	retval |= (val2 & 0xffff);
	return retval;
}

// stuffs four unsigned bytes into an unsigned int
unsigned int instr_util_ustuffu8(unsigned int byte3, unsigned int byte2, unsigned int byte1, unsigned int byte0)
{
	unsigned int retval;
	retval = ((byte3 << 24) & 0xff000000);
	retval |= ((byte2 << 16) & 0x00ff0000);
	retval |= ((byte1 << 8) & 0x0000ff00);
	retval |= (byte0 & 0x000000ff);
	return retval;
}

// stuffs four signed bytes into an unsigned int
unsigned int instr_util_ustuffs8(int byte3, int byte2, int byte1, int byte0)
{
	unsigned int retval;
	retval = (unsigned int)((byte3 << 24) & 0xff000000);
	retval = (unsigned int)((byte2 << 16) & 0x00ff0000);
	retval = (unsigned int)((byte1 << 8)  & 0x0000ff00);
	retval |= (unsigned int)(byte0 & 0x000000ff);
	return retval;
}

// saturate unsigned integer
unsigned int instr_util_usat(int val, unsigned int bitc)
{
	int retval;
	int max = (1 << bitc) - 1;

	if (val > max) retval = max;
	else if (val < 0) retval = 0;
	else retval = val;
	return (int)retval;
}

// saturate signed long long integer
long long int instr_util_lssat(long long int val, unsigned int bitc)
{
	long long int retval;
	long long int max = 1L;
	long long int min = 1L;
	max = (max << (bitc -1)) -1;
	min = -(min << (bitc - 1));
	if (val > max) retval = max;
	else if (val < min) retval = min;
	else retval = val;
	return retval;
}

// saturate signed integer
int instr_util_ssat(int val, unsigned int bitc)
{
	return (int)instr_util_lssat((long long int)val, bitc);
}

// right-rotate bytes of an unsigned integer
unsigned int instr_util_rorb(unsigned int val, int count)
{
	int i, retval;
	retval = val;
	for (i=0; i<count; i++)
	{
		val = retval;
		retval = ((val & 0xff) << 24) | ((val >> 8) & 0x00ffffff);
	}
	return retval;
}

// swap half words of an unsigned integer
unsigned int instr_util_swaph(unsigned int val)
{
	return ((val << 16) & 0xffff0000) | ((val >> 16) & 0xffff);
}


