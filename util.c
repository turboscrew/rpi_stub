/*
util.c

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

#include "util.h"

// hex digit to 4-bit value (nibble)
int util_hex_to_nib(char ch)
{
	int val;
	switch (ch)
	{
	case 'a':
	case 'A':
		val = 10;
		break;
	case 'b':
	case 'B':
		val = 11;
		break;
	case 'c':
	case 'C':
		val = 12;
		break;
	case 'd':
	case 'D':
		val = 13;
		break;
	case 'e':
	case 'E':
		val = 14;
		break;
	case 'f':
	case 'F':
		val = 15;
		break;
	default:
		val = (int)ch - (int)'0';
	}
	if ((val < 0) || (val > 15))
	{
		val = -1; // illegal hex
	}
	return val;
}

// 4-bit value (nibble) to hex digit
int util_nib_to_hex(int nibble)
{
	if (nibble < 0) return -1; // illegal value
	if (nibble < 10) return nibble + 0x30; // 0 -> '0'
	if (nibble < 16) return nibble + 0x57; // 10 -> 'a'
	return -1; // illegal value
}

// convert upto 2 hex digits to unsigned char
unsigned char util_hex_to_byte(char *p)
{
	int val;
	unsigned char retval;
	val = util_hex_to_nib(*p);
	if (val < 0) return 0; // no digits
	retval = (unsigned char) val;
	val |= util_hex_to_nib(*(++p));
	if (val < 0) return retval; // 1 digit
	retval <<= 4;
	retval |= (val & 0x0f);
	return retval;
}

// convert upto 8 hex digits to unsigned int
unsigned int util_hex_to_word(char *p)
{
	int val;
	int i;
	unsigned int retval = 0;
	for (i=0; i<8; i++)
	{
		val = util_hex_to_nib(*p);
		if (val < 0) return retval;
		retval <<= 4;
		retval |= (val & 0x0f);
		p++;
	}
	return retval;
}

// byte to hex
void util_byte_to_hex(char *dst, unsigned char b)
{
	char ch;
	*(dst++) = util_nib_to_hex((int) ((b >> 4) & 0x0f));
	*(dst++) = util_nib_to_hex((int) (b & 0x0f));
	*dst = 0;
}

// word to hex
void util_word_to_hex(char *dst, unsigned int w)
{
	unsigned char byte;
	byte = (unsigned char)((w >> 24) & 0xff);
	util_byte_to_hex(dst, byte);
	dst +=2;
	byte = (unsigned char)((w >> 16) & 0xff);
	util_byte_to_hex(dst, byte);
	dst +=2;
	byte = (unsigned char)((w >> 8) & 0xff);
	util_byte_to_hex(dst, byte);
	dst +=2;
	byte = (unsigned char)(w & 0xff);
	util_byte_to_hex(dst, byte);
	dst +=2;
	*dst = 0;
}

// bin (escaped binary) to byte
unsigned char util_bin_to_byte(unsigned char *p)
{
	unsigned char retval;
	if (*p == 0x7d) // escape
	{
		retval = (*(++p)) ^ 0x20;
	}
	else
	{
		retval = *p;
	}
	return retval;
}

// byte to bin (escaped binary)
int util_byte_to_bin(unsigned char *dst, unsigned char b)
{
	int retval;
	switch (b)
	{
	case 0x7d: // escape
	case 0x23: // '#'
	case 0x24: // '$'
	case 0x2a: // '*'
		*(dst++) = 0x7d;
		*dst = b ^ 0x20;
		retval = 2;
		break;
	default:
		*dst = b;
		retval = 1;
		break;
	}
	return retval;
}

// string length
int util_str_len(char *p)
{
	int i = 0;
	while (*(p++) != '\0') i++;
	return i;
}

// compare strings - 0 = equal
int util_str_cmp(char *str1, char *str2)
{
	while (*str1== *(str2++))
	{
		if (*(str1++) == '\0') return 0;
	}
	return 1; // not equal
}

// compare strings - returns the number of same characters
int util_cmp_substr(char *str1, char *str2)
{
	int i = 0;
	while (*(str1) == *(str2))
	{
		i++;
		if (*(str1++) == '\0') return i;
		if (*(str2++) == '\0') return i;
	}
	return i; // not equal
}

// copy string, max_count = maximum number of characters to copy
// returns number of chars copied
int util_str_copy(char *dest, char *src, int max_count)
{
	int cnt = 0;

	if (max_count <= 0) return cnt;
	while (*src != '\0')
	{
		*(dest++) = *(src++);
		cnt++;

		if (cnt >= max_count) break;
	}
	*dest = '\0'; // add end-of-string
	return cnt;
}

// append str append string to another - max = maximum result size in chars
// returns the length of result string
int util_append_str(char *dst, char *src, int max)
{
	int i = 0;
	// find end of dst string
	while (*dst != '\0')
	{
		dst++;
		i++;
		if (i >= max-1) return i; // no end-of-string found
	}
	// copy until end of src string - overwrite end-of-string of dst
	while (*src != '\0')
	{
		*(dst++) = *(src++);
		i++;

		if (i >= (max -1)) break; // dest buffer too small
	}
	*dst = '\0'; // add end-of-string
	return i; // total length of dst excluding the '\0'
}

// copy from src until delimiter (delimiter not included)
// max = maximum number of chars to copy
// returns number of strings copied
int util_cpy_substr(char *dst, char *src, char delim, int max)
{
	int i = 0;
	while (*src != delim)
	{
		if (*src == '\0') break;
		*(dst++) = *(src++);
		i++;
		if (i >= (max -1)) break;
	}
	*dst = '\0';
	return i;
}

// skip leading zeros from a number string
// returns pointer to new location and the length of the
// resulted string
int util_strip_zeros(char *src, char **dst)
{
	int i = 0;

	if (*src == '\0')
	{
		*dst = src;
		return 0;
	}
	// skip leading zeroes, but in case of number being
	// zero, leave the last '0'
	while ((*(src+1) != '\0') && (*src == '0'))
	{
		src++;
	}
	*dst = src;
	// count the length of the result
	while (*(src++) != '\0')
	{
		i++;
	}
	return i;
}

// reads a signed decimal number from a string
// returns a signed integer and the number of characters read
int util_read_dec(char *str, int *result)
{
	int neg = 0; // flag - if the number is negative
	int tmp, i=0;
	long long tmp2;

	*result = 0;

	if (*str == '-')
	{
		neg = 1;
		str++;
		i++;
	}
	tmp2 = 0LL;
	while (*str != '\0')
	{
		tmp = (int)(*str) - (int)'0';
		if ((tmp < 0) || (tmp > 9))
		{
			break; // not a decimal digit
		}
		else
		{
			tmp2 *= 10;
			tmp2 += (long long)tmp;
			if (tmp2 > 0x7fffffff) break; // integer overflow
			str++;
			i++;
			*result = (int)tmp2;
		}
	}
	if (neg)
	{
		if (i == 1) return 0; // only sign - no number
		*result = -(*result);
	}
	return i;
}

// converts a word endianness (swaps bytes)
void util_swap_bytes(unsigned char *src, unsigned char *dst)
{
	*(dst++) = *(src+3);
	*(dst++) = *(src+2);
	*(dst++) = *(src+1);
	*dst = *src;
}
