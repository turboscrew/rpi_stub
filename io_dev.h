/*
io_dev.h

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

#ifndef IO_DEV_H_
#define IO_DEV_H_

/* A driver-kind of thing just in case... */

typedef struct
{
	void (*start)();
	int (*get_char)();
	int (*put_char)(char);
	int (*get_string)(char *, int);
	int (*put_string)(char *, int);
	int (*read)(char *, int);
	int (*write)(char *, int);
	void (*set_ctrlc)(void *);
	int (*get_ctrl_c)();

}io_device;


#endif /* IO_DEV_H_ */
