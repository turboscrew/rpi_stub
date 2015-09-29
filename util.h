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
int util_hex_to_nib(char ch);
int util_nib_to_hex(int nibble);
unsigned char util_hex_to_byte(char *p);
unsigned int util_hex_to_word(char *p);
void util_byte_to_hex(char *dst, unsigned char b);
void util_word_to_hex(char *dst, unsigned int w);
unsigned char util_bin_to_byte(unsigned char *p);
int util_byte_to_bin(unsigned char *dst, unsigned char b);
int util_str_len(char *p);
int util_str_cmp(char *str1, char *str2);
int util_str_copy(char *dest, char *src, int max_count);
int util_append_str(char *dst, char *src, int max);
int util_cpy_substr(char *dst, char *src, char delim, int max);

#endif /* UTIL_H_ */
