/*
gdb.c

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

#include <stdint.h>
#include "serial.h"
#include "rpi2.h"
#include "util.h"
#include "gdb.h"
#include "instr.h"

//#define DEBUG_GDB

// 'reasons'-Events mapping (see below)
// SIG_INT = ctrl-C
// SIG_TRAP = bkpt
// SIG_ILL = undef
// SIG_BUS = abort
// SIG_USR1 = unhandled HW interrupt
// SIG_USR2 = unhandled SW interrupt
// Any reason without defined reason
// => 'O'-packet?

// 'reasons' for target halt
#define SIG_INT  2
#define SIG_TRAP 5
#define SIG_ILL  4
#define SIG_BUS  7
#define SIG_USR1 10
#define SIG_USR2 12
#define ALOHA 32
#define FINISHED 33
#define PANIC 34

#define GDB_MAX_BREAKPOINTS 64
#define GDB_MAX_MSG_LEN 1024

// breakpoint
typedef struct {
	void *trap_address;
	union {
	uint32_t arm;
	uint16_t thumb;
	} instruction;
	int trap_kind;
	int valid;	// 1=valid
} gdb_trap_rec;

// debug breakpoints set by user
volatile gdb_trap_rec gdb_usr_breakpoint[GDB_MAX_BREAKPOINTS];
// number of breakpoints in use
volatile int gdb_num_bkpts = 0;

// breakpoint for single-stepping
volatile gdb_trap_rec gdb_step_bkpt;
volatile int gdb_single_stepping = 0; // flag: 1 = currently single stepping
static volatile uint32_t gdb_single_stepping_address = 0xffffffff; // step until this
static volatile int gdb_trap_num = -1; // breakpoint number in case of bkpt
static int gdb_resuming = -1; // flag for single stepping over resumed breakpoint
// program to be debugged
volatile gdb_program_rec gdb_debuggee;

// device for communicating with gdb client
volatile io_device *gdb_iodev;

// gdb I/O packet buffers
static volatile uint8_t gdb_in_packet[GDB_MAX_MSG_LEN]; // packets from gdb
static volatile uint8_t gdb_out_packet[GDB_MAX_MSG_LEN]; // packets to gdb
static volatile uint8_t gdb_tmp_packet[GDB_MAX_MSG_LEN]; // for building packets

// flag: 0 = return to debuggee, 1 = stay in monitor
static volatile int gdb_monitor_running = 0;

#ifdef DEBUG_GDB
static char scratchpad[16]; // scratchpad for debugging
#endif

// The 'command interpreter'
void gdb_monitor(int reason);

// resume needs this
void gdb_do_single_step();

// packet sending
int gdb_send_packet(char *src, int count);

// ckeck if cause of exception was a breakpoint
// return breakpoint number, or -1 if none
int gdb_check_breakpoint()
{
	uint32_t pc;
	int num = -1;
	int i;

	pc = rpi2_reg_context.reg.r15;
	// single stepping?
	if (gdb_step_bkpt.valid)
	{
		if (gdb_step_bkpt.trap_address == (void *)pc)
		{
			num = GDB_MAX_BREAKPOINTS;
		}
	}
	// user breakpoint?
	else
	{
		for (i=0; i<GDB_MAX_BREAKPOINTS; i++)
		{
			if (gdb_usr_breakpoint[i].trap_address == (void *)pc)
			{
				if (gdb_usr_breakpoint[i].valid)
				{
					num = i;
				}
			}
		}
	}
	return num;
}

// exception handler
void gdb_trap_handler()
{
	int break_char = 0;
	int reason = 0;
#ifdef DEBUG_GDB
	char *msg;
	static char scratchpad[16];
#endif
	gdb_trap_num = -1;
	reason = exception_info;
#if 0
	// currently done in rpi2.c
	// IRQs need to be enabled for serial I/O
	uint32_t irqmask = ~(1<<7);
	asm volatile (
			"push r0\n\t"
			"mrs r0, cpsr\n\t"
			"and r0, irqmask"
			"msr r0, cpsr\n\t"
			"pop r0\n\t"
	);
#endif

#ifdef DEBUG_GDB
	msg = "\r\ngdb_trap_handler\r\nexception_info = ";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, exception_info);
	scratchpad[8]='\0'; // end-nul
	gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
	msg = " exception_extra = ";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, exception_extra);
	scratchpad[8]='\0'; // end-nul
	gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
	gdb_iodev->put_string("\r\n", 3);
#endif

	if (rpi2_get_sigint_flag() == 1)
	{
		rpi2_set_sigint_flag(0); // handled
		// ctrl-C pressed
		reason = SIG_INT;
		// flush input - read until ctrl-c
		//while (gdb_iodev->get_char() != 3);
	}
	else if (exception_extra > 0xff00) // internal error
	{
		reason = PANIC;
	}
	else
	{
		switch (exception_info)
		{
		case RPI2_EXC_NONE:
			reason = ALOHA;
			break;
		case RPI2_EXC_RESET:
			// figure this out - shouldn't happen
			reason = PANIC; // for now
			break;
		case RPI2_EXC_PABT:
			// bkpt (ARM) or bkpt (THUMB)
			if ((exception_extra == RPI2_TRAP_ARM) || (exception_extra == RPI2_TRAP_THUMB))
			{
				gdb_trap_num = gdb_check_breakpoint();
				if (gdb_trap_num < 0)
				{
					// not one of our breakpoints
					reason = SIG_USR2;
				}
				else
				{
					// one of ours
					reason = SIG_TRAP;
				}
			}
			else if (exception_extra == RPI2_TRAP_INITIAL)
			{
				reason = ALOHA;
			}
			else // pabt
			{
				reason = SIG_BUS;
			}
			break;
		case RPI2_EXC_DABT:
			reason = SIG_BUS;
			break;
		case RPI2_EXC_UNDEF:
			reason = SIG_ILL;
			break;
		case RPI2_EXC_SVC:
			reason = SIG_USR2;
			break;
		case RPI2_EXC_AUX:
			reason = SIG_USR2;
			break;
		case RPI2_EXC_IRQ:
			reason = SIG_USR1;
			break;
		case RPI2_EXC_FIQ:
			reason = SIG_USR1;
			break;
		default:
			reason = PANIC;
			break;
		}
	}
	gdb_monitor(reason);
}

/* cause trap - enter gdb stub */
void gdb_trap()
{
	rpi2_trap();
}

/* initialize this gdb stub */
void gdb_init(io_device *device)
{
	/* store I/O device to be used */
	gdb_iodev = device;
	// number of breakpoints in use
	gdb_num_bkpts = 0;

	gdb_single_stepping = 0; // flag: 0 = currently not single stepping
	gdb_single_stepping_address = 0xffffffff; // not valid
	gdb_trap_num = -1; // breakpoint number
	gdb_resuming = -1; // flag for single stepping over resumed breakpoint
	// flag: 0 = return to debuggee, 1 = stay in monitor
	gdb_monitor_running = 0;
	/* install trap handler */
	//rpi2_set_vector(RPI2_EXC_TRAP, &gdb_trap_handler);
	/* install ctrl-c handling */
	rpi2_set_sigint_flag(0);
	gdb_iodev->set_ctrlc((void *)rpi2_pend_trap);
#ifdef DEBUG_GDB
	gdb_iodev->put_string("\r\ngdb_init\r\n", 13);
#endif
	return;
}

/* Clean-up after last session */
void gdb_reset()
{
	int i;
	/* clean up user breakpoints */
	for (i=0; i<GDB_MAX_BREAKPOINTS; i++)
	{
		gdb_usr_breakpoint[i].trap_address = (void *)0xffffffff;
		gdb_usr_breakpoint[i].instruction.arm = 0;
		gdb_usr_breakpoint[i].valid = 0;
	}
	gdb_num_bkpts = 0;
	// clean up single stepping breakpoints
	gdb_step_bkpt.instruction.arm = 0;
	gdb_step_bkpt.trap_address = (void *)0xffffffff;
	gdb_step_bkpt.valid = 0;
	gdb_single_stepping = 0;

	// reset debuggee info
	gdb_debuggee.start_addr = (void *)0x00008000; // default
	gdb_debuggee.size = 0;
	gdb_debuggee.entry = (void *)0x00008000; // default
	gdb_debuggee.curr_addr = (void *)0x00008000; // default
	gdb_debuggee.status = 0;
	return;
}

/* set a breakpoint to given address */
/* returns -1 if trap can't be set */
int dgb_set_trap(void *address, int kind)
{
	int i;
	// char dbg_help[32];
	// char scratchpad[10];
	uint32_t *p = (uint32_t *) address;
	if (gdb_num_bkpts == GDB_MAX_BREAKPOINTS)
	{
		return -1;
	}
	for (i=0; i<GDB_MAX_BREAKPOINTS; i++)
	{
		if (!gdb_usr_breakpoint[i].valid)
		{
			gdb_usr_breakpoint[i].instruction.arm = *p;
			gdb_usr_breakpoint[i].trap_address = p;
			gdb_usr_breakpoint[i].trap_kind = kind;
			gdb_usr_breakpoint[i].valid = 1;
			gdb_num_bkpts++;
			rpi2_set_trap(address, kind);
			/*
			util_str_copy(dbg_help, "Oset trap: i =", 25);
			util_word_to_hex(scratchpad, i);
			scratchpad[8]='\0'; // end-nul
			util_append_str(dbg_help, scratchpad, 32);
			util_append_str(dbg_help, "\r\n", 32);
			//gdb_iodev->put_string(dbg_help, util_str_len(dbg_help)+1);
			gdb_send_packet(dbg_help, util_str_len(dbg_help));
			*/
			return 0; // success
		}
	}
	return -1; // failure - something wrong with the table
}

int dgb_unset_trap(void *address, int kind)
{
	int i;
	// char dbg_help[32];
	// char scratchpad[10];
	uint32_t *p = (uint32_t *) address;
	if (gdb_num_bkpts == 0)
	{
		return -1;
	}
	for (i=0; i<GDB_MAX_BREAKPOINTS; i++)
	{
		if (gdb_usr_breakpoint[i].valid)
		{
			if (gdb_usr_breakpoint[i].trap_address == p)
			{
				if (gdb_usr_breakpoint[i].trap_kind == kind)
				{
					gdb_usr_breakpoint[i].trap_address = (void *)0xffffffff;
					gdb_usr_breakpoint[i].valid = 0;
					gdb_num_bkpts--;
					*p = gdb_usr_breakpoint[i].instruction.arm;
					/*
					util_str_copy(dbg_help,"Ounset trap: i =", 25);
					util_word_to_hex(scratchpad, i);
					scratchpad[8]='\0'; // end-nul
					util_append_str(dbg_help, scratchpad, 32);
					util_append_str(dbg_help, "\r\n", 32);
					//gdb_iodev->put_string(dbg_help, util_str_len(dbg_help)+1);
					gdb_send_packet(dbg_help, util_str_len(dbg_help));
					*/
					return 0; // success
				}
				else
				{
					return -1; // wrong kind
				}
			}
		}
	}
	return -1; // failure - not found
}

void gdb_resume()
{
	int i;
	gdb_trap_rec *p;
	if (gdb_resuming >= 0)
	{

		if (gdb_resuming < GDB_MAX_BREAKPOINTS) // user bkpt
		{
			// re-apply valid breakpoints - needed?
			/*
			for (i=0; i<GDB_MAX_BREAKPOINTS; i++)
			{
				if (i != gdb_resuming) // skip the breakpoint to be resumed
				{
					p = &(gdb_usr_breakpoint[i]);
					if (p->valid)
					{
						rpi2_set_trap(p->trap_address, p->trap_kind);
					}
				}
			}
			*/
			// single step old instruction
			//gdb_do_single_step();
		}
	}
}

/*
 * Packet data formats
 */

// return value gives the number of bytes sent (to the output buffer)
// count = bytes to send, max = output buffer size
int gdb_write_hex_data(uint8_t *indata, int count, char *hexdata, int max)
{
	int i = 0; // indata counter
	int j = 0; // hexdata counter
	uint8_t byte;
	uint8_t hex;
	unsigned char *src = (unsigned char *)indata;
	unsigned char *dst = (unsigned char *)hexdata;
	while (i < count) // 1 byte = 2 chars
	{
		if (j + 2 > max) break; // both output characters wouldn't fit
		byte = *(src++);
		util_byte_to_hex(dst, byte);
		dst += 2;
		j += 2;
		i++;
	}
	return i;
}

// return value gives the number of bytes sent (to the output buffer)
// count = bytes to send, max = output buffer size
// no run length coding for now
int gdb_write_bin_data(uint8_t *indata, int count, uint8_t *bindata, int max)
{
	int i = 0; // indata counter
	int j = 0; // bindata counter
	int len;
	unsigned char *src = (unsigned char *)indata;
	unsigned char *dst = (unsigned char *)bindata;
	while (i < count)
	{
		// if escaped, both output bytes would not fit
		if (j + 2 > max) break;
		len = util_byte_to_bin(dst, *(src++));
		dst += len;
		j += len;
		i++;
	}
	return i;
}

// return value gives the number of bytes received (from the hexdata buffer)
// count = bytes to receive, max = output buffer size
int gdb_read_hex_data(uint8_t *hexdata, int count, uint8_t *outdata, int max)
{
	int i = 0; // hexdata counter
	int j = 0; // outdata counter
	uint8_t inbyte;
	unsigned char *src = (unsigned char *)hexdata;
	unsigned char *dst = (unsigned char *)outdata;

	while (j < max)
	{
		// check that there are at least two digits left
		// 1 byte = 2 chars
		if (i + 2 > count*2) break;
		*(dst++) = util_hex_to_byte(src);
		src += 2;
		j++;
		i += 2;
	}

	return i;
}

// return value gives the number of bytes received (from the bindata buffer)
// count = bytes to receive, max = output buffer size
int gdb_read_bin_data(uint8_t *bindata, int count, uint8_t *outdata, int max)
{
	int i = 0; // bindata counter
	int j = 0; // outdata counter
	//uint8_t inbyte;
	int cnt;
	unsigned char *src = (unsigned char *)bindata;
	unsigned char *dst = (unsigned char *)outdata;
	while (j < count)
	{
		if (i + 2 > max) break;
		cnt = util_bin_to_byte(src, dst);
//		inbyte = *(src++);
//		if (inbyte == 0x7d) // escaped
//		{
//			i++; // escape char
//			inbyte = *(src++) ^ 0x20; // escaped char
//		}
//		*(dst++) = inbyte;
		src += cnt;
		dst++;
		i+= cnt;
		j++;
	}
	return i;
}

/*
 * I/O packets
 */

// Returns packet length or negative number indicating a special case
// Special cases:
// -1 = max. packet size exceeded
// -2 = checksum doesn't match
// -3 = BRK received
// -4 = Other error
int receive_packet(char *dst)
{
	int len;
	int ch;
	int tmp;
	int checksum;
	int chksum;
	while ((ch = gdb_iodev->get_char()) != (int)'$') // Wait for '$'
	{
		// fake ack/nack packets
		if ((char)ch == '+')
		{
			len = util_str_copy(dst, "+", 2);
			return len;
		}
		else if ((char)ch == '-')
		{
			len = util_str_copy(dst, "-", 2);
			return len;
		}
	}
	// receive payload
	len = 0;
	checksum = 0;
	while ((ch = gdb_iodev->get_char()) != (int)'#')
	{
		if (ch == -1) continue; // wait for char

		// calculate checksum
		checksum += ch;
		checksum %= 256;

		// copy payload data
		*(dst++) = (char) ch;
		len++;
		// check for buffer overflow
		if (len == GDB_MAX_MSG_LEN)
		{
			len = -1; // unsupported packet size error
			break;
		}
	}

	*dst = '\0'; // end-NUL
	// if no errors this far, handle checksum
	if (len > 0)
	{
		// get first checksum hex digit and check for BRK
		// the '#' gets dropped
		while ((ch = gdb_iodev->get_char()) == -1);

		// upper checksum hex digit to binary
		chksum = util_hex_to_nib((char) ch);
		if (chksum < 0)
		{
			return -4; // not hex digit
		}

		// get second checksum hex digit and check for BRK
		while ((ch = gdb_iodev->get_char()) == -1);

		// lower checksum hex digit to binary
		tmp = util_hex_to_nib((char) ch);
		if (tmp < 0)
		{
			return -4; // Other error
		}
		// combine upper and lower to checksum
		chksum = (chksum << 4) + tmp;

		// check read checksum against the one we calculated
		if (chksum != checksum)
		{
			len = -2; // Checksum mismatch
		}
	}
	return len;
}

int gdb_send_packet(char *src, int count)
{
	int checksum = 0;
	int cnt = 1; // message byte count
	int ch; // temporary for checksum handling
	char *ptr = (char *) gdb_out_packet;
	// add '$'
	*(ptr++) = '$';
	// add payload, check length and calculate checksum
	while (*src != '\0')
	{
		// double assignment
		checksum += (int)(*(ptr++) = *(src++));
		checksum &= 0xff;
		cnt++;
		// '#' + checksum take 3 bytes
		if (cnt > GDB_MAX_MSG_LEN - 3)
		{
			return -1; // error - too long message
		}
	}
	// add '#'
	*(ptr++) = '#';

	// add checksum - upper nibble
	ch = util_nib_to_hex((checksum & 0xf0)>> 4);
	if (ch > 0) *(ptr++) = (char)ch;
	else return -1; // overkill
	// add checksum - lower nibble
	ch = util_nib_to_hex(checksum & 0x0f);
	if (ch > 0) *(ptr) = (char)ch;
	else return -1; // overkill
	// update msg length after checksum
	cnt += 3;
	// send
	gdb_iodev->write((char *)gdb_out_packet, cnt);
	return cnt;
}

void gdb_packet_ack()
{
	//gdb_iodev->put_string("$+#2b", 6);
	gdb_iodev->put_char('+');
}

void gdb_packet_nack()
{
	//gdb_iodev->put_string("$-#2d", 6);
	gdb_iodev->put_char('-');
}

void gdb_response_not_supported()
{
	gdb_iodev->put_string("$#00", 5);
}

/*
 * gdb_commands
 */


// answer to query '?'
void gdb_resp_target_halted(int reason)
{
	int len;
	char *err = "E00";
	const int scratch_len = 16;
	const int resp_buff_len = 128; // should be enough to hold any response
	char scratchpad[scratch_len]; // scratchpad
	char resp_buff[resp_buff_len]; // response buffer
#ifdef DEBUG_GDB
	char *msg;
#endif

	gdb_monitor_running = 1; // by default

	if (reason == 0)
	{
		// No debuggee yet
		if (gdb_debuggee.size == 0)
		{
			reason = ALOHA;
		}
	}
	switch(reason)
	{
	case SIG_INT: // ctrl-C
		// S02 - response for ctrl-C
		len = util_str_copy(resp_buff, "S02", resp_buff_len);
		break;
	case SIG_TRAP: // bkpt
		// T05 - breakpoint response
		len = util_str_copy(resp_buff, "T05", resp_buff_len);
		// PC value given
		//len = util_append_str(resp_buff, "f:", resp_buff_len);
		// put PC-value into message
		//util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		//len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_ILL: // undef
		// T04 - undef response
		len = util_str_copy(resp_buff, "T04", resp_buff_len);
		// PC value given
		//len = util_append_str(resp_buff, "f:", resp_buff_len);
		// put PC-value into message
		//util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		//len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_BUS: // pabt/dabt
		// T07 - pabt/dabt response
		len = util_str_copy(resp_buff, "T07", resp_buff_len);
		// PC value given
		//len = util_append_str(resp_buff, "f:", resp_buff_len);
		// put PC-value into message
		//util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		//len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_USR1: // unhandled HW interrupt - no defined response
		// send 'OUnhandled HW interrupt'
		len = util_str_copy(resp_buff, "OUnhandled HW interrupt", resp_buff_len);
		gdb_send_packet(resp_buff, len);
		len = util_str_copy(resp_buff, err, resp_buff_len);
		break;
	case SIG_USR2: // unhandled SW interrupt - no defined response
		// send 'OUnhandled SW interrupt'
		len = util_str_copy(resp_buff, "OUnhandled SW interrupt", resp_buff_len);
		gdb_send_packet(resp_buff, len);
		len = util_str_copy(resp_buff, err, resp_buff_len);
		break;
	case ALOHA: // no debuggee loaded yet - no defined response
		// send 'Ogdb stub started'
		//len = util_str_copy(resp_buff, "Ogdb stub started\n", resp_buff_len);
		//gdb_send_packet(resp_buff, len);
		// application terminated with status 0
		len = util_str_copy(resp_buff, "S05", resp_buff_len);
		break;
	case FINISHED: // program has finished
		// send 'W<exit status>' response
		len = util_str_copy(resp_buff, "W", resp_buff_len);
		// put exit status value into message
		util_byte_to_hex(scratchpad, gdb_debuggee.status);
		len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		// gdb_monitor_running = 0; // TODO: check
		break;
	default:
		// send 'Ounknown event'
		len = util_str_copy(resp_buff, "OUnknown event\n", resp_buff_len);
		gdb_send_packet(resp_buff, len);
		gdb_response_not_supported();
		return; // don't send anything more
		break;
	}
	// send response
	gdb_send_packet(resp_buff, len);
}

// c [addr]
void gdb_cmd_cont(char *src, int len)
{
	uint32_t address;
	int bkptnum;
	if (len > 1)
	{
		address = util_hex_to_word(src);
		rpi2_reg_context.reg.r15 = address;
	}
	// breakpoint?
	bkptnum = gdb_check_breakpoint();

	if (bkptnum > 0)
	{
		gdb_resuming = bkptnum;
		gdb_resume();
	}
	gdb_monitor_running = 0; // return
}

// g
void gdb_cmd_get_regs(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len, i;
	const int resp_buff_len = 512; // should be enough to hold 'g' response
	static char resp_buff[512]; // response buff
	const int tmp_buff_len = 256; // should be enough to hold 'g' response
	static char tmp_buff[256]; // response buff
#if 0
	char scratchpad[16]; // scratchpad
#endif
#if 0
	char *p1, *p2;
#else
	uint32_t *p1, *p2;
#endif
#if 0
	util_word_to_hex(scratchpad, packet_len);
	gdb_iodev->put_string("\r\ng_cmd: ", 10);
	gdb_iodev->put_string(scratchpad, 9);
	gdb_iodev->put_string(" =len: ", 10);
	gdb_iodev->put_string((char *)gdb_in_packet, packet_len);
	gdb_iodev->put_string("\r\n", 3);
#endif

	if (packet_len >= 0)
	{
		// response for 'g' - regs r0 - r15 + cpsr
#if 0
		p1 = (char *)&(rpi2_reg_context.storage);
		p2 = tmp_buff;
		// regs reported
		for (i=0; i<16; i++) // r0 - r15
		{
			util_swap_bytes(p1, p2);
			p1 += 4;
			p2 += 4;
		}
#else
		p1 = (uint32_t *)&(rpi2_reg_context.storage);
		p2 = (uint32_t *)tmp_buff;
		for (i=0; i<16; i++) // r0 - r15
		{
			*(p2++) = *(p1++);
		}

#endif
		//for (i=0; i<9*4; i++) // 8 x fp + fps (all dummy)
		for (i=16; i<25; i++) // 8 x fp + fps (all dummy)
		{
			*(p2++) = 0x90000009;
		}
		// p1 = (char *)&(rpi2_reg_context.reg.cpsr);
		// util_swap_bytes(p1, p2);
		*p2 = rpi2_reg_context.reg.cpsr;
		len = gdb_write_hex_data(tmp_buff, 4*26, resp_buff, resp_buff_len);
#if 0
		util_word_to_hex(scratchpad, len);
		gdb_iodev->put_string("\r\ng_cmd2: ", 10);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" =len: ", 10);
		gdb_iodev->put_string(resp_buff, 26*4);
		gdb_iodev->put_string("\r\n", 3);
#endif
		gdb_send_packet(resp_buff, len);
	}
}

// G
void gdb_cmd_set_regs(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len, i;
	char *resp_str = "OK";
	const int resp_buff_len = 512; // should be enough to hold 'g' response
	static char resp_buff[512]; // response buff
	const int tmp_buff_len = 256; // should be enough to hold 'g' response
	static char tmp_buff[256]; // response buff
#if 0
	char scratchpad[16]; // scratchpad
#endif
	//char *p1, *p2;
	uint32_t *p1, *p2;

	if (packet_len > 0)
	{
		gdb_read_hex_data((char *)gdb_in_packet, packet_len,
				tmp_buff, tmp_buff_len);
		p2 = (uint32_t *)tmp_buff;
		p1 = (uint32_t *)&(rpi2_reg_context.storage);
#if 0
		util_word_to_hex(scratchpad, packet_len);
		gdb_iodev->put_string("\r\nG_cmd: ", 10);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" =len: ", 10);
		gdb_iodev->put_string((char *)gdb_in_packet, 26*4);
		gdb_iodev->put_string("\r\n", 3);
#endif
		// 17 regs
		for (i=0; i<16; i++) // r0 - r15
		{
/*
			util_swap_bytes(p2, p1);
			p1 += 4;
			p2 += 4;

*/
			*(p1++) = *(p2++);
		}
		// for (i=0; i<9*4; i++) // 8 x fp + fps (all dummy)
		for (i=16; i<25; i++) // 8 x fp + fps (all dummy)
		{
			p2++;
		}
		*p2 = (uint32_t)(rpi2_reg_context.reg.cpsr);
		// p2 = (char *)&(rpi2_reg_context.reg.cpsr);
		// util_swap_bytes(p1, p2);
		// send response
		gdb_send_packet(resp_str, util_str_len(resp_str));

	}
}

// m addr,length
void gdb_cmd_read_mem_hex(volatile uint8_t *gdb_in_packet, int packet_len)
{
	uint32_t addr;
	uint32_t bytes;
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (packet_len > 0)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		addr = util_hex_to_word(scratchpad); // address to binary
		bytes = util_hex_to_word((char *)gdb_in_packet); // read nuber of bytes
#if 0
		gdb_iodev->put_string("\r\nm_cmd: addr= ", 16);
		util_word_to_hex(scratchpad, addr);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" len= ", 10);
		util_word_to_hex(scratchpad, bytea);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string("\r\n", 3);
#endif
		// dump memory as hex into temp buffer
		len = gdb_write_hex_data((uint8_t *)addr, (int)bytes, (char *)gdb_tmp_packet,
				 GDB_MAX_MSG_LEN - 5); // -5 to allow message overhead
		// send response
		gdb_send_packet((char *)gdb_tmp_packet, len);
	}
}

// M addr,length:XX XX ...
void gdb_cmd_write_mem_hex(volatile uint8_t *gdb_in_packet, int packet_len)
{
	uint32_t addr;
	uint32_t bytes;
	int len;
	char *resp_str = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (packet_len > 0)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		packet_len -= (len+1);
		addr = util_hex_to_word(scratchpad); // address to binary
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ':', scratch_len);
		gdb_in_packet += len+1; // skip bytecount and delimiter
		packet_len -= (len+1);
		bytes = util_hex_to_word(scratchpad); // address to binary
#if 0
		gdb_iodev->put_string("\r\nM_cmd: addr= ", 16);
		util_word_to_hex(scratchpad, addr);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" len= ", 10);
		util_word_to_hex(scratchpad, bytea);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string("\r\ndata:", 10);
		gdb_iodev->put_string((char *)gdb_in_packet, packet_len);
		gdb_iodev->put_string("\r\n", 3);
#endif
		// write to memory
		gdb_read_hex_data((char *)gdb_in_packet, (int)bytes, (uint8_t *) addr,
				GDB_MAX_MSG_LEN); // can't be more than message size
		// send response
		gdb_send_packet(resp_str, util_str_len(resp_str));
	}
}

// p n
void gdb_cmd_read_reg(volatile uint8_t *gdb_in_packet, int packet_len)
{
	uint32_t value, tmp;
	uint32_t reg;
	uint32_t *p;
	char *cp;
	char *err = "E00";
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (packet_len > 1)
	{
		reg = (uint32_t)util_hex_to_word((char *)gdb_in_packet);
		if (reg < 16)
		{
			//p = (uint32_t *) &(rpi2_reg_context.storage);
			//value = p[reg];
			value = rpi2_reg_context.storage[reg];
			util_swap_bytes((unsigned char *)(&value), (unsigned char *)(&tmp));
			util_word_to_hex(scratchpad, tmp);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		else if ((reg > 15) && (reg < 25)) // dummy fp-register
		{
			util_word_to_hex(scratchpad, 0x90000009);
			util_swap_bytes((unsigned char *)(&value), (unsigned char *)(&tmp));
			util_word_to_hex(scratchpad, tmp);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		else if (reg == 25) // cpsr
		{
			//p = (uint32_t *) &(rpi2_reg_context.storage);
			//value = p[reg];
			value = rpi2_reg_context.reg.cpsr;
			util_word_to_hex(scratchpad, value);
			util_swap_bytes((unsigned char *)(&value), (unsigned char *)(&tmp));
			util_word_to_hex(scratchpad, tmp);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		else
		{
			gdb_send_packet(err, util_str_len(err));
		}
#if 0
		gdb_iodev->put_string("\r\np_cmd: ", 10);
		util_word_to_hex(scratchpad, reg);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string(" : ", 4);
		util_word_to_hex(scratchpad, value);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string("\r\n", 3);
#endif
	}
}

// P n...=r...
void gdb_cmd_write_reg(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	char *resp_str = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	uint32_t value, tmp;
	uint32_t reg;
	uint32_t *p;
	char *err = "E00";
	if (packet_len > 1)
	{
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, '=', scratch_len);
		reg = (uint32_t)util_hex_to_word(scratchpad);
		gdb_in_packet += (len + 1);  // skip register number and '='
		packet_len -= (len + 1);
		tmp = (uint32_t)util_hex_to_word((char *)gdb_in_packet);
		util_swap_bytes((unsigned char *)&tmp, (unsigned char *)&value);
#if 0
		gdb_iodev->put_string((char *)gdb_in_packet, packet_len);
		gdb_iodev->put_string("\r\nP_cmd: ", 10);
		util_word_to_hex(scratchpad, reg);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string(" plen: ", 10);
		util_word_to_hex(scratchpad, packet_len);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string(" tmp: ", 10);
		util_word_to_hex(scratchpad, tmp);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string(" val: ", 10);
		util_word_to_hex(scratchpad, value);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string("\r\n", 3);
#endif
		if ((reg > 15) && (reg < 25))
		{
			// gdb_send_packet("OK", 2); // dummy fp-register
		}
		else if (reg < 16)
		{
			//util_swap_bytes((unsigned char *)(&value), (unsigned char *)(&tmp));
			// p = (uint32_t *) &(rpi2_reg_context.storage);
			// p[reg] = value;
			rpi2_reg_context.storage[reg] = value;
		}
		else if (reg == 25) // cpsr
		{
			//util_swap_bytes((unsigned char *)(&value), (unsigned char *)(&tmp));
			// p = (uint32_t *) &(rpi2_reg_context.storage);
			// p[reg] = value;
			rpi2_reg_context.reg.cpsr = value;
		}
		else
		{
			gdb_send_packet(err, util_str_len(err));
			return;
		}
		gdb_send_packet(resp_str, util_str_len(resp_str));
	}
	else
	{
		gdb_send_packet(err, util_str_len(err));
	}
}

void gdb_cmd_detach(volatile uint8_t *gdb_packet, int packet_len)
{
	gdb_send_packet("OK", 2); // for now
}

void gdb_cmd_set_thread(volatile uint8_t *gdb_packet, int packet_len)
{
	int len;
	int num;
	char *msg;
	char *packet;
	const int scratch_len = 32;
	const int resp_buff_len = 128; // should be enough to hold any response
	char scratchpad[scratch_len]; // scratchpad
	char resp_buff[resp_buff_len]; // response buffer

	packet = (char *)gdb_packet;
	resp_buff[0] = '\0';
	if (packet_len > 1)
	{

		if (*packet == 'H')
		{
#ifdef DEBUG_GDB
			gdb_iodev->put_string("\r\nH-cmd: ", 10);
			gdb_iodev->put_string(packet, util_str_len(packet)+1);
			gdb_iodev->put_string("\r\n", 3);
#endif
			packet++; // skip the read and delimiter
			if (--packet_len == 0)
			{
				gdb_send_packet("E01", 3);
				return;
			}
			if ((*packet == 'g') || (*packet == 'c'))
			{
				packet++;
				if (--packet_len == 0)
				{
					gdb_send_packet("E02", 3);
					return;
				}
				// get thread ID
				len = util_read_dec(packet, &num);
				if (len == 0)
				{
					gdb_send_packet("E03", 3);
					return;
				}
				if ((packet_len -= len) > 0)
				{
					gdb_response_not_supported();
					return;
				}

				// thread ID -1 and 0 are OK
				// -1 = all, 0 = arbitrary
				if ((num == -1) || (num == 0))
				{
					gdb_send_packet("OK", 2);
				}
				else
				{
					gdb_response_not_supported();
				}
			}
			else
			{
				gdb_response_not_supported(); // for now
			}
		}
		else
		{
			gdb_send_packet("E04", 3);
		}
	}
	else
	{
		gdb_send_packet("E05", 3);
	}
}

// q - for single core bare metal, fake single process (PID = 1)
// If non-SMP config, the cores are different targets, if SMP-config,
// then ad-hoc way to switch cores
void gdb_cmd_common_query(volatile uint8_t *gdb_packet, int packet_len)
{
	int len;
	char *msg;
	char *packet;
	const int scratch_len = 32;
	const int resp_buff_len = 128; // should be enough to hold any response
	char scratchpad[scratch_len]; // scratchpad
	char resp_buff[resp_buff_len]; // response buffer
	// q name params
	// qSupported [:gdbfeature [;gdbfeature]... ]
	// PacketSize=bytes  value required
	packet = (char *)gdb_packet;
	resp_buff[0] = '\0';
	if (packet_len > 1)
	{
		len = util_cpy_substr(scratchpad, packet, ':', scratch_len);
#ifdef DEBUG_GDB
		gdb_iodev->put_string("cmd: ", 6);
		gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
		gdb_iodev->put_string("\r\n", 3);
#endif
		packet += len+1; // skip the read and delimiter
		// len = util_cmp_substr(packet, "qSupported:");
		// if (len == util_str_len("qSupported:"))
		// {
		// 		packet += len;
		if (util_str_cmp(scratchpad, "qSupported") == 0)
		{
#if 0
			packet_len -= (len + 1);
			while (packet_len > 0)
			{
				len = util_cpy_substr(scratchpad, packet, ';', scratch_len);
				packet += len+1; // the read and delimiter
				packet_len -= (len + 1);
#ifdef DEBUG_GDB
				gdb_iodev->put_string("\r\nparam: ", 12);
				gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
#endif
				// reply: 'stubfeature [;stubfeature]...'
				// 'name=value', 'name+' or 'name-'

				if (util_str_cmp(scratchpad, "PacketSize") == 0)
				{
					len = util_append_str(resp_buff, scratchpad, resp_buff_len);
					len = util_append_str(resp_buff, "=", resp_buff_len);
					util_word_to_hex(resp_buff + len, GDB_MAX_MSG_LEN);
					len = util_str_len(resp_buff);
				}
				else
				{
					/* unsupported feature */
					len = util_append_str(resp_buff, scratchpad, resp_buff_len);
					len = util_append_str(resp_buff, "-", resp_buff_len);
				}
				if (packet_len > 0)
				{
					len = util_append_str(resp_buff, ";", resp_buff_len);
				}
			}
#ifdef DEBUG_GDB
			gdb_iodev->put_string("resp: ", 8);
			gdb_iodev->put_string(resp_buff, util_str_len(resp_buff)+1);
			gdb_iodev->put_string("\r\n", 3);
#endif
#else

			len = util_str_copy(resp_buff, "swbreak+;PacketSize=", resp_buff_len);
			util_word_to_hex(scratchpad, GDB_MAX_MSG_LEN);
			len = util_append_str(resp_buff, scratchpad, resp_buff_len);
#endif
			gdb_send_packet(resp_buff, len);
		}
		else if (util_str_cmp(scratchpad, "qOffset") == 0)
		{
			// reply: 'Text=xxxx;Data=xxxx;Bss=xxxx'
			len = util_str_copy(resp_buff, "Text=8000;Data=8000;Bss=8000", resp_buff_len);
			gdb_send_packet(resp_buff, len);
		}
		else if (util_str_cmp(scratchpad, "qC") == 0)
		{
			// reply: 'QC1' - current thread id = 1
			// reply: <empty> - NULL-thread is used
#if 0
			len = util_str_copy(resp_buff, "QC1", resp_buff_len);
			gdb_send_packet(resp_buff, len);
#else
			gdb_response_not_supported(); // empty response - NULL-thread
#endif
		}
		else if (util_str_cmp(scratchpad, "qAttached") == 0)
		{
			// reply: '1' - Attached to existing process
			// reply: '0' - new process created
			len = util_str_copy(resp_buff, "0", resp_buff_len);
			gdb_send_packet(resp_buff, len);
		}
		else if (util_str_cmp(scratchpad, "qSymbol") == 0)
		{
			// reply: 'OK' - Offer to provide symbol values (none)
			len = util_str_copy(resp_buff, "OK", resp_buff_len);
			gdb_send_packet(resp_buff, len);
		}
		else
		{
			gdb_response_not_supported(); // for now
		}
	}
}

// R XX
void gdb_cmd_restart_program(volatile uint8_t *gdb_in_packet, int packet_len)
{

}

void gdb_do_single_step(void)
{
	instr_next_addr_t next_addr;
	unsigned int curr_addr;
	int len;
	const int resp_buff_len = 128; // should be enough to hold any response
	char resp_buff[resp_buff_len]; // response buffer
	char *err = "E01";
	char *okresp = "OK";

	curr_addr = rpi2_reg_context.reg.r15; // stored PC

	if (gdb_single_stepping_address != 0xffffffff)
	{
		if (gdb_single_stepping_address == (uint32_t)curr_addr)
		{
			// stop single stepping
			gdb_single_stepping = 0; // just to be sure
			gdb_single_stepping_address = 0xffffffff;
			// send response
			len = util_str_copy(resp_buff, "OTarget address reached", resp_buff_len);
			gdb_send_packet((char *)resp_buff, len);
			gdb_send_packet(okresp, util_str_len(okresp)); // response
			gdb_monitor_running = 1;
			return;
		}
	}
	// step
	next_addr = next_address(curr_addr);
	if (next_addr.flag & (~INSTR_ADDR_UNPRED) == INSTR_ADDR_UNDEF)
	{
		// stop single stepping
		gdb_single_stepping = 0; // just to be sure
		gdb_single_stepping_address = 0xffffffff;
		// send response
		len = util_str_copy(resp_buff, "ONext instruction is UNDEFINED", resp_buff_len);
		gdb_send_packet((char *)resp_buff, len); // response
		gdb_send_packet(err, util_str_len(err));
		gdb_monitor_running = 1;
		return;
	}
	else if (next_addr.flag & INSTR_ADDR_UNPRED)
	{
		// TODO: configuration - allow unpredictables
		// stop single stepping
		gdb_single_stepping = 0; // just to be sure
		gdb_single_stepping_address = 0xffffffff;
		// send response
		len = util_str_copy(resp_buff, "ONext instruction is UNPREDICTABLE", resp_buff_len);
		gdb_send_packet((char *)resp_buff, len);
		gdb_send_packet(err, util_str_len(err)); // response
		gdb_monitor_running = 1;
		return;
	}
	// store original instruction (only ARM instruction set supported yet)
	// TODO: add thumb-mode
	gdb_step_bkpt.trap_address = (void *) next_addr.address;
	gdb_step_bkpt.instruction.arm = *((uint32_t *)next_addr.address);
	gdb_step_bkpt.trap_kind = RPI2_TRAP_ARM;
	gdb_step_bkpt.valid = 1;
	// patch single stepping breakpoint
	rpi2_set_trap(gdb_step_bkpt.trap_address, RPI2_TRAP_ARM);
	gdb_monitor_running = 0;
	// Delayed response - sent when stepping is done
}

// s [addr]
void gdb_cmd_single_step(volatile uint8_t *gdb_in_packet, int packet_len)
{
	uint32_t address;
	int len;
	const int resp_buff_len = 128; // should be enough to hold any response
	char resp_buff[resp_buff_len]; // response buffer
	char *err = "E02";

	if (packet_len > 1)
	{
		// get hex for address
		address = util_hex_to_word((char *)gdb_in_packet); // address to binary
		if (gdb_single_stepping_address == 0xffffffff)
		{
			// Start single stepping to address
			gdb_single_stepping_address = address;
		}
		else
		{
			// single stepping in progress already (shouldn't happen)
			len = util_str_copy(resp_buff, "OAlready single stepping", resp_buff_len);
			gdb_send_packet((char *)resp_buff, len);
			gdb_send_packet(err, util_str_len(err));
			return;
		}
	}
	gdb_single_stepping_address = 0xffffffff; // just one single step
	gdb_do_single_step();
}

// X addr,len: XX XX ...
void gdb_cmd_write_mem_bin(volatile uint8_t *gdb_in_packet, int packet_len)
{
	char ok_resp[]="OK";
	uint32_t addr;
	uint32_t bytes;
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	// X addr,length:XX XX ...
	if (packet_len > 1)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		packet_len -= (len + 1);
		addr = util_hex_to_word(scratchpad); // address to binary
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ':', scratch_len);
		gdb_in_packet += len+1; // skip bytecount and delimiter
		packet_len -= (len + 1);
		bytes = util_hex_to_word(scratchpad); // address to binary

		// write to memory
		gdb_read_bin_data((char *)gdb_in_packet, (int)bytes, (uint8_t *) addr,
				GDB_MAX_MSG_LEN); // can't be more than message size
			// send response
		gdb_send_packet(ok_resp, util_str_len(ok_resp));
	}

}

// x addr,len
void gdb_cmd_read_mem_bin(volatile uint8_t *gdb_in_packet, int packet_len)
{
	uint32_t addr;
	uint32_t bytes;
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len];
	if (packet_len > 1)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		addr = util_hex_to_word(scratchpad); // address to binary
		bytes = util_hex_to_word((char *)gdb_in_packet); // read nuber of bytes
		// dump memory as bin into temp buffer
		len = gdb_write_bin_data((uint8_t *)addr, (int)bytes, (uint8_t *)gdb_tmp_packet,
				 GDB_MAX_MSG_LEN - 5); // -5 to allow message overhead
		// send response
		gdb_send_packet((char *)gdb_tmp_packet, len);
	}
}

// Z1 - Z4 = HW breakpoints/watchpoints
// These breakpoint kinds are defined for the Z0 and Z1 packets.
// 2 16-bit Thumb mode breakpoint
// 3 32-bit Thumb mode (Thumb-2) breakpoint
// 4 32-bit ARM mode breakpoint
// Z0,addr,kind[;cond_list...][;cmds:persist,cmd_list...]
void gdb_cmd_add_breakpoint(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	uint32_t addr;
	uint32_t kind;
	char *err = "E01";
	char *okresp = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	// get addr
	if (*gdb_in_packet == ',') gdb_in_packet++; // skip ','
	len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
	gdb_in_packet += len+1; // skip address and delimiter
	addr = util_hex_to_word(scratchpad);
	kind = util_hex_to_word((char *)gdb_in_packet); // does this work?
	if (kind == 2) // 16-bit THUMB
	{
		if (dgb_set_trap((void *)addr, RPI2_TRAP_THUMB))
		{
			gdb_send_packet(err, util_str_len(err));
		}
		gdb_send_packet(okresp, util_str_len(okresp));
	}
	else if (kind == 4) // 32-bit ARM
	{
		if (dgb_set_trap((void *)addr, RPI2_TRAP_ARM))
		{
			gdb_send_packet(err, util_str_len(err));
		}
		gdb_send_packet(okresp, util_str_len(okresp));
	}
	else
	{
		gdb_response_not_supported();
	}
}

// z0,addr,kind
void gdb_cmd_delete_breakpoint(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	uint32_t addr;
	uint32_t kind;
	char *err = "E01";
	char *okresp = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (*gdb_in_packet == ',') gdb_in_packet++; // skip ',' after z0
	// get addr
	len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
	gdb_in_packet += len+1; // skip address and delimiter
	addr = util_hex_to_word(scratchpad);
	kind = util_hex_to_word((char *)gdb_in_packet); // does this work?

	if (kind == 2) // 16-bit THUMB
	{
		if (dgb_unset_trap((void *)addr, RPI2_TRAP_THUMB))
		{
			gdb_send_packet(err, util_str_len(err));
		}
		gdb_send_packet(okresp, util_str_len(okresp));
	}
	else if (kind == 4) // 32-bit ARM
	{
		if (dgb_unset_trap((void *)addr, RPI2_TRAP_ARM))
		{
			gdb_send_packet(err, util_str_len(err));
		}
		gdb_send_packet(okresp, util_str_len(okresp));
	}
	else
	{
		gdb_response_not_supported();
	}
}

void gdb_restore_breakpoint(volatile gdb_trap_rec *bkpt)
{
	if (bkpt->trap_kind == RPI2_TRAP_ARM)
	{
		*((uint32_t *)(bkpt->trap_address)) = bkpt->instruction.arm;
	}
	else
	{
		*((uint16_t *)(bkpt->trap_address)) = bkpt->instruction.thumb;
		return; // only ARM supported for now
	}
}

// handle stuff left pending until exception
void gdb_handle_pending_state(int reason)
{
	instr_next_addr_t next_addr;
	unsigned int curr_addr;
	int len, i;
	const int resp_buff_len = 128; // should be enough to hold any response
	char resp_buff[resp_buff_len]; // response buffer
	// char *err = "E01";
	char *okresp = "OK";
#ifdef DEBUG_GDB
	char *msg;
#endif
	curr_addr = rpi2_reg_context.reg.r15; // stored PC

	// break point or single step
	if (reason == SIG_TRAP)
	{
		gdb_monitor_running = 1; // by default
		// if single step
		if (gdb_trap_num == GDB_MAX_BREAKPOINTS)
		{
			gdb_restore_breakpoint(&gdb_step_bkpt);
			gdb_step_bkpt.valid = 0; // used -> removed
			// if single stepping to an address
			if (gdb_single_stepping_address != 0xffffffff)
			{
				if (gdb_single_stepping_address != curr_addr)
				{
					gdb_do_single_step();
					return; // response handled within the call
				}
				else
				{
					// stop 'automatic' single stepping
					gdb_single_stepping_address = 0xffffffff;
				}
			}
			if (gdb_resuming >= 0)
			{
				// re-apply resumed breakpoint
				rpi2_set_trap(gdb_usr_breakpoint[gdb_resuming].trap_address,
						gdb_usr_breakpoint[gdb_resuming].trap_kind);
				gdb_resuming = -1; // done
				gdb_monitor_running = 0; // resume kind of continues
				return;
			}
		}
		else
		{
			// should all breakpoints be restored - and re-installed at resume?
			/*
			for (i = 0; i < GDB_MAX_BREAKPOINTS; i++)
			{
				gdb_restore_breakpoint(&(gdb_usr_breakpoint[i]));
			}
			*/
			//gdb_restore_breakpoint(&(gdb_usr_breakpoint[gdb_trap_num]));
		}
		// response generated later
	}
	if (reason == SIG_INT)
	{
		if (!gdb_monitor_running)
		{
			// program execution stopped by ctrl-C
			gdb_monitor_running = 1; // by default
		}
		else
		{
			// Needless ctrl-C - don't bother with response (?)
			return;
		}
	}
	// stop responses - responses to commands 'c', 's', and the like
	gdb_resp_target_halted(reason);
	// go to monitor
}

/* The main debugger loop */
void gdb_monitor(int reason)
{
	int packet_len;
	char *ch;
	char scratchpad[16];
	char *inpkg = (char *)gdb_in_packet;
#ifdef DEBUG_GDB
	char *msg;
#endif

#ifdef DEBUG_GDB
	msg = "\r\n\r\nEntering GDB-monitor\r\nreason = ";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, reason);
	scratchpad[8]='\0'; // end-nul
	gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
#endif

	gdb_handle_pending_state(reason);

#ifdef DEBUG_GDB
	msg = "\r\ngdb_monitor_running = ";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, gdb_monitor_running);
	scratchpad[8]='\0'; // end-nul
	gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
	gdb_iodev->put_string("\r\n", 3);
#endif

	// don't let CTRL-C interfere with binary data transfer
	if (gdb_monitor_running)
	{
		gdb_iodev->set_ctrlc((void *)0);
	}

	while(gdb_monitor_running)
	{
		packet_len = (int)serial_get_rx_dropped();
		if (packet_len)
		{
			util_str_copy(scratchpad, "Odrop:", 16);
			util_word_to_hex(scratchpad+6, packet_len);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		packet_len = (int)serial_get_rx_ovr();
		if (packet_len)
		{
			util_str_copy(scratchpad, "Oovr:", 16);
			util_word_to_hex(scratchpad+6, packet_len);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}

		packet_len = receive_packet(inpkg);

#ifdef DEBUG_GDB
		if (packet_len < 0)
		{
			util_word_to_hex(scratchpad, -packet_len);
			scratchpad[8]='\0'; // end-nul
			gdb_iodev->put_string("\r\npacket_len= -", 18);
			gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
		}
		else
		{
			util_word_to_hex(scratchpad, packet_len);
			scratchpad[8]='\0'; // end-nul
			gdb_iodev->put_string("\r\npacket_len= ", 18);
			gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
			gdb_iodev->put_string("\r\n", 3);
			if (packet_len > 0)
			{
				gdb_iodev->put_string(inpkg, packet_len+1);
			}
		}
		gdb_iodev->put_string("\r\n", 3);
#endif
		if (packet_len < 0)
		{
			// send NACK
			gdb_packet_nack();
			// flush, re-read
			packet_len = 0;
#ifdef DEBUG_GDB
			gdb_iodev->put_string("\r\nsent nack\r\n", 16);
#endif
			continue;

		}
		else
		{
			if (packet_len == 0)
			{
				// dummy
				continue;
			}

			// packet ch points to beginning of buffer
			ch = inpkg;

			// TODO: add retransmission
			if (packet_len == 1)
			{
				// ACK received
				if (*ch == '+')
				{
#ifdef DEBUG_GDB
					gdb_iodev->put_string("\r\ngot ack\r\n", 13);
#endif
					// flush, re-read
					packet_len = 0;
					continue; // for now
				}
				// NACK received
				if (*ch == '-')
				{
#ifdef DEBUG_GDB
					gdb_iodev->put_string("\r\ngot nack\r\n", 13);
#endif
					// flush, re-read
					packet_len = 0;
					continue; // for now
				}
			}

			// unbroken packet received
			gdb_packet_ack();
#ifdef DEBUG_GDB
			gdb_iodev->put_string("\r\nsent ack\r\n", 16);
#endif

			// Commands
			switch (*ch)
			{
			case '?':	// why halted
				gdb_resp_target_halted(reason);
				break;
			case 'A':	// args
				gdb_response_not_supported(); // for now
				break;
			case 'c':	// continue +
				gdb_cmd_cont(++inpkg, --packet_len);
				break;
			case 'D':	// detach
				gdb_cmd_detach(++inpkg, --packet_len);
				break;
			case 'F':	// file I/O - for gdb console
				gdb_response_not_supported(); // for now
				break;
			case 'g':	// get regs +
				gdb_cmd_get_regs(++inpkg, --packet_len);
				break;
			case 'G':	// set regs +
				gdb_cmd_set_regs(++inpkg, --packet_len);
				break;
			case 'H':	// set thread - dummy
				gdb_cmd_set_thread(inpkg, packet_len);
				break;
			case 'k':	// kill
				gdb_response_not_supported(); // for now
				break;
			case 'm':	// read mem hex +
				gdb_cmd_read_mem_hex(++inpkg, --packet_len);
				break;
			case 'M':	// write mem hex +
				gdb_cmd_write_mem_hex(++inpkg, --packet_len);
				break;
			case 'p':	// read reg +
				gdb_cmd_read_reg(++inpkg, --packet_len);
				break;
			case 'P':	// write reg +
				gdb_cmd_write_reg(++inpkg, --packet_len);
				break;
			case 'q':	// query + (support)
				gdb_cmd_common_query(inpkg, packet_len);
				break;
			case 'Q':	// set
				gdb_response_not_supported(); // for now
				break;
			case 'R':	// restart program
				gdb_cmd_restart_program(inpkg, packet_len);
				break;
			case 's':	// single step +
				gdb_cmd_single_step(++inpkg, --packet_len);
				break;
			case 'T':	// thread alive?
				gdb_response_not_supported(); // for now
				break;
			case 'v':	// v-command
				gdb_response_not_supported(); // for now
				break;
			case 'X':	// write mem binary
				gdb_cmd_write_mem_bin(++inpkg, --packet_len);
				break;
			case 'x':	// read mem binary
				gdb_cmd_read_mem_bin(++inpkg, --packet_len);
				break;
			case 'Z':	// add Z0-breakpoint +
				// Z1 - Z4 = HW breakpoints/watchpoints
				if (*(++inpkg) == '0') // if Z0
				{
					gdb_cmd_add_breakpoint(++inpkg, --packet_len);
				}
				else // Z1 - Z4 not supported
				{
					gdb_response_not_supported();
				}
				break;
			case 'z':	// remove Z0-breakpoint +
				// z1 - z4 = HW breakpoints/watchpoints
				if (*(++inpkg) == '0') // if z0
				{
					gdb_cmd_delete_breakpoint(++inpkg, --packet_len);
				}
				else // z1 - z4 not supported
				{
					gdb_response_not_supported();
				}
				break;
			default:
				gdb_response_not_supported();
				break; // needless, but some tools want this
			}
		}
	}
	// enable CTRL-C
	gdb_iodev->set_ctrlc((void *)rpi2_pend_trap);
	return;
}
