/*
log.h

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

#ifndef LOG_H_
#define LOG_H_

// stuff for debugging

#define LOGGING_ON

#include "io_dev.h"

#ifndef LOGGING_ON

#define LOG_PR_STR(x)
#define LOG_PR_STR_CONT(x)
#define LOG_PR_VAL(x, y)
#define LOG_PR_VAL_CONT(x, y)
#define LOG_NEWLINE()

#else

#define LOG_PR_STR(x) \
	log_pr_head(__builtin_FILE (), __builtin_LINE ());\
	log_pr_str(x)

#define LOG_PR_STR_CONT(x) log_pr_str(x)

#define LOG_PR_VAL(x, y) \
	log_pr_head(__builtin_FILE (), __builtin_LINE ());\
	log_pr_val((x),(y))

#define LOG_PR_VAL_CONT(x, y) log_pr_val((x),(y))

#define LOG_NEWLINE() log_pr_str("\r\n")

#endif

// initialize logging
void log_init(io_device *device);
// print log header
void log_pr_head(const char *file, int line);
// print a log string - blocking
void log_pr_str(char *str);
// print an unsigned integer
void log_pr_val(char *str, unsigned int val);

#endif /* LOG_H_ */
