/*
 * gdb.c
 *
 *  Created on: Mar 28, 2015
 *      Author: jaa
 */

#include <stdint.h>
#include "serial.h"
#include "rpi2.h"
#include "util.h"
#include "gdb.h"

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

// program
typedef struct {
	void *start;
	uint32_t size; // load size
	void *entry;
	void *curr_addr;
	uint8_t status;
} gdb_program_rec;

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

// breakpoints for single-stepping - both ways of conditional branches
volatile gdb_trap_rec gdb_step_bkpt[2];
volatile int gdb_single_stepping;

// program to be debugged
volatile gdb_program_rec gdb_debuggee;

// device for communicating with gdb client
volatile io_device *gdb_iodev;

// gdb I/O packet buffers
static volatile uint8_t gdb_in_packet[GDB_MAX_MSG_LEN]; // packets from gdb
static volatile uint8_t gdb_out_packet[GDB_MAX_MSG_LEN]; // packets to gdb
static volatile uint8_t gdb_tmp_packet[GDB_MAX_MSG_LEN]; // for building packets
static volatile int gdb_monitor_running = 0;

// The 'command interpreter'
void gdb_monitor(int reason);

// ckeck if cause of exception was a breakpoint
// return breakpoint number, or -1 if none
int gdb_check_breakpoint()
{
	uint32_t pc;
	int num = -1;
	int i;

	pc = rpi2_reg_context.reg.r15;
	// single stepping 1?
	if (gdb_step_bkpt[0].trap_address == (void *)pc)
	{
		if (gdb_step_bkpt[0].valid)
		{
			num = GDB_MAX_BREAKPOINTS;
		}
	}
	// single stepping 2?
	else if (gdb_step_bkpt[1].trap_address == (void *)pc)
	{
		if (gdb_step_bkpt[0].valid)
		{
			num = GDB_MAX_BREAKPOINTS + 1;
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
	int trap_num;
	reason = exception_info;
#if 0
	// currently done in rpi2.c
	// IRQs need to be enabled for serial I/O
	uint32_t irqmask = ~(1<<7);
	asm volatile (
			"push r0\n\t"
			"mrs.w r0, cpsr\n\t"
			"and r0, irqmask"
			"msr.w r0, cpsr\n\t"
			"pop r0\n\t"
	);
#endif
	// check if breakpoint or actual pabt
	if (serial_get_ctrl_c() == 1)
	{
		// ctrl-C pressed
		reason = SIG_INT;
		// flush input - read until ctrl-c
		while (gdb_iodev->get_char() != 3);
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
			if ((exception_extra == 1) || (exception_extra == 2))
			{
				trap_num = gdb_check_breakpoint();
				if (trap_num == 0)
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
	/* initialize rpi2 */
	rpi2_init();
	/* install trap handler */
	rpi2_set_vector(RPI2_EXC_TRAP, &gdb_trap_handler);
	/* install ctrl-c handling */
	gdb_iodev->set_ctrlc((void *)rpi2_pend_trap);
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
	gdb_step_bkpt[0].instruction.arm = 0;
	gdb_step_bkpt[0].trap_address = (void *)0xffffffff;
	gdb_step_bkpt[0].valid = 0;
	gdb_step_bkpt[1].instruction.arm = 0;
	gdb_step_bkpt[1].trap_address = (void *)0xffffffff;
	gdb_step_bkpt[1].valid = 0;
	gdb_single_stepping = 0;
	// reset debuggee info
	gdb_debuggee.start = (void *)0xffffffff;
	gdb_debuggee.size = 0;
	gdb_debuggee.entry = (void *)0xffffffff;
	gdb_debuggee.curr_addr = (void *)0xffffffff;
	gdb_debuggee.status = 0;
	return;
}

/* set a breakpoint to given address */
/* returns -1 if trap can't be set */
int dgb_set_trap(void *address, int kind)
{
	int i;
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
			return 0; // success
		}
	}
	return -1; // failure - something wrong with the table
}

int dgb_unset_trap(void *address, int kind)
{
	int i;
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

int is_jmp_thumb(uint16_t instr)
{
	return 0;
}

int is_cbranch_thumb(uint16_t instr)
{
	return 0;
}

int is_jmp_arm(uint32_t instr)
{
	return 0;
}

int is_cbranch_arm(uint32_t instr)
{
	return 0;
}

void gdb_resume(int bkptnum)
{
	uint32_t *addr_a; // ARM
	uint16_t *addr_t; // THUMB
	int i;
	if (bkptnum > 0)
	{
		if (bkptnum < GDB_MAX_BREAKPOINTS) // user bkpt
		{
			// restore instruction
			// put breakpoint to next instruction
			// run old instr
			// re-apply old breakpoint
			// restore new instruction
		}
		else // single step
		{
			// restore instruction(s)
			for (i=0; i<2; i++)
			{
				if (gdb_step_bkpt[i].valid)
				{
					if (gdb_step_bkpt[i].trap_kind == RPI2_TRAP_THUMB)
					{
						addr_t = (uint16_t *) gdb_step_bkpt[i].trap_address;
						*addr_t = gdb_step_bkpt[i].instruction.thumb;
					}
					else
					{
						addr_a = (uint32_t *) gdb_step_bkpt[i].trap_address;
						*addr_a = gdb_step_bkpt[i].instruction.arm;
					}
					gdb_step_bkpt[i].trap_kind = 0;
					gdb_step_bkpt[i].valid = 0;
				}
			}
			// ARM or THUMB mode?
			if (rpi2_reg_context.reg.spsr & 0x20)
			{
				// THUMB mode
				addr_t = (uint16_t *) gdb_step_bkpt[i].trap_address;
				addr_t++; // next instruction
				if (is_jmp_thumb(gdb_step_bkpt[0].instruction.thumb))
				{
					// jump - find target address
				}
				else if (is_cbranch_thumb(gdb_step_bkpt[0].instruction.thumb))
				{
					// two possible branches find branch target address
				}
				else
				{
					// linear execution - next instruction

				}
			}
			// put breakpoint to next instruction(s)
			// return
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
	while (i < count)
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
		if (j + 2 > max) break; // out of while
		len = util_byte_to_bin(dst, *(src++));
		bindata += len;
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
		if (i + 2 > count) break;
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
	uint8_t inbyte;
	unsigned char *src = (unsigned char *)bindata;
	unsigned char *dst = (unsigned char *)outdata;
	while (j < max)
	{
		if (i + 2 > count) break;
		inbyte = *(src++);
		if (inbyte == 0x7d) // escaped
		{
			i++; // escape char
			inbyte = *(src++) ^ 0x20; // escaped char
		}
		*(dst++) = inbyte;
		j++;
		i++;
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
	int len = 0;
	int ch;
	int tmp;
	int checksum;
	int chksum;
	while (gdb_iodev->get_char() != (int)'$'); // Wait for '$'
	// receive payload
	while ((ch = gdb_iodev->get_char()) != (int)'#')
	{
		// break-character received
		if (ch == 3)
		{
			len = -3;
			break;
		}
		// calculate checksum
		checksum += (unsigned char) ch;
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
	// if no errors this far, handle checksum
	if (len > 0)
	{
		// get first checksum hex digit and check for BRK
		// the '#' gets dropped
		if ((ch = gdb_iodev->get_char()) == 3)
		{
			return -3; // BRK
		}
		// upper checksum hex digit to binary
		chksum = util_hex_to_nib((char) ch);
		if (chksum < 0)
		{
			return -4; // not hex digit
		}

		// get second checksum hex digit and check for BRK
		if ((ch = gdb_iodev->get_char()) == 3)
		{
			return -3; // BRK
		}

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
	gdb_iodev->put_string("$+#2b", 6);
}

void gdb_packet_nack()
{
	gdb_iodev->put_string("$-#2d", 6);
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
	const int scratch_len = 16;
	const int resp_buff_len = 128; // should be enough to hold any response
	char scratchpad[scratch_len]; // scratchpad
	char resp_buff[resp_buff_len]; // response buffer

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
		len = util_append_str(resp_buff, "0F", resp_buff_len);
		// put PC-value into message
		util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_ILL: // undef
		// T04 - undef response
		len = util_str_copy(resp_buff, "T04", resp_buff_len);
		// PC value given
		len = util_append_str(resp_buff, "0F", resp_buff_len);
		// put PC-value into message
		util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_BUS: // pabt/dabt
		// T07 - pabt/dabt response
		len = util_str_copy(resp_buff, "T07", resp_buff_len);
		// PC value given
		len = util_append_str(resp_buff, "0F", resp_buff_len);
		// put PC-value into message
		util_word_to_hex(scratchpad, rpi2_reg_context.reg.r15);
		len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	case SIG_USR1: // unhandled HW interrupt - no defined response
		// send 'OUnhandled HW interrupt'
		len = util_str_copy(resp_buff, "OUnhandled HW interrupt", resp_buff_len);
		break;
	case SIG_USR2: // unhandled SW interrupt - no defined response
		// send 'OUnhandled SW interrupt'
		len = util_str_copy(resp_buff, "OUnhandled SW interrupt", resp_buff_len);
		break;
	case ALOHA: // no debuggee loaded yet - no defined response
		// send 'Ogdb stub started'
		len = util_str_copy(resp_buff, "Ogdb stub started\n", resp_buff_len);
		break;
	case FINISHED: // program has finished
		// send 'W<exit status>' response
		len = util_str_copy(resp_buff, "W", resp_buff_len);
		// put exit status value into message
		util_byte_to_hex(scratchpad, gdb_debuggee.status);
		len = util_append_str(resp_buff, scratchpad, resp_buff_len);
		break;
	default:
		// send 'Ounknown event'
		len = util_str_copy(resp_buff, "OUnknown event\n", resp_buff_len);
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
		gdb_resume(bkptnum);
	}
	gdb_monitor_running = 0; // return

	// is 'response pending' needed?
}

// g
void gdb_cmd_get_regs(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	const int resp_buff_len = 128; // should be enough to hold 'g' response
	char resp_buff[resp_buff_len]; // response buff
	if (packet_len == 1)
	{
		// response for 'g'
		len = gdb_write_hex_data((char *)&(rpi2_reg_context.storage),
				sizeof(rpi2_reg_context), resp_buff, resp_buff_len);
		gdb_send_packet(resp_buff, len);
	}
}

// G
void gdb_cmd_set_regs(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	char *resp_str = "OK";
	const int resp_buff_len = 128; // should be enough to hold 'g' response
	char resp_buff[resp_buff_len]; // response buff
	if (packet_len > 1)
	{
		gdb_read_hex_data((char *)gdb_in_packet, packet_len - 1,
				(char *)&(rpi2_reg_context.storage), sizeof(rpi2_reg_context));
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
	if (packet_len > 1)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		addr = util_hex_to_word(scratchpad); // address to binary
		bytes = util_hex_to_word((char *)gdb_in_packet); // read nuber of bytes
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
	if (packet_len > 1)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		addr = util_hex_to_word(scratchpad); // address to binary
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ':', scratch_len);
		gdb_in_packet += len+1; // skip bytecount and delimiter
		bytes = util_hex_to_word(scratchpad); // address to binary
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
	uint32_t value;
	uint8_t reg;
	uint32_t *p;
	char *err = "E00";
	int len;
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	if (packet_len > 1)
	{
		reg = util_hex_to_byte((char *)gdb_in_packet);
		if (reg < 17)
		{
			p = (uint32_t *) &(rpi2_reg_context.storage);
			value = p[reg];
			util_word_to_hex(scratchpad, value);
			gdb_send_packet(scratchpad, 4);
		}
		else
		{
			gdb_send_packet(err, util_str_len(err));
		}
	}
}

// P n...=r...
void gdb_cmd_write_reg(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	char *resp_str = "OK";
	const int scratch_len = 16;
	char scratchpad[scratch_len]; // scratchpad
	uint32_t value;
	uint8_t reg;
	uint32_t *p;
	char *err = "E00";
	if (packet_len > 1)
	{
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, '=', scratch_len);
		reg = util_hex_to_byte(scratchpad);
		if (reg < 17)
		{
			gdb_in_packet += len; // skip reg number
			value = util_hex_to_word((char *)gdb_in_packet + 1); // skip '='
			p = (uint32_t *) &(rpi2_reg_context.storage);
			p[reg] = value;
		}
		else
		{
			gdb_send_packet(err, util_str_len(err));
			return;
		}
	}
	gdb_send_packet(resp_str, util_str_len(resp_str));
}

// q
void gdb_cmd_common_query(volatile uint8_t *gdb_in_packet, int packet_len)
{
	int len;
	const int scratch_len = 16;
	const int resp_buff_len = 128; // should be enough to hold any response
	char scratchpad[scratch_len]; // scratchpad
	char resp_buff[resp_buff_len]; // response buffer
	// q name params
	// qSupported [:gdbfeature [;gdbfeature]... ]
	// PacketSize=bytes  value required
	if (packet_len > 1)
	{
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ':', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		if (util_str_cmp(scratchpad, "Supported") == 0)
		{
			// reply: 'stubfeature [;stubfeature]...'
			// 'name=value', 'name+' or 'name-'
			len = util_str_copy(resp_buff, "PacketSize=", resp_buff_len);
			util_word_to_hex(resp_buff + len, GDB_MAX_MSG_LEN);
			gdb_send_packet(resp_buff, len + 8);
		}
		if (util_str_cmp(scratchpad, "Offset") == 0)
		{
			// reply: 'Text=xxxx;Data=xxxx;Bss=xxxx'
			len = util_str_copy(resp_buff, "Text=8000;Data=8000;Bss=8000", resp_buff_len);
			util_word_to_hex(resp_buff + len, GDB_MAX_MSG_LEN);
			gdb_send_packet(resp_buff, len + 8);
		}

		gdb_response_not_supported(); // for now
		return;
	}
}

// R XX
void gdb_cmd_restart_program(volatile uint8_t *gdb_in_packet, int packet_len)
{

}

// s [addr]
void gdb_cmd_single_step(volatile uint8_t *gdb_in_packet, int packet_len)
{
	// gdb_single_stepping = 1;
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
	// M addr,length:XX XX ...
	if (packet_len > 1)
	{
		// get hex for address
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ',', scratch_len);
		gdb_in_packet += len+1; // skip address and delimiter
		addr = util_hex_to_word(scratchpad); // address to binary
		len = util_cpy_substr(scratchpad, (char *)gdb_in_packet, ':', scratch_len);
		gdb_in_packet += len+1; // skip bytecount and delimiter
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
	addr = util_hex_to_byte(scratchpad);
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
	addr = util_hex_to_byte(scratchpad);
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

// handle stuff left pending until exception
void gdb_handle_pending_state(int reason)
{
	// single step?

	// stop responses - responses to commands 'c', 's', and the like
	gdb_resp_target_halted(reason);
	// go to monitor
}

/* The main debugger loop */
void gdb_monitor(int reason)
{
	int packet_len;
	char *ch;
	char *inpkg = (char *)gdb_in_packet;
	gdb_handle_pending_state(reason);

	gdb_monitor_running = 1;
	while(gdb_monitor_running)
	{
		packet_len = receive_packet(inpkg);
		if (packet_len < 0)
		{
			// BRK?
			if (packet_len == -3)
			{
				// send ACK
				gdb_packet_ack();
				// clear BRK
				gdb_iodev->get_ctrl_c();
				// flush input - read until ctrl-c
				while (gdb_iodev->get_char() != 3);
				continue;
			}
			else // corrupted packer
			{
				// send NACK
				gdb_packet_nack();
				// flush, re-read
				packet_len = 0;
				continue;
			}
		}
		else
		{
			if (packet_len == 0)
			{
				// dummy
				continue;
			}

			// Handle packet ch points to beginning of buffer
			ch = inpkg;

			if (packet_len == 1)
			{
				// ACK received
				if (*ch == '+')
				{
					// allow new message to be sent
					continue; // for now
				}
				// NACK received
				if (*ch == '-')
				{
					// resend
					continue; // for now
				}
			}

			// unbroken packet received
			gdb_packet_ack();

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
				gdb_cmd_cont(++inpkg, packet_len);
				break;
			case 'D':	// detach
				gdb_response_not_supported(); // for now
				break;
			case 'F':	// file I/O - for gdb console
				gdb_response_not_supported(); // for now
				break;
			case 'g':	// get regs +
				gdb_cmd_get_regs(++inpkg, packet_len);
				break;
			case 'G':	// set regs +
				gdb_cmd_set_regs(++inpkg, packet_len);
				break;
			case 'H':	// set thread - dummy
				gdb_response_not_supported(); // for now
				break;
			case 'k':	// kill
				gdb_response_not_supported(); // for now
				break;
			case 'm':	// read mem hex +
				gdb_cmd_read_mem_hex(++inpkg, packet_len);
				break;
			case 'M':	// write mem hex +
				gdb_cmd_write_mem_hex(++inpkg, packet_len);
				break;
			case 'p':	// read reg +
				gdb_cmd_read_reg(++inpkg, packet_len);
				break;
			case 'P':	// write reg +
				gdb_cmd_write_reg(++inpkg, packet_len);
				break;
			case 'q':	// query + (support)
				gdb_cmd_common_query(++inpkg, packet_len);
				break;
			case 'Q':	// set
				gdb_response_not_supported(); // for now
				break;
			case 'R':	// restart program
				gdb_cmd_restart_program(inpkg, packet_len);
				break;
			case 's':	// single step +
				gdb_cmd_single_step(++inpkg, packet_len);
				break;
			case 'T':	// thread alive?
				gdb_response_not_supported(); // for now
				break;
			case 'v':	// v-command
				gdb_response_not_supported(); // for now
				break;
			case 'X':	// write mem binary +
				gdb_cmd_write_mem_bin(++inpkg, packet_len);
				break;
			case 'x':	// read mem binary +
				gdb_cmd_read_mem_bin(++inpkg, packet_len);
				break;
			case 'Z':	// add Z0-breakpoint +
				// Z1 - Z4 = HW breakpoints/watchpoints
				if (*(++inpkg) == '0') // if Z0
				{
					gdb_cmd_add_breakpoint(++inpkg, packet_len);
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
					gdb_cmd_delete_breakpoint(++inpkg, packet_len);
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
	return;
}
