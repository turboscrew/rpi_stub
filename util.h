/*
 * util.h
 *
 *  Created on: Apr 12, 2015
 *      Author: jaa
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
