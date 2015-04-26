/*
 * instr.h
 *
 *  Created on: Apr 15, 2015
 *      Author: jaa
 */

#ifndef INSTR_H_
#define INSTR_H_

// marker: address can't be resolved here - like exceptions
#define INSTR_ADDR_NONE 0xfffffffff
// marker: address can't be used
#define INSTR_ADDR_UNDEF 0xfffffffe

// finds out the branch info about the instruction at address
unsigned int check_branching(unsigned int address);

#endif /* INSTR_H_ */
