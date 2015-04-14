/*
 * io_dev.h
 *
 *  Created on: Mar 28, 2015
 *      Author: jaa
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
