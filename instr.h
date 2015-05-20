/*
 * instr.h
 *
 *  Created on: Apr 15, 2015
 *      Author: jaa
 */

#ifndef INSTR_H_
#define INSTR_H_

// This is needed, because one (unsigned) integer can't carry
// address validity information (UNDEFINED, UNPREDICTABLE, ...)
#include "instr_comm.h"

// finds out the next address after executing the instruction at 'address'
instr_next_addr_t next_address(unsigned int address);

#endif /* INSTR_H_ */
