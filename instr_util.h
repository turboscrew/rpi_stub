/*
 * instr_util.h
 *
 *  Created on: May 27, 2015
 *      Author: jaa
 */

#ifndef INSTR_UTIL_H_
#define INSTR_UTIL_H_

// gets high half word as signed int
int instr_util_shgethi(unsigned int val);
// gets low half word as signed int
int instr_util_shgetlo(unsigned int val);
// sign-extends the lower short in unsigned int to signed integer
int instr_util_signx_short(unsigned int val);
// sign-extends the lowest byte in unsigned int to signed integer
int instr_util_signx_byte(unsigned int val);
// stuffs two signed shorts into high and low half words of an unsigned int
unsigned int instr_util_ustuffs16(int val1, int val2);
// stuffs two unsigned shorts into high and low half words of an unsigned int
unsigned int instr_util_ustuffu16(unsigned int val1, unsigned int val2);
// stuffs four unsigned bytes into an unsigned int
unsigned int instr_util_ustuffu8(unsigned int byte3, unsigned int byte2, unsigned int byte1, unsigned int byte0);
// stuffs four signed bytes into an unsigned int
unsigned int instr_util_ustuffs8(int byte3, int byte2, int byte1, int byte0);
// saturate unsigned integer
unsigned int instr_util_usat(int val, unsigned int bitc);
// saturate signed integer
int instr_util_ssat(int val, unsigned int bitc);
// saturate signed long long integer
long long int instr_util_lssat(long long int val, unsigned int bitc);
// right-rotate bytes of an unsigned integer
unsigned int instr_util_rorb(unsigned int val, int count);
// swap half words of an unsigned integer
unsigned int instr_util_swaph(unsigned int val);
#endif /* INSTR_UTIL_H_ */
