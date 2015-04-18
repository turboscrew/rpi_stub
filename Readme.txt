This is my very first RPi(2) project ever, and first ever project that
I have to deal with ARM core (and assembly).
This project started in the first days of april 2015.

This will hopefully become a standalone gdb-stub for RPi2 using serial line
for communicating with a gdb client.

The idea is that this loader/debugger boots, and it could be used for uploading
and then debugging bare metal progams on RPi2. The only 'external' h-file used
is stdint.h.

I don't take any responsibility of anything that happens, if someone dares to
try this. You can use this but totally on your own risk.
I don't have any means of debugging, so I have to write this code "blindfolded".
I may fix this and that and even expand this, but I give no promises to maintain
this in any way.

Hopefully this is useful reading material for those that want to learn about
gdb-stubs.

missing:
breakpoints and single-stepping


--------------------------
Some kind of description

This is a rough 'conceptual' pseudocode of the program
Here asynchronous/independent code sequences are called processes
PROCESS loader
    set up exceptions
    set up serial
    set up gdb-stub
    reset gdb-stub
    FOREVER
        BKPT
        IF program loaded THEN // loaded by gdb-stub
            IF arguments THEN
                CALL program with arguments (probably doesn't return)
                notify program status (if program returns)
                reset gdb-stub
            ELSE
                CALL program with NULL arguments (probably doesn't return)
                notify program status (if program returns)
                reset gdb-stub
            ENDIF
        ENDIF
    ENDFOREVER
ENDPROCESS

PROCESS exception_handler (really kind of exception vector)
    store registers
    IF exception is reset THEN
        START reset
    ELSIF exception is serial tx THEN
        START serial_output
    ELSIF exception is serial rx THEN
        START serial_output
    ELSE
        START gdb-stub
    ENDIF
ENDPROCESS

PROCESS reset
    move program to upper memory
    START loader
ENDPROCESS

PROCESS serial_output
    IF data to send in queue THEN
        send data from queue
    ENDIF
    restore registers
    return from exception
ENDPROCESS

PROCESS serial_input
    IF data to receive THEN
        receive data to queue
        IF ctrl-C encountered THEN
            notify gdb_stub about ctrl-C
            set exception pending for gdb-stub
        ENDIF
    ENDIF
    restore registers
    return from exception
ENDPROCESS

PROCESS gdb-stub
    handle debug exception (all except reset, serial tx and serial rx)
    handle pending status (like remove single-stepping breakpoints)
    send pending response to gdb-client (response for s,c,...)
    set stub_running to TRUE
    WHILE stub_running DO
        CALL gdb-stub command_interpreter
        // to resume, set stub_running to FALSE and return from call
    ENDWHILE
    restore registers
    return from exception
ENDPROCESS


single-stepping
in command interpreter s-command checks the next instruction
if instruction is branch, set breakpoint to branch target
if instruction is cónditiomal branch, set bkpt to branch target and next instruction
else set set bkpt to next instruction
set stub_running to FALSE and return from command interpreter
(that's resume)
When the debuggee hits a single-step bkpt, it causes exception,
removal of single-step breakpoint(s), reporting 's'-command response and
entering back to command interpreter

What if single-stepping breakpoint address already contains 'Z'-command
breakpoint? 

Seems like the dbg client handles it.
infrun.c, void proceed (CORE_ADDR addr, enum gdb_signal siggnal, int step):

  /* If we need to step over a breakpoint, and we're not using
     displaced stepping to do so, insert all breakpoints (watchpoints,
     etc.) but the one we're stepping over, step one instruction, and
     then re-insert the breakpoint when that step is finished.  */
 

breakpoint handling
In command interpreter Z-command sets breakpoint to the target address
and returns to the command interpreter
When program hits the breakpoint, it causes exception and the execution ends up
in the command interpreter through reporting 'T'-response (for 'c' or what ever)
to gdb client. The breakpoint state is left pending for resume to take care of.

From 'Internals':
"When the user says to continue, GDB will restore the original instruction,
single-step, re-insert the trap, and continue on."

That is: Bkpt is replaced with the original instruction and an 's'-command is
internally executed. When a single-step breakpoint is hit, the single-step
breakpoint(s) are removed and the breakpoint is re-installed to the place defined
by the earlier 'Z'-command.


------------------------
Notes and reminders

single-step
- marker: we need to know if we were single-stepping when we hit breakpoint.

- we should take into account all branching instructions
  both ARM and THUMB
  
- Also jumps and loading PC should be treated as branches

Instruction classification for single stepping


 qOffsets. Report the offsets to use when relocating downloaded code.

 qSupported. Report the features supported by the RSP server.
   As a minimum, just the packet size can be reported.

 A RSP server supporting standard remote debugging (i.e. using the GDB target
 remote command) should implement at least the following RSP packets in
 addition to those required for standard remote debugging:

    !. Advise the target that extended remote debugging is being used.

    R. Restart the program being run.

    vAttach. Attach to a new process with a specified process ID.
      This packet need not be implemented if the target has no concept of
      a process ID, but should return an error code.

    vRun. Specify a new program and arguments to run. A minimal implementation
      may restrict this to the case where only the current program may be run again. 
    
 In the RSP, the s packet indicates stepping of a single machine instruction,
 not a high level statement. In this way it maps to GDB's stepi command, not its
 step command (which confusingly can be abbreviated to just s). 
    

------
0xef9f0001 = SWI/SVC 9f0001
0xe7f001f0 = Architecturally undefined

/* Correct in either endianness.  */
static const unsigned long arm_breakpoint = 0xef9f0001;
#define arm_breakpoint_len 4
static const unsigned short thumb_breakpoint = 0xde01;
static const unsigned short thumb2_breakpoint[] = { 0xf7f0, 0xa000 };

/* For new EABI binaries.  We recognize it regardless of which ABI
   is used for gdbserver, so single threaded debugging should work
   OK, but for multi-threaded debugging we only insert the current
   ABI's breakpoint instruction.  For now at least.  */
static const unsigned long arm_eabi_breakpoint = 0xe7f001f0;

--------

instructions

BX = branch and change ARM/THUMB mode

ARM
Each ARM instruction is a single 32-bit word
 
branches
conditional branches

THUMB
branches
conditional branches

--------

example:

static int
arm_breakpoint_at (CORE_ADDR where)
{
  struct regcache *regcache = get_thread_regcache (current_thread, 1);
  unsigned long cpsr;

  collect_register_by_name (regcache, "cpsr", &cpsr);

  if (cpsr & 0x20)
    {
      /* Thumb mode.  */
      unsigned short insn;

      (*the_target->read_memory) (where, (unsigned char *) &insn, 2);
      if (insn == thumb_breakpoint)
	return 1;

      if (insn == thumb2_breakpoint[0])
	{
	  (*the_target->read_memory) (where + 2, (unsigned char *) &insn, 2);
	  if (insn == thumb2_breakpoint[1])
	    return 1;
	}
    }
  else
    {
      /* ARM mode.  */
      unsigned long insn;

      (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
      if (insn == arm_breakpoint)
	return 1;

      if (insn == arm_eabi_breakpoint)
	return 1;
    }

  return 0;
}


