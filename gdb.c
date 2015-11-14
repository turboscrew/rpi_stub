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
#include "log.h"

// 'reasons'-Events mapping (see below)
// SIG_INT = ctrl-C
// SIG_TRAP = bkpt
// SIG_ILL = undef
// SIG_BUS = abort
// SIG_USR1 = unhandled HW interrupt
// SIG_USR2 = unhandled SW interrupt
// SIG_STOP = Any reason without defined reason

// 'reasons' for target halt
#define SIG_INT  RPI2_REASON_SIGINT
#define SIG_TRAP 5
#define SIG_ILL  4
#define SIG_BUS  10
#define SIG_EMT  7
#define SIG_USR1 RPI2_REASON_HW_EXC
#define SIG_USR2 RPI2_REASON_SW_EXC
#define ALOHA 32
#define FINISHED 33
#define PANIC 34

#define GDB_MAX_BREAKPOINTS 64
#define GDB_MAX_WATCHPOINTS 4
#define GDB_MAX_MSG_LEN 1024

//extern void loader_main();
extern volatile uint32_t rpi2_debuggee_running; // for debug

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

// watchpoint
typedef struct {
	void *address;
	uint32_t type;
	uint32_t size;
	uint32_t value;
	uint32_t mask;
	uint32_t bas;
	int valid;	// 1=valid
} gdb_watch_rec;

// debug breakpoints set by user
volatile gdb_watch_rec gdb_usr_watchpoint[GDB_MAX_WATCHPOINTS];
// number of breakpoints in use
volatile int gdb_num_watchps = 0;

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

// features
static uint32_t gdb_swbreak;
static uint32_t gdb_hwbreak;

// flag: 0 = return to debuggee, 1 = stay in monitor
static volatile int gdb_monitor_running = 0;

volatile uint32_t gdb_dyn_debug;

#ifdef DEBUG_GDB
static char scratchpad[16]; // scratchpad for debugging
#endif

// The 'command interpreter'
void gdb_monitor(int reason);

// resume needs this
void gdb_do_single_step();

// packet sending
int gdb_send_packet(char *src, int count);

// check if cause of exception was a breakpoint
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

void gdb_clear_breakpoints(int remove_traps)
{
	int i;
	uint32_t *tmp;

	/* clean up user breakpoints */
	for (i=0; i<GDB_MAX_BREAKPOINTS; i++)
	{
		if (remove_traps)
		{
			// restore 'bkpts'
			if (gdb_usr_breakpoint[i].valid)
			{
				tmp = (uint32_t *)gdb_usr_breakpoint[i].trap_address;
				*tmp = gdb_usr_breakpoint[i].instruction.arm;
				rpi2_flush_address((unsigned int) tmp);
				SYNC;
			}
		}
		gdb_usr_breakpoint[i].trap_address = (void *)0xffffffff;
		gdb_usr_breakpoint[i].instruction.arm = 0;
		gdb_usr_breakpoint[i].valid = 0;
	}
	gdb_num_bkpts = 0;
	// clean up single stepping breakpoints
	if (remove_traps)
	{
		// restore 'bkpts'
		if (gdb_step_bkpt.valid)
		{
			tmp = (uint32_t *)gdb_step_bkpt.trap_address;
			*tmp = gdb_step_bkpt.instruction.arm;
			rpi2_flush_address((unsigned int) tmp);
			SYNC;
		}
	}
	gdb_step_bkpt.instruction.arm = 0;
	gdb_step_bkpt.trap_address = (void *)0xffffffff;
	gdb_step_bkpt.valid = 0;
	gdb_single_stepping = 0;
}

// return watchpoint number, or -1 if none
int gdb_check_watchpoint()
{
	uint32_t tmp;
	int num = -1;
	int i;
#if 0
	char scratch[9];
#endif

	num = 0;
	for (i=0; i<GDB_MAX_WATCHPOINTS; i++)
	{
		if (gdb_usr_watchpoint[i].valid)
		{
			if (gdb_usr_watchpoint[i].size > 4)
			{
				if (rpi2_dbg_rec.far & gdb_usr_watchpoint[i].mask
					== gdb_usr_watchpoint[i].value)
				{
					num = i;
					break;
				}
			}
			else
			{
				if ((rpi2_dbg_rec.far & (~7)) == gdb_usr_watchpoint[i].value)
				{
					tmp = rpi2_dbg_rec.far & 7;
					if ((1 << tmp) & gdb_usr_watchpoint[i].bas)
					{
						num = i;
						break;
					}
				}
			}
#if 0
	gdb_iodev->put_string("\r\nwatch! num= ", 15);
	util_word_to_hex(scratch, num);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string(" far= ", 7);
	util_word_to_hex(scratch, rpi2_dbg_rec.far);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string("\r\nmask= ", 9);
	util_word_to_hex(scratch, gdb_usr_watchpoint[num].mask);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string(" value= ", 4);
	util_word_to_hex(scratch, gdb_usr_watchpoint[num].value);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string(" bas= ", 4);
	util_word_to_hex(scratch, gdb_usr_watchpoint[num].bas);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string("\r\n", 3);
#endif
		}
	}
	if (i >= GDB_MAX_WATCHPOINTS) return -1; // not found

	return num;
}

void gdb_clear_watchpoints(int remove_traps)
{
	int i;
	for(i=0; i<GDB_MAX_WATCHPOINTS; i++)
	{
		if (remove_traps)
		{
			if (gdb_usr_watchpoint[i].valid)
			{
				// remove watchpoint
				rpi2_unset_watchpoint((unsigned int) i);
			}
		}
		gdb_usr_watchpoint[i].valid = 0;
		gdb_usr_watchpoint[i].address = (void *)0;
		gdb_usr_watchpoint[i].size = 0;
		gdb_usr_watchpoint[i].type = 0;
		gdb_usr_watchpoint[i].value = 0;
		gdb_usr_watchpoint[i].mask = 0;
		gdb_usr_watchpoint[i].bas = 0;
	}
	gdb_num_watchps = 0;
}

// exception handler
void gdb_trap_handler()
{
	int break_char = 0;
	int reason = 0;

//#ifdef DEBUG_GDB
#if 1
	char *msg;
	static char scratchpad[16];
#endif
	gdb_trap_num = -1;
	reason = exception_info;

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
		reason = rpi2_get_exc_reason();
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
			else if (exception_extra == RPI2_TRAP_BKPT)
			{
				// not one of our breakpoints
				reason = SIG_USR2;
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
			if (exception_extra == RPI2_TRAP_WATCH)
			{
				reason = SIG_TRAP;
			}
			else
			{
				reason = SIG_EMT;
			}
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
	rpi2_gdb_trap();
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
	gdb_swbreak = 0;
	gdb_hwbreak = 0;
	// flag: 0 = return to debuggee, 1 = stay in monitor
	gdb_monitor_running = 0;
	gdb_dyn_debug = 0;
#ifdef DEBUG_GDB
	gdb_iodev->put_string("\r\ngdb_init\r\n", 13);
#endif
	return;
}

/* Clean-up after last session */
void gdb_reset(int remove_traps)
{
	unsigned int cpsr_store;

	//rpi2_unset_watchpoint(0);

	cpsr_store = rpi2_disable_save_ints();
	rpi2_set_vectors();
	gdb_iodev->enable_ctrlc();
	rpi2_set_sigint_flag(0);
	set_gdb_enabled(1); // enable gdb calls
	gdb_clear_breakpoints(remove_traps);
	gdb_clear_watchpoints(remove_traps);
#ifdef GDB_INVALIDATE_CACHES
	if (rpi2_use_mmu)
	{
		rpi2_invalidate_caches();
	}
#endif

	rpi2_restore_ints(cpsr_store);

	//rpi2_set_watchpoint(0, (unsigned int)(&gdb_num_bkpts), 4);

	// reset debuggee info
	gdb_debuggee.start_addr = (void *)0x00008000; // default
	gdb_debuggee.size = 0;
	gdb_debuggee.entry = (void *)0x00008000; // default
	gdb_debuggee.curr_addr = (void *)0x00008000; // default
	gdb_debuggee.status = 0;
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
		return 1;
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
	return 3; // failure - something wrong with the table
}

int dgb_unset_trap(void *address, int kind)
{
	int i;
	// char dbg_help[32];
	char scratchpad[10];
	uint32_t *p = (uint32_t *) address;
	if (gdb_num_bkpts == 0)
	{
		return 1;
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
					rpi2_flush_address((unsigned int)p);
					asm volatile("dsb\n\tisb\n\t");
#ifdef DEBUG_GDB_EXC
					util_word_to_hex(scratchpad, (unsigned int) p);
					gdb_iodev->put_string("\r\ninstr at: ", 13);
					gdb_iodev->put_string(scratchpad, 9);
					gdb_iodev->put_string("\r\n", 3);
#endif
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
					return 2; // wrong kind
				}
			}
		}
	}
	return 3; // failure - not found
}

int dgb_add_watchpoint(uint32_t type, uint32_t addr, uint32_t bytes)
{
	int i, num;
	uint32_t control;
	uint32_t value;
	uint32_t alignment;
	uint32_t mask;
	uint32_t bas;
	uint32_t tmp;
	char scratch[9];

	if (bytes == 0) return -2; // invalid range

	for(num=0; num<GDB_MAX_WATCHPOINTS; num++)
	{
		if (gdb_usr_watchpoint[num].valid == 0)
		{
			break;
		}
	}
	if (num >= GDB_MAX_WATCHPOINTS) return -1; // full

	gdb_usr_watchpoint[i].type = type;
	gdb_usr_watchpoint[i].size = bytes;
	gdb_usr_watchpoint[i].address = (void *)addr;

	// Cortex-A8 Technical Reference Manual
	// 12.4.16. Watchpoint Control Registers
	// if 8-bit bas is implemented, alignment must be doubleword
	// the basic idea is probably: one breakpoint - one type (aligned) and the
	// range is maybe meant for memory pages and such (the 2**n limitation)
	if (bytes <= 4)
	{
		value = addr & (~7); // word aligned address
		alignment = addr & 7;	// first byte in doubleword
		bas = 0;
		for (i=0; i<bytes; i++)
		{
			bas |= (1 << (alignment + i));
		}
		mask = 0;
		gdb_usr_watchpoint[num].mask = ((~0) << 3);
	}
	else
	{
		// check alignment
		alignment = 0;
		tmp = bytes;
		while (tmp >>= 1)
		{
			alignment++;
		}
		if (alignment > 31) return -2; // invalid range
		if (alignment < 4) return -2; // invalid range

		if (bytes & (~(1 << (alignment - 1)))) // more than 1 bit set?
		{
			return -2; // illegal range, must be 2**n bytes
		}
#if 1
		if (addr & ((1 << alignment) -1 )) return -3; // address not aligned with range
#else
		// make base alignment consistent with the range
		value = addr & (~((1<<alignment) - 1)); // mask off lower bits
#endif
		mask = alignment - 1;
		bas = 0xff; // must be if mask is used
		gdb_usr_watchpoint[num].mask = ((~0) << mask);
	}
	control = mask << 24; // MASK
	control |= (1 << 13); // HMC
	control |= (bas << 5); // BAS: 4 bytes
	control |= (type << 3);
	control |= (3 << 1); // PAC
	control |= 1; // enable

#if 0
	gdb_iodev->put_string("\r\nwatch num= ", 14);
	util_word_to_hex(scratch, num);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string(" val= ", 4);
	util_word_to_hex(scratch, value);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string(" ctrl= ", 4);
	util_word_to_hex(scratch, control);
	gdb_iodev->put_string(scratch, 9);
	gdb_iodev->put_string("\r\n", 3);
#endif
	rpi2_set_watchpoint((unsigned int) num, (unsigned int) value,
			(unsigned int) control);
	gdb_usr_watchpoint[num].bas = bas;
	gdb_usr_watchpoint[num].value = value;
	gdb_usr_watchpoint[num].valid = 1;
	gdb_num_watchps++;
	return num;
}

int dgb_del_watchpoint(uint32_t type, uint32_t addr, uint32_t bytes)
{
	int i;

	for(i=0; i<GDB_MAX_WATCHPOINTS; i++)
	{
		if (gdb_usr_watchpoint[i].valid)
		{
			if (gdb_usr_watchpoint[i].address == (void *) addr)
			{
				if (gdb_usr_watchpoint[i].size == bytes)
				{
					if (gdb_usr_watchpoint[i].type == type)
					{
						// delete watchpoint
						rpi2_unset_watchpoint((unsigned int) i);
						gdb_usr_watchpoint[i].valid = 0;
						gdb_num_watchps--;
						return i;
					}
				}
			}
		}
	}
	return -1; // not found
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
	*dst = '\0';
	return j;
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

	while (i < count) // here i counts bytes, not hex-digits
	{
		// check that there are at least two digits left
		// 1 byte = 2 chars
		if (j + 1 > max) break;
		*(dst++) = util_hex_to_byte(src);
		src += 2;
		j++;
		i++;
	}
	return i;
}

// return value gives the number of bytes received (from the bindata buffer)
// count = bytes to receive, max = output buffer size
int gdb_read_bin_data(uint8_t *bindata, int count, uint8_t *outdata, int max)
{
	int j = 0; // bindata counter (bytes, not buffer size)
	int i = 0; // outdata counter
	//uint8_t inbyte;
	int cnt;
	unsigned char *src = (unsigned char *)bindata;
	unsigned char *dst = (unsigned char *)outdata;
	while (j < count)
	{
		if (i + 2 > max) break;
		cnt = util_bin_to_byte(src, dst);
		src += cnt;
		dst++;
		i++;
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
	int tmp, i;
	int checksum;
	int chksum;
	int got;
	char *curr;
#if defined(GDB_DEBUG_RX) || defined(GDB_DEBUG_RX_LED)
	static char scratchpad[16]; // scratchpad
	uint32_t tm1, tm2, led, xcpsr;
	volatile uint32_t *tmr;
	char *msg;
	char *buff = dst;
#endif

#ifdef GDB_DEBUG_RX
	msg = "receive packet\r\n";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

#ifdef GDB_DEBUG_RX_LED
	tmr = (volatile uint32_t *)SYSTMR_CLO;
	tm1 = *tmr;
	led = 1;
	rpi2_led_on();
	asm volatile
	(
			"mrs %[var_reg], cpsr\n\t"
			"dsb\n\t"
			:[var_reg] "=r" (xcpsr) ::
	);
	//serial_raw_puts("\r\nreceive:");
	//serial_raw_puts(" cpsr = ");
	//util_word_to_hex(scratchpad, xcpsr);
	//serial_raw_puts(scratchpad);
	//serial_raw_puts("\r\n");
#endif
	while ((ch = gdb_iodev->get_char()) != (int)'$') // Wait for '$'
	{
#ifdef GDB_DEBUG_RX_LED
		tm2 = *tmr;
		if (tm2 -tm1 > 1000000)
		{
			tm1 = tm2;
			if (led)
			{
				rpi2_led_off();
				led = 0;
			}
			else
			{
				rpi2_led_on();
				led = 1;
			}
		}
#endif
		// fake ack/nack packets
		if ((char)ch == '+')
		{
			len = util_str_copy(dst, "+", 2);
		}
		else if ((char)ch == '-')
		{
			len = util_str_copy(dst, "-", 2);
			return len;
		}
		else if ((char)ch == '#')
		{
#ifdef GDB_DEBUG_RX
			msg = "got '#': broken\r\n";
			gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif
			len = 1; // a broken packet
			break;
		}
	}
#ifdef GDB_DEBUG_RX_LED
	rpi2_led_on();
	led = 1;
#endif
#ifdef GDB_DEBUG_RX
	msg = "start packet\r\n";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

	// receive payload
	checksum = -1; // for broken packets
	curr = dst;
	if (ch != (int)'#') // if '#' - a broken packet
	{
		len = 0;
		checksum = 0; // initialize for calculation
#ifdef GDB_DEBUG_RX
		msg = "get payload\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

		i = 0;
		do // read the payload
		{
#if 0
			do // try to get at least one character
			{
				// try to read the payload at once
				// to reduce the number of calls
				got = gdb_iodev->get_string(dst, '#', GDB_MAX_MSG_LEN - 1 - len);
			} while (got == 0); // empty-handed, try again

			dst += got;
			len += got;

			// check what we got while, maybe, waiting for more
			for (i=0; i<got; i++)
			{
				if (*curr == '#') break; // end of payload - just in case
				// calculate checksum
				checksum += (int)*(curr++);
				checksum %= 256;
			}
#else
			got = gdb_iodev->get_string(dst, '#', GDB_MAX_MSG_LEN - 1 - len);
			if (got == 0)
			{
				if (i < len)
				{
					// calculate checksum
					checksum += (int)*(curr++);
					checksum %= 256;
					i++;
				}
			}
			dst += got;
			len += got;
			if (*(dst - 1) == '#') // if the whole payload is read
			{
				// finish the check
				while (*curr != '#')
				{
					checksum += (int)*(curr++);
					checksum %= 256;
				}
			}

#endif

#ifdef GDB_DEBUG_RX
			msg = "read: len = ";
			gdb_iodev->put_string(msg, util_str_len(msg)+1);
			util_word_to_hex(scratchpad, len);
			gdb_iodev->put_string(scratchpad, 9);
			gdb_iodev->put_string("\n\rgot = ", 9);
			util_word_to_hex(scratchpad, got);
			gdb_iodev->put_string(scratchpad, 9);
			msg = "\r\nnow: ";
			gdb_iodev->put_string(buff, len);
			msg = "\r\n";
#endif
		} while (*(dst - 1) != '#'); // if not the whole message, a new round
#ifdef GDB_DEBUG_RX
	msg = "payload\r\n";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif
		dst--; // back up to '#'
		len--;
		*dst = '\0'; // just in case...
#if 0
		// check the packet payload
		for (i=0; i<len; i++) // go through what we got
		{
			if (*curr == '#') break; // end of payload - just in case
			// calculate checksum
			checksum += (int)*(curr++);
			checksum %= 256;
		}
#endif
	}
#ifdef GDB_DEBUG_RX
	msg = "end packet: len = ";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, len);
	gdb_iodev->put_string(scratchpad, 9);
#endif
	// if no errors this far, handle checksum
	if (len > 0)
	{
#ifdef GDB_DEBUG_RX
		msg = "checksum\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif
		// get first checksum hex digit and check for BRK
		// the '#' gets dropped
		while ((ch = gdb_iodev->get_char()) == -1);
#ifdef GDB_DEBUG_RX
		msg = "check-digit1\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

		// upper checksum hex digit to binary
		chksum = util_hex_to_nib((char) ch);
		if (chksum < 0)
		{
			return -4; // not hex digit
		}

		// get second checksum hex digit and check for BRK
		while ((ch = gdb_iodev->get_char()) == -1);
#ifdef GDB_DEBUG_RX
		msg = "check-digit2\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

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
#ifdef GDB_DEBUG_RX
		msg = "packet got\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
		rpi2_delay_loop(10);
#endif
	return len;
}

int gdb_send_packet(char *src, int count)
{
	int checksum = 0;
	int cnt = 0; // message byte count
	int ch; // temporary for checksum handling
	char *ptr = (char *) gdb_out_packet;
#ifdef GDB_DEBUG_TX
	int i, tmp;
	static char scratchpad[16];
#endif
	if (count > GDB_MAX_MSG_LEN - 4)
	{
#if 0
		gdb_iodev->put_string("send_packet: count!\r\n", 22);
#endif
		return -1;
	}
	// add '$'
	*(ptr++) = '$';
	// add payload, check length and calculate checksum
	while (cnt < count)
	{
		// double assignment
		checksum += (int)(*(ptr++) = *(src++));
		checksum &= 0xff;
		cnt++;
		// '#' + checksum take 3 bytes
	}
	// add '#'
	*(ptr++) = '#';

	// add checksum - upper nibble
	ch = util_nib_to_hex((checksum & 0xf0)>> 4);
	if (ch > 0) *(ptr++) = (char)ch;
	else return -1; // overkill
	// add checksum - lower nibble
	ch = util_nib_to_hex(checksum & 0x0f);
	if (ch > 0) *(ptr++) = (char)ch;
	else return -1; // overkill
	// update msg length after checksum
	cnt += 4; // add '$', '#' and two-digit checksum
	*ptr = '\0'; // just in case
#ifdef GDB_DEBUG_TX
	ptr = (char *) gdb_out_packet;
	for (i=0; i<cnt; i++)
	{
		if ((ptr[i] < 32) || (ptr[i] > 126))
		{
			gdb_iodev->write("\r\nbad ch, i= ", 14);
			util_word_to_hex(scratchpad, i);
			gdb_iodev->write(scratchpad, 9);
			gdb_iodev->write("\r\n", 3);
		}
	}
#endif
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
	gdb_send_packet("", 0);
	//gdb_iodev->put_string("$#00", 5);
}

void gdb_send_text_packet(char *msg, unsigned int msglen)
{
	int len;

	*gdb_tmp_packet = 'O';
	len = 1;
	len += gdb_write_hex_data((uint8_t *)msg, (int)msglen, (char *)(gdb_tmp_packet + 1),
					 GDB_MAX_MSG_LEN - 6); // -5 to allow message overhead

	gdb_send_packet((char *)gdb_tmp_packet, len);
}

/*
 * gdb_commands
 */

// answer to query '?'
// signal numbers from gdb_7.9.0/binutils-gdb/include/gdb/signals.def
void gdb_resp_target_halted(int reason)
{
	int len;
	char *err = "E00";
	const int scratch_len = 16;
	const int resp_buff_len = 128; // should be enough to hold any response
	char scratchpad[scratch_len]; // scratchpad
	char resp_buff[resp_buff_len]; // response buffer
	unsigned int tmp;
	int tmp2;
	char *text;
#ifdef DEBUG_GDB
	char *msg;
#endif

	gdb_monitor_running = 1; // by default

	switch(reason)
	{
	case SIG_INT: // ctrl-C
		// S02 - response for ctrl-C
		len = util_str_copy(resp_buff, "S02", resp_buff_len);
		break;
	case SIG_TRAP: // bkpt
		// T05 - breakpoint response - swbreak is not supported by gdb client
		len = util_str_copy(resp_buff, "T05", resp_buff_len);
		if (exception_info == RPI2_EXC_DABT)
		{
			tmp2 = gdb_check_watchpoint();
			if (tmp2 < 0)
			{
				text = "rpi_stub: Watchpoint not found";
				gdb_send_text_packet(text, util_str_len(text));
			}
			else
			{
				// if (gdb_hwbreak) needed?
				tmp = gdb_usr_watchpoint[tmp2].type;
				switch (tmp)
				{
				case 1: // read
					len = util_append_str(resp_buff, "rwatch:", resp_buff_len);
					break;
				case 2: // write
					len = util_append_str(resp_buff, "watch:", resp_buff_len);
					break;
				case 3: // access
					len = util_append_str(resp_buff, "awatch:", resp_buff_len);
					break;
				default:
					len = util_append_str(resp_buff, "awatch:", resp_buff_len);
					break;
				}
#if 0
				util_swap_bytes(&(rpi2_dbg_rec.far), &tmp);
				util_word_to_hex(scratchpad, tmp);
#else
				util_word_to_hex(scratchpad, rpi2_dbg_rec.far);
				len = util_append_str(resp_buff, scratchpad, resp_buff_len);
#endif
				len = util_append_str(resp_buff, ";", resp_buff_len);
			}
			//text = "Watchpoint";
			//gdb_send_text_packet(text, util_str_len(text));
		}
		else
		{
			//text = "Breakpoint";
			//gdb_send_text_packet(text, util_str_len(text));
			if (gdb_swbreak)
			{
				len = util_append_str(resp_buff, "swbreak:;", resp_buff_len);
			}
		}

		// PC value given - current gdb client has a bug - this doesn't work
		//len = util_append_str(resp_buff, "f:", resp_buff_len);
		// put PC-value into message
		//util_swap_bytes((unsigned int *)&(rpi2_reg_context.reg.r15), &tmp);
		//util_word_to_hex(scratchpad, tmp);
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
	case SIG_BUS: // pabt
		// T0A - pabt response (SIGBUS)
		text = "Prefetch Abort";
		gdb_send_text_packet(text, util_str_len(text));
		len = util_str_copy(resp_buff, "T0A", resp_buff_len);
		// PC value given
		//len = util_append_str(resp_buff, "f:", resp_buff_len);
		// put PC-value into message
		//util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		//len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_EMT: // dabt - used to tell PABT and DABT apart
		// T0A - dabt response (SIGBUS)
		text = "Data Abort";
		gdb_send_text_packet(text, util_str_len(text));
		len = util_str_copy(resp_buff, "T0A", resp_buff_len);
		// PC value given
		//len = util_append_str(resp_buff, "f:", resp_buff_len);
		// put PC-value into message
		//util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		//len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_USR1: // unhandled HW interrupt - no defined response
		// send 'OUnhandled HW interrupt' + T1F (SIGUSR1)
		text = "Unhandled HW interrupt";
		gdb_send_text_packet(text, util_str_len(text));
		//len = util_str_copy(resp_buff, "OUnhandled SW interrupt", resp_buff_len);
		//gdb_send_packet(resp_buff, len);
		len = util_str_copy(resp_buff, "T1f", resp_buff_len);
		break;
	case SIG_USR2: // unhandled SW interrupt - no defined response
		// send 'OUnhandled SW interrupt' + T1f (SIGUSR2)
		text = "Unhandled SW interrupt";
		gdb_send_text_packet(text, util_str_len(text));
		//len = util_str_copy(resp_buff, "OUnhandled SW interrupt", resp_buff_len);
		//gdb_send_packet(resp_buff, len);
		len = util_str_copy(resp_buff, "T1f", resp_buff_len);
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
		// send 'Ounknown event' + T11 (SIGSTOP)
		text = "Unknown event";
		gdb_send_text_packet(text, util_str_len(text));
		//len = util_str_copy(resp_buff, "OUnknown event", resp_buff_len);
		//gdb_send_packet(resp_buff, len);
		len = util_str_copy(resp_buff, "T11", resp_buff_len);
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
	if (len > 0)
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
	/*
	if (exception_info == RPI2_EXC_RESET)
	{
		rpi2_reg_context.reg.r15 = (unsigned int)(&loader_main);
	}
	*/
	gdb_monitor_running = 0; // return
	rpi2_debuggee_running = 1;
	//gdb_dyn_debug = 1;
#ifdef GDB_INVALIDATE_CACHES
	if (rpi2_use_mmu)
	{
		rpi2_invalidate_caches();
	}
#endif
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
		gdb_iodev->put_string(resp_buff, len);
		gdb_iodev->put_string("\r\n", 3);
#endif
		gdb_send_packet(resp_buff, len); // two digits per byte
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
		// dump memory as hex into temp buffer
		len = gdb_write_hex_data((uint8_t *)addr, (int)bytes, (char *)gdb_tmp_packet,
				 GDB_MAX_MSG_LEN - 5); // -5 to allow message overhead
#ifdef DEBUG_GDB
		gdb_iodev->put_string("\r\nm_cmd: addr= ", 16);
		util_word_to_hex(scratchpad, addr);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" bytes= ", 10);
		util_word_to_hex(scratchpad, bytes);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" len= ", 10);
		util_word_to_hex(scratchpad, len);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string("\r\n", 3);
#endif
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
#ifdef DEBUG_GDB
		gdb_iodev->put_string("\r\nM_cmd: addr= ", 16);
		util_word_to_hex(scratchpad, addr);
		gdb_iodev->put_string(scratchpad, 9);
		gdb_iodev->put_string(" len= ", 10);
		util_word_to_hex(scratchpad, bytes);
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
	unsigned int value, tmp;
	uint32_t reg;
	uint32_t *p;
	char *cp;
	char *err = "E00";
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (packet_len > 0)
	{
		reg = (uint32_t)util_hex_to_word((char *)gdb_in_packet);
		if (reg < 16)
		{
			//p = (uint32_t *) &(rpi2_reg_context.storage);
			//value = p[reg];
			value = rpi2_reg_context.storage[reg];
			util_swap_bytes(&value, &tmp);
			util_word_to_hex(scratchpad, tmp);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		else if ((reg > 15) && (reg < 25)) // dummy fp-register
		{
			util_word_to_hex(scratchpad, 0x90000009);
			util_swap_bytes(&value, &tmp);
			util_word_to_hex(scratchpad, tmp);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		else if (reg == 25) // cpsr
		{
			//p = (uint32_t *) &(rpi2_reg_context.storage);
			//value = p[reg];
			value = rpi2_reg_context.reg.cpsr;
			util_swap_bytes(&value, &tmp);
			util_word_to_hex(scratchpad, tmp);
			gdb_send_packet(scratchpad, util_str_len(scratchpad));
		}
		else
		{
			gdb_send_packet(err, util_str_len(err));
		}
#ifdef DEBUG_GDB
		gdb_iodev->put_string("\r\np_cmd: ", 10);
		util_word_to_hex(scratchpad, reg);
		gdb_iodev->put_string(scratchpad, 10);
		gdb_iodev->put_string(" : ", 4);
		util_word_to_hex(scratchpad, tmp);
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
	unsigned int value, tmp;
	uint32_t reg;
	uint32_t *p;
	char *err = "E00";
	if (packet_len > 0)
	{
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, '=', scratch_len);
		reg = (uint32_t)util_hex_to_word(scratchpad);
		gdb_in_packet += (len + 1);  // skip register number and '='
		packet_len -= (len + 1);
		tmp = (uint32_t)util_hex_to_word((char *)gdb_in_packet);
		util_swap_bytes(&tmp, &value);
#ifdef DEBUG_GDB
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

void gdb_cmd_kill(volatile uint8_t *gdb_packet, int packet_len)
{
	// kill doesn't have responses
	gdb_reset(1);
}
void gdb_cmd_detach(volatile uint8_t *gdb_packet, int packet_len)
{
	gdb_send_packet("OK", 2); // for now
	gdb_clear_breakpoints(1); // remove all breakpoints
	gdb_cmd_cont("", 0);
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
	int packlen = 0, params = 0;
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
		gdb_iodev->put_string(scratchpad, util_str_len(scratchpad));
		gdb_iodev->put_string("\r\n", 3);
#endif
		packet += len+1; // skip the read and delimiter
		packet_len -= (len+1);
		// len = util_cmp_substr(packet, "qSupported:");
		// if (len == util_str_len("qSupported:"))
		// {
		// 		packet += len;
		if (util_str_cmp(scratchpad, "qSupported") == 0)
		{
#if 1
			while (packet_len > 0)
			{
				len = util_cpy_substr(scratchpad, packet, ';', scratch_len);
				packet += len+1; // the read and delimiter
				packet_len -= (len + 1);
//#ifdef DEBUG_GDB
#if 0
				gdb_iodev->put_string("\r\nparam: ", 12);
				gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
#endif
				// reply: 'stubfeature [;stubfeature]...'
				// 'name=value', 'name+' or 'name-'
#if 1
				if (util_str_cmp(scratchpad, "PacketSize") == 0)
				{
					packlen = 1;
					if (params)
					{
						len = util_append_str(resp_buff + len, ";", resp_buff_len);
					}
					params++;
					len = util_append_str(resp_buff, scratchpad, resp_buff_len);
					len = util_append_str(resp_buff + len, "=", resp_buff_len);
					util_word_to_hex(resp_buff + len, 256);
//					util_word_to_hex(resp_buff + len, GDB_MAX_MSG_LEN);
					len = util_str_len(resp_buff);
				}
				else if (util_str_cmp(scratchpad, "swbreak+") == 0)
				{
					if (params)
					{
						len = util_append_str(resp_buff + len, ";", resp_buff_len);
					}
					params++;
#if 1
					// don't use until the T05-format is clear
					len = util_append_str(resp_buff, "swbreak+", resp_buff_len);
#else
					len = util_append_str(resp_buff, "swbreak-", resp_buff_len);
#endif
					len = util_str_len(resp_buff);
					gdb_swbreak = 1;
				}
				else if (util_str_cmp(scratchpad, "swbreak-") == 0)
				{
					if (params)
					{
						len = util_append_str(resp_buff + len, ";", resp_buff_len);
					}
					params++;
					len = util_append_str(resp_buff, "swbreak-", resp_buff_len);
					len = util_str_len(resp_buff);
					gdb_swbreak = 0;
				}
				else if (util_str_cmp(scratchpad, "hwbreak+") == 0)
				{
					if (rpi2_use_hw_debug)
					{
						if (params)
						{
							len = util_append_str(resp_buff + len, ";", resp_buff_len);
						}
						params++;
#if 0
						// Only use when HW breakpoints are supported
						len = util_append_str(resp_buff, "hwbreak+", resp_buff_len);
#else
						len = util_append_str(resp_buff, "hwbreak-", resp_buff_len);
#endif
						len = util_str_len(resp_buff);
						gdb_hwbreak = 1;
					}
					else
					{
						if (params)
						{
							len = util_append_str(resp_buff + len, ";", resp_buff_len);
						}
						params++;
						len = util_append_str(resp_buff, "hwbreak-", resp_buff_len);
						len = util_str_len(resp_buff);
						gdb_hwbreak = 0;
					}
				}
				else if (util_str_cmp(scratchpad, "hwbreak-") == 0)
				{
					if (params)
					{
						len = util_append_str(resp_buff + len, ";", resp_buff_len);
					}
					params++;
					len = util_append_str(resp_buff, "hwbreak-", resp_buff_len);
					len = util_str_len(resp_buff);
					gdb_hwbreak = 0;
				}
				else
				{
					/* unsupported feature */
					//len = util_append_str(resp_buff, scratchpad, resp_buff_len);
					//len = util_append_str(resp_buff, "-", resp_buff_len);
				}
#endif
#if 0
				if (packet_len > 0)
				{
					len = util_append_str(resp_buff, ";", resp_buff_len);
				}
#endif
			}
			if (packlen == 0) // PacketSize hasn't been given yet
			{
				if (params)
				{
					len = util_append_str(resp_buff + len, ";", resp_buff_len);
				}
				params++;
				len = util_append_str(resp_buff, "PacketSize=", resp_buff_len);
				util_word_to_hex(scratchpad, 256); // GDB_MAX_MSG_LEN / 4
				len = util_append_str(resp_buff, scratchpad, resp_buff_len);
			}
//#ifdef DEBUG_GDB
#if 0
			gdb_iodev->put_string("resp: ", 8);
			gdb_iodev->put_string(resp_buff, util_str_len(resp_buff)+1);
			gdb_iodev->put_string("\r\n", 3);
#endif
#else

			len = util_str_copy(resp_buff, "swbreak+;hwbreak+;PacketSize=", resp_buff_len);
			util_word_to_hex(scratchpad, 256); // GDB_MAX_MSG_LEN / 4
			len = util_append_str(resp_buff, scratchpad, resp_buff_len);
#endif
			gdb_send_packet(resp_buff, len);
		}
		else if (util_str_cmp(scratchpad, "qOffsets") == 0)
		{
			// reply: 'Text=xxxx;Data=xxxx;Bss=xxxx'
			len = util_str_copy(resp_buff, "Text=0;Data=0;Bss=0", resp_buff_len);
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
	gdb_response_not_supported(); // for now
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
			LOG_PR_STR("Target address reached\r\n");
			//len = util_str_copy(resp_buff, "OTarget address reached", resp_buff_len);
			//gdb_send_packet((char *)resp_buff, len);
			gdb_send_packet(okresp, util_str_len(okresp)); // response
			gdb_monitor_running = 1;
			return;
		}
	}
	// step
	next_addr = next_address(curr_addr);
	if (next_addr.flag & (~INSTR_ADDR_UNPRED) == INSTR_ADDR_UNDEF)
	{
		LOG_PR_VAL("Next instr undef: ", *((uint32_t *)next_addr.address));
		LOG_PR_VAL_CONT(" addr: ", next_addr.address);
		LOG_NEWLINE();
		// stop single stepping
		gdb_single_stepping = 0; // just to be sure
		gdb_single_stepping_address = 0xffffffff;
		// send response
		//len = util_str_copy(resp_buff, "ONext instruction is UNDEFINED", resp_buff_len);
		//gdb_send_packet((char *)resp_buff, len); // response
		gdb_send_packet(err, util_str_len(err));
		gdb_monitor_running = 1;
		return;
	}
	else if (next_addr.flag & INSTR_ADDR_UNPRED)
	{
		LOG_PR_VAL("Next instr unpred: ", *((uint32_t *)next_addr.address));
		LOG_PR_VAL_CONT(" addr: ", next_addr.address);
		LOG_NEWLINE();
		// TODO: configuration - allow unpredictables
		// stop single stepping
		gdb_single_stepping = 0; // just to be sure
		gdb_single_stepping_address = 0xffffffff;
		// send response
		//len = util_str_copy(resp_buff, "ONext instruction is UNPREDICTABLE", resp_buff_len);
		//gdb_send_packet((char *)resp_buff, len);
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
	LOG_PR_VAL("Next address: ", next_addr.address);
	LOG_NEWLINE();
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
			LOG_PR_STR("Already single stepping\r\n");
			//len = util_str_copy(resp_buff, "OAlready single stepping", resp_buff_len);
			//gdb_send_packet((char *)resp_buff, len);
			gdb_send_packet(err, util_str_len(err));
			return;
		}
	}
	else
	{
		gdb_single_stepping_address = 0xffffffff; // just one single step
	}
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
		gdb_in_packet += len+1; // skip byte count and delimiter
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

void gdb_cmd_special(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len];
	uint32_t tmp;
	if (packet_len > 1)
	{
		switch (*gdb_in_packet)
		{
		case '0':
			// Y0:n
			len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ':', scratch_len);
			gdb_in_packet += len+1; // skip address and delimiter
			tmp = util_hex_to_word((char *)gdb_in_packet);
			gdb_dyn_debug = tmp;
			break;
		default:
			gdb_response_not_supported();
			break;
		}
	}
	else
	{
		gdb_response_not_supported();
	}
}

// Z1 - Z4 = HW breakpoints/watchpoints

void gdb_cmd_add_watchpoint(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	int result;
	uint32_t addr;
	uint32_t bytes;
	uint32_t type;
	uint8_t tmp;
	char *okresp = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad

	switch (*gdb_in_packet)
	{
	case '2': // write
		type = 2;
		break;
	case '3': // read
		type = 1;
		break;
	case '4': // read-write
		type = 3;
		break;
	default:
		return;
		break;
	}
	gdb_in_packet += 2; // skip number and ','
	packet_len -= 2;
	len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
	gdb_in_packet += len+1; // skip address and delimiter
	addr = util_hex_to_word(scratchpad);
	bytes = util_hex_to_word((char *)gdb_in_packet);

	result = dgb_add_watchpoint(type, addr, bytes);
	if (result < 0)
	{
		tmp = (uint8_t)(-result);
		scratchpad[0] = 'E';
		util_byte_to_hex(scratchpad + 1, (unsigned char) tmp);
		gdb_send_packet(scratchpad, util_str_len(scratchpad));
	}
	else
	{
		gdb_send_packet(okresp, util_str_len(okresp));
	}
}

void gdb_cmd_del_watchpoint(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	int result;
	uint32_t addr;
	uint32_t bytes;
	uint32_t type;
	uint8_t tmp;
	char *okresp = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad

	switch (*gdb_in_packet)
	{
	case '2': // write
		type = 2;
		break;
	case '3': // read
		type = 1;
		break;
	case '4': // read-write
		type = 3;
		break;
	default:
		return;
		break;
	}
	gdb_in_packet += 2; // skip number and ','
	packet_len -= 2;
	len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
	gdb_in_packet += len+1; // skip address and delimiter
	addr = util_hex_to_word(scratchpad);
	bytes = util_hex_to_word((char *)gdb_in_packet);

	result = dgb_del_watchpoint(type, addr, bytes);
	if (result < 0)
	{
		tmp = (uint8_t)(-result);
		scratchpad[0] = 'E';
		util_byte_to_hex(scratchpad + 1, (unsigned char) tmp);
		gdb_send_packet(scratchpad, util_str_len(scratchpad));
	}
	else
	{
		gdb_send_packet(okresp, util_str_len(okresp));
	}

}

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
	kind = util_hex_to_word((char *)gdb_in_packet);
	//rpi2_unset_watchpoint(0);
	if (kind == 2) // 16-bit THUMB
	{
		if (dgb_set_trap((void *)addr, RPI2_TRAP_THUMB))
		{
			gdb_send_packet(err, util_str_len(err));
		}
		else
		{
			gdb_send_packet(okresp, util_str_len(okresp));
		}
	}
	else if (kind == 4) // 32-bit ARM
	{
		if (dgb_set_trap((void *)addr, RPI2_TRAP_ARM))
		{
			gdb_send_packet(err, util_str_len(err));
		}
		else
		{
			gdb_send_packet(okresp, util_str_len(okresp));
		}
	}
	else
	{
		gdb_response_not_supported();
	}
	//rpi2_set_watchpoint(0, (unsigned int)(&gdb_num_bkpts), 4);
}

// z0,addr,kind
void gdb_cmd_delete_breakpoint(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	uint32_t addr;
	uint32_t kind;
	uint32_t errval;
	char *err;
	char *okresp = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (*gdb_in_packet == ',') gdb_in_packet++; // skip ',' after z0
	// get addr
	len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
	gdb_in_packet += len+1; // skip address and delimiter
	addr = util_hex_to_word(scratchpad);
	kind = util_hex_to_word((char *)gdb_in_packet);
	//rpi2_unset_watchpoint(0);
	if (kind == 2) // 16-bit THUMB
	{
		if ((errval = dgb_unset_trap((void *)addr, RPI2_TRAP_THUMB)) != 0)
		{
			if (errval == 1) err = "E01";
			else if (errval == 2) err = "E02";
			else if (errval == 3) err = "E03";
			else err = "E04";
			gdb_send_packet(err, util_str_len(err));
		}
		else
		{
			gdb_send_packet(okresp, util_str_len(okresp));
		}
	}
	else if (kind == 4) // 32-bit ARM
	{
		if ((errval = dgb_unset_trap((void *)addr, RPI2_TRAP_ARM)) != 0)
		{
			if (errval == 1) err = "E01";
			else if (errval == 2) err = "E02";
			else if (errval == 3) err = "E03";
			else err = "E04";
			gdb_send_packet(err, util_str_len(err));
		}
		else
		{
			gdb_send_packet(okresp, util_str_len(okresp));
		}
	}
	else
	{
		gdb_response_not_supported();
	}
	//rpi2_set_watchpoint(0, (unsigned int)(&gdb_num_bkpts), 4);
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
	}
	rpi2_flush_address((unsigned int)(bkpt->trap_address));
	asm volatile("dsb\n\tisb\n\t");

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
				//gdb_monitor_running = 0; // resume kind of continues
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
#ifdef GDB_DEBUG_LED
	uint32_t xcpsr;
#endif

#ifdef DEBUG_GDB
	gdb_dyn_debug = 1;

	msg = "\r\n\r\nEntering GDB-monitor\r\nreason = ";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
	util_word_to_hex(scratchpad, reason);
	scratchpad[8]='\0'; // end-nul
	gdb_iodev->put_string(scratchpad, util_str_len(scratchpad)+1);
	msg = "\r\n";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
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
	// or commands
	if (gdb_monitor_running)
	{
		gdb_iodev->disable_ctrlc(); // disable
		rpi2_debuggee_running = 0;
		SYNC;
	}
#ifdef DEBUG_GDB
	msg = "crtl-c handler set\r\n";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

	while(gdb_monitor_running)
	{
#ifdef RPI2_DEBUG_TIMER
		rpi2_timer_kick();
#endif

#ifdef DEBUG_GDB
		msg = "monitor-loop\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif
#ifdef GDB_DEBUG_LED
		asm volatile
		(
				"mrs %[var_reg], cpsr\n\t"
				"dsb\n\t"
				:[var_reg] "=r" (xcpsr) ::
		);
#if 0
		if (xcpsr & (1<<7))
		{
			// irq masked
			serial_raw_puts("\r\ncpsr: ");
			util_word_to_hex(scratchpad, xcpsr);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\n");
			rpi2_led_blink(500, 500, 5);
			asm volatile("cpsie i\n\t");
		}
#endif
#if 0
		if (xcpsr & (1<<6))
		{
			// fiq masked
			serial_raw_puts("\r\ncpsr: ");
			util_word_to_hex(scratchpad, xcpsr);
			serial_raw_puts(scratchpad);
			serial_raw_puts("\r\n");
			rpi2_led_blink(500, 500, 5);
			asm volatile("cpsie f\n\t");
		}
#endif

		rpi2_led_on();
#endif

#if 0
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
#endif
		packet_len = receive_packet(inpkg);
#ifdef DEBUG_GDB
		msg = "packet received\r\n";
		gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif

#if 0
		if (gdb_dyn_debug == 1)
		{
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
		}
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
					gdb_out_packet[0] = '\0';
					continue; // for now
				}
				// NACK received
				if (*ch == '-')
				{
#ifdef DEBUG_GDB
					gdb_iodev->put_string("\r\ngot nack\r\n", 13);
#else
					packet_len = util_str_len((char *)gdb_out_packet);
					gdb_iodev->write((char *)gdb_out_packet, packet_len);
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
				gdb_cmd_kill(++inpkg, --packet_len);
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
				//gdb_response_not_supported(); // for now
				break;
			case 'x':	// read mem binary
				gdb_cmd_read_mem_bin(++inpkg, --packet_len);
				break;
			case 'Y':	// specials
				gdb_cmd_special(++inpkg, --packet_len);
				break;
			case 'Z':	// add Z0-breakpoint +
				// Z1 - Z4 = HW breakpoints/watchpoints
				switch (*(++inpkg))
				{
				case '0': // SW breakpoint
					packet_len--;
					gdb_cmd_add_breakpoint(++inpkg, --packet_len);
					break;
				case '1': // HW breakpoint
					gdb_response_not_supported();
					break;
				case '2':
				case '3':
				case '4':
					if (rpi2_use_hw_debug)
					{
						packet_len--;
						gdb_cmd_add_watchpoint(inpkg, packet_len);
					}
					else
					{
						gdb_response_not_supported();
					}
					break;
				default:
					gdb_response_not_supported();
					break;
				}
				break;
			case 'z':	// remove Z0-breakpoint +
				// z1 - z4 = HW breakpoints/watchpoints
				switch (*(++inpkg))
				{
				case '0': // SW breakpoint
					packet_len--;
					gdb_cmd_delete_breakpoint(++inpkg, --packet_len);
					break;
				case '1': // HW breakpoint
					gdb_response_not_supported();
					break;
				case '2':
				case '3':
				case '4':
					if (rpi2_use_hw_debug)
					{
						packet_len--;
						gdb_cmd_del_watchpoint(inpkg, packet_len);
					}
					else
					{
						gdb_response_not_supported();
					}
					break;
				default:
					gdb_response_not_supported();
					break;
				}
				break;
			default:
				gdb_response_not_supported();
				break; // needless, but some tools want this
			}
		}
		SYNC;
	}
	// enable CTRL-C
	gdb_iodev->enable_ctrlc(); // enable

	//ch = "\r\nLeaving GDB-monitor\r\n";
	//gdb_send_text_packet(ch, util_str_len(ch));
#ifdef DEBUG_GDB
	msg = "\r\ngdb_monitor returns\r\n";
	gdb_iodev->put_string(msg, util_str_len(msg)+1);
#endif
#ifdef GDB_DEBUG_LED
	rpi2_led_off();
#endif
	return;
}
