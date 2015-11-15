/*
log.c

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

#include "log.h"
#include "util.h"

io_device *logdev;

void log_init(io_device *device)
{
	logdev = device;
}

/*
	// Fill up the device structure
	device->put_char = serial_put_char;
	device->put_string = serial_put_string;
	device->write = serial_write;
*/

void log_pr_head(const char *file, int line)
{
	char scratch[9];
	logdev->put_string("\r\n", 3);
	logdev->put_string((char *)file, util_str_len((char *)file));
	logdev->put_string(" ", 2);
	util_word_to_hex(scratch, (unsigned int)line);
	logdev->put_string(scratch, util_str_len(scratch));
	logdev->put_string("\r\n", 3);
}

void log_pr_str( char *str)
{
	logdev->put_string(str, util_str_len(str));
}

void log_pr_val(char *str, unsigned int val)
{
	char scratch[9];

	if (*str != '\0')
	{
		logdev->put_string(str, util_str_len(str));
		logdev->put_string(" ", 2);
	}
	util_word_to_hex(scratch, val);
	logdev->put_string(scratch, util_str_len(scratch));
}
