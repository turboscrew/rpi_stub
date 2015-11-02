/*
util.h

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

#ifndef UTIL_H_
#define UTIL_H_

/*
 * 'unsigned int' and 'unsigned char' are used here instead of
 * uint32_t and uint8_t to avoid multiple inclusions of the same file
 */

// hex digit to 4-bit value (nibble)
int util_hex_to_nib(char ch);

// 4-bit value (nibble) to hex di
int util_nib_to_hex(int nibble);

// convert upto 2 hex digits to unsigned char
unsigned char util_hex_to_byte(char *p);

// convert upto 8 hex digits to unsigned int
unsigned int util_hex_to_word(char *p);

// convert unsigned char to 2 hex digits
void util_byte_to_hex(char *dst, unsigned char b);

// convert unsigned int to 8 hex digits
void util_word_to_hex(char *dst, unsigned int w);

// convert"escaped" string used in gdb serial protocol to unsigned char
int util_bin_to_byte(unsigned char *src, unsigned char *dst);

// convert unsigned char to "escaped" string used in gdb serial protocol
int util_byte_to_bin(unsigned char *dst, unsigned char b);

// returns the length of the string excluding the end NUL
int util_str_len(char *p);

// compare strings - 0 = equal
int util_str_cmp(char *str1, char *str2);

// compare strings - returns the number of same characters
int util_cmp_substr(char *str1, char *str2);

// copy string, max_count = maximum number of characters to copy
// returns number of chars copied
int util_str_copy(char *dest, char *src, int max_count);

// append string to another - max = maximum result size in chars
// returns the total length of result string
int util_append_str(char *dst, char *src, int max);

// copy from src until delimiter (delimiter not included)
// max = maximum number of chars to copy
// returns number of strings copied
int util_cpy_substr(char *dst, char *src, char delim, int max);

// skip leading zeros from a number string
// returns pointer to new location and the length of the
// resulted string
int util_strip_zeros(char *src, char **dst);

// reads a signed decimal number from a string
// returns a signed integer and the number of characters read
int util_read_dec(char *str, int *result);

// converts a word endianness (swaps bytes)
void util_swap_bytes(unsigned int *src, unsigned int *dst);

// number of bits needed to represent a value
int util_num_bits(unsigned int val);

#endif /* UTIL_H_ */
