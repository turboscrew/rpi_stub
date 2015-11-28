Copyright (C) 2015 Juha Aaltonen


STANDALONE GDB STUB
for Raspberry Pi 2B



WORK IN PROGRESS!

The rpi_stub is a bare metal standalone gdb server/stub to aid in remote debugging
of bare metal programs using gdb via serial cable. It boots from the SD-card and
runs there without any additional programs.
Firmware (boot files) of Oct 11 2015 or later is recommended.

The bare metal program to be debugged is loaded on Raspberry Pi 2B using
gdb's 'load'-command (see the INSTRUCTIONS.txt), and then run with gdb's
'cont'-command. You can break a runaway program with ctrl-C.

There are also some command line parameters.
'rpi_stub_mmu' causes rpi_stub to use MMU and caches. Default is no mmu or caches.
'rpi_stub_interrupt=<int>' makes rpi_stub to handle UART0 interrupts
in a different way:
If <int> is 'irq' rpi_stub uses irq exception also within the debugger.
If <int> is 'fiq' rpi_stub uses fiq exception also within the debugger.
if <int> is 'poll' rpi_stub uses irq when the debuggee runs, but the debugger
is run with interrupts masked, and the UART0 is handled by polling.
The default (and recommended) option is 'poll'.
'rpi_stub_keep_ctrlc' makes rpi_stub to re-enable UART0 interrupts each time
the execution is returned to the debuggee.
'rpi_stub_baud=<baudrate>' makes rpi_stub set the UART0 baudrate to <baudrate>
It uses the UART clock from the GPU and the <baudrate> parameter to calculate the
ibrd and fbrd for UART0. The sensibility of the parameters are not checked.
'rpi_stub_hw_dbg=<n>' enables or disables HW watchpoints, because using HW
breaks makes the debuggee to run in debug monitor mode, and in some cases it
may be harmful. If <n> = 1 the HW watchpoints are enabled, if <n> = 0. the HW
watchpoints are disabled. Default is 1.
'rpi_stub_dbg_info', if present, causes rpi_stub to print out some info about
the debug HW present, and some settings, like whether or not FP and SIMD are enabled.
To use this parameter, start with serial terminal instead of gdb, and after
the info is printed, close the serial terminal and start gdb normally.
'rpi_stub_enable_neon', if present, causes rpi_stub to try to enable Neon and support
for xml description of Neon registers. It initializes Neon (if possible) and turns
also the 'rpi_stub_use_neon'-option on.
'rpi_stub_use_neon', if present, causes rpi_stub to believe that the debuggee
enables the Neon and turns on support for xml target description. If used gdb version
supports xml target descriptions the Neon registers can be seen with
'info all-registers', and Neon registers can be read and written.
NOTE: if debuggee fails to enable neon, rpi_stub probably crashes, because
rpi_stub doesn't poll the Neon state before accessessing it.

At the moment the main restrictions are:
- Only ARM instruction set is supported
- Floating point and vector registers not supported, but instructions should work.
  (If not, try parameter 'rpi_stub_dbg_info' to find out why.)
- Only single core supported
- UART0 (the full UART) is reserved exclusively for the rpi_stub.
- The breakpoints bkpt #0x7ffb to 0x7fff are reserved exclusively for the stub.
- The double vectoring adds exception latency, especially for IRQ.
- Watchpoints work with newer gdb-versions that can do single-stepping,
  without stub support, using breakpoints, but rpi_stub single-stepping support
  works too, at least to some extent (mostly untested).
  (If watchpoints don't work, try parameter 'rpi_stub_dbg_info' to find out why.)

Breakpoint #0x7ffc and #0x7ffb can be used for sending messages to gdb client.
The pointer to the string needs to be in r0.
#0x7ffc sends a null-terminated string and #0x7ffb needs the length to be in r1.

I could list the gdb serial protocol commands that rpi_stub supports,
but it wouldn't be of much help for a gdb user - the gdb-commands and gdb
serial protocol commands do not map one-to-one.

Roughly: rpi_stub supports:
- loading programs
- 'cont'
- SW breakpoints
- HW watchpoints (see limitations)
- single-stepping (see limitations)
- reading and writing registers r0 - r15 and cpsr
- reading and writing memory
- stopping with ctrl-C
- through-gdb logging

About mmu, caches and UART0 configuration (including interrupt), check
the command line parameters.

Also, gdb can do single stepping without single-stepping support just
by using breakpoints. At least newer 'gdb-multiarch'-versions (like 7.7.1)
can do single-stepping. Also arm-linux-gnueabihf-gdb 7.6.1 seems to work.
Seems like arm-none-eabi-gdb 7.7.1 doesn't support single-stepping, but
works - at least to some extent - with rpi_stub singe-stepping support.

If you need to ask something, contact me via the e-mail mentioned in the
github. You can also send bug reports either via the e-mail or via the
'issues' on the github page.


This is my very first RPi(2) project ever, and first ever project that
I have to deal with ARM core (and assembly).
This project started in the first days of april 2015.

This will hopefully become a standalone gdb-stub for RPi2 using serial line
for communicating with a gdb client.

I don't take any responsibility of anything that happens, if someone dares to
try this. You can use this but totally on your own risk.
I don't have any means of debugging, so I have to write this code "blindfolded".
I may fix this and that and even expand this, but I give no promises to maintain
this in any way.

Hopefully this is useful reading material for those that want to learn about
gdb-stubs.

As a by-product I put together a spreadsheet table (OpenOffice Calc) of both
ARM and Thumb instructions to help figuring out the decoding. I couldn't find
them anywhere in the net - not even in text format.
Feel free to download them and play with them.


--------------------------
Some kind of description

This is a rough 'conceptual' pseudocode of the program
Here asynchronous/independent code sequences are called processes

PROCESS loader
	handle command line parameters
    // set up exceptions and some other low level stuff (MMU, caches)
    CALL rpi2_init
    // set up serial (UART0)
    CALL serial_init
    FOREVER
    	CALL gdb_init // initialize debugger
        CALL gdb_reset // initialize for debugging a program
        execute initial-entry bkpt
    ENDFOREVER
ENDPROCESS

// called from excepion initializations and reset_gdb_stub
PROCEDURE set_up_exception_vectors
	lower_vectors =
		".word rpi2_reset_handler\n\t"
		".word rpi2_undef_handler\n\t"
		".word rpi2_svc_handler\n\t"
		".word rpi2_dabt_handler\n\t"
		".word rpi2_unhandled_dabt\n\t"
		".word rpi2_aux_handler @ used in some cases\n\t "
		".word rpi2_unhandled_irq\n\t"
		".word rpi2_unhandled_fiq\n\t"
		
	upper_vectors =
		".word rpi2_reroute_reset\n\t"
		".word rpi2_reroute_exc_und\n\t"
		".word rpi2_reroute_exc_svc\n\t"
		".word rpi2_pabt_handler\n\t"
		".word rpi2_reroute_exc_dabt\n\t"
		".word rpi2_reroute_exc_aux @ in some cases\n\t "
		".word rpi2_irq_handler\n\t"
		".word rpi2_reroute_exc_fiq\n\t"
		
	IF rpi_stub_interrupt is 'fiq' THEN
		rpi2_upper_vector[IRQ] = &rpi2_reroute_exc_irq
		rpi2_upper_vector[FIQ] = &rpi2_fiq_handler
	ENDIF
ENDPROC

PROCESS rpi2_reroute_<exception>
	"mov PC, low_vector_address_of<exception>
ENDPROC

PROCESS rpi2_unhandled_<exception>
    store sp locally
    switch to rpi_stub stack
	fix return address
	store register cotext to this exception's register storage
	mark exception reason and type
	GOTO gdb_exception
ENDPROC

PROCESS pabt_exception_handler (upper vector)
    store sp locally
    switch to rpi_stub stack
    push registers
	IF debug event THEN
		IF rpi_stub breakpoint THEN
			mark exception reason and type
          	pop registers
    		load locally stored sp
    		fix return address
    		store register cotext to this exception's register storage
    		GOTO gdb_exception			
		ELSE
			pop registers
			load locally stored sp
			"mov PC, low_vector_address_of_PABT
		ENDIF
	ELSE
		pop registers
		load locally stored sp
		"mov PC, low_vector_address_of_PABT
	ENDIF
ENDPROCESS

PROCESS irq_fiq_exception_handler (upper vector)
    store sp locally
    switch to rpi_stub stack
    push registers
    IF UART0 interrupt THEN
    	CALL serial_interrupt_handler
    	IF ctrl-C flag THEN
    		clear ctrl-C flag
    		set sigint flag (instead)
    		mark exception reason and type
          	pop registers
    		load locally stored sp
    		fix return address
    		store register cotext to this exception's register storage
    		GOTO gdb_exception
    	ELSE
    		IF irq THEN
    			IF other irqs THEN
        			pop registers
    				load locally stored sp
    				"mov PC, low_vector_address_of<exception>
    			ENDIF
    		ENDIF
          	pop registers
    		load locally stored sp
    		return from interrupt
    	ENDIF
    ELSE
    	pop registers
    	load locally stored sp
    	"mov PC, low_vector_address_of<exception>
    ENDIF
ENDPROCESS    

PROCEDURE gdb_exception
	switch mode to ABT
	START gdb_exception_handler
ENDPROC

PROCESS gdb_exception_handler
	check which exception type
	copy processor context from exception register storage to
		debugger register storage
	CALL gdb_monitor
	IF rpi_stub_keep_ctrlc THEN
		IF rpi_stub_interrupt is 'fiq' THEN
			try to force-enable UART0 FIQ
		ELSE
			try to force-enable UART0 IRQ
		ENDIF
	ENDIF
	load registers from debugger storage
	return from exception
ENDPROCESS

PROCEDURE serial_interrupt_handler
    IF data to send in queue THEN
        send data from queue
    ENDIF

    IF data to receive THEN
        receive data to queue
        IF ctrl-C encountered THEN
        	IF ctrl-C handling enabled
            	set ctrl_C flag
            	discard ctrl-C character
            ENDIF
        ENDIF
    ENDIF
ENDPROC

PROCEDURE gdb_monitor
	IF rpi_stub_interrupt is not 'poll' THEN
    	enable UART0 interrupt
    ENDIF
    handle debug exception (all except reset, serial tx and serial rx)
    set stub_running to TRUE
    handle pending status (like remove single-stepping breakpoints)
    send pending response to gdb-client (response for s,c,...)
    IF pending status expects continuation THEN
    	set stub_running to FALSE
    ENDIF
    disable ctrl-C handling 
    WHILE stub_running DO
    	get command packet
    	IF NACK received THEN
    		resend last packet
    		continue
    	ENDIF
    	IF ACK received THEN
    		continue
    	ENDIF
    	IF command packet is good THEN
    		send ACK
    	ELSE
    		send NACK
    		continue
    	ENDIF
        CALL gdb-stub command_interpreter
        // to resume, set stub_running to FALSE and return from call
    ENDWHILE
    enable ctrl-C handling 
ENDPROC

When resume takes place, a variable 'gdb_resuming' is set (to the index of
the trapping breakpoint) for resuming from single stepping over the
"breakpointed" instruction is made.

When the single step fires, the single step handling in gdb_handle_pending_state
puts the "missing" bkpt in the code and resumes again.


------------------------
Notes and reminders to myself


single-stepping
in command interpreter s-command checks the next instruction
if instruction is branch, set breakpoint to branch target
if instruction is c�nditiomal branch, set bkpt to branch target and next instruction
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


