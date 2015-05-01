/*
 * instr.h
 *
 *  Created on: Apr 15, 2015
 *      Author: jaa
 */

#ifndef INSTR_H_
#define INSTR_H_

// marker: address can't be resolved here - like exceptions
#define INSTR_ADDR_NONE 0xffffffff
// marker: instruction is UNDEF
#define INSTR_ADDR_UNDEF 0xfffffffe
// marker: instruction is UNPREDICTABLE
#define INSTR_ADDR_UNPRED 0xfffffffd

// finds out the branch info about the instruction at address
unsigned int check_branching(unsigned int address);

#endif /* INSTR_H_ */
