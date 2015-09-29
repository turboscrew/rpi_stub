/*
ARM_decode_table.h

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

#ifndef ARM_DECODE_TABLE_H_
#define ARM_DECODE_TABLE_H_

#include "instr_comm.h"
instr_next_addr_t ARM_decoder_dispatch(unsigned int instr);

#endif /* ARM_DECODE_TABLE_H_ */
