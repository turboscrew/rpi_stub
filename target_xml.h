/*
target_xml.h

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

#ifndef TARGET_XML_H_
#define TARGET_XML_H_

#define MAX_TARGET_DESCR_LEN 4096

// the buffer where the description is generated
typedef struct
{
	int len; // generated descriptor length
//	int ptr; // current pointer for sending in fragments
	char buff[MAX_TARGET_DESCR_LEN]; // the generated descriptor
} target_xml;

// arhitectures
typedef enum
{
	arch_arm,
	arch_last
} arch_type_t;

void gen_target(target_xml *buf, arch_type_t arch);


#endif /* TARGET_XML_H_ */
