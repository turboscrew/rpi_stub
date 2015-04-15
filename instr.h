/*
 * instr.h
 *
 *  Created on: Apr 15, 2015
 *      Author: jaa
 */

#ifndef INSTR_H_
#define INSTR_H_

#define INSTR_BRTYPE_LINEAR 0
#define INSTR_BRTYPE_BRANCH 1
#define INSTR_BRTYPE_CONDBR 2
#define INSTR_BRTYPE_NONE 3

#define INSTR_BRVAL_NONE 0
#define INSTR_BRVAL_REG 1
#define INSTR_BRVAL_IMM 2
#define INSTR_BRVAL_REG_IMM 3

// Note that linear instruction doesn't have branch address
typedef struct {
	int btype; // branch type
	int bvaltype; // branch value type (16-bit offset, absolute,...)
	unsigned int bval; // branch value
} branch_info;

// finds out the branch info about the instruction at address
void check_branching(unsigned int address, branch_info *info);

#endif /* INSTR_H_ */
