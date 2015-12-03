/*
start.S

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
//#define ENABLE_DEBUG_LEDS

.extern __code_begin
.extern __load_start
.extern __load_end
.extern __fiq_stack
.extern __usrsys_stack
.extern __svc_stack
.extern __irq_stack
.extern __mon_stack
.extern __hyp_stack
.extern __und_stack
.extern __abrt_stack
.extern __new_org
.extern start1_fun
.extern rpi2_debug_leds
.globl _start
.globl debug_blink
.globl debug_wait

.sect .init

_start:

	/*
	This is due to change (Oct 2, 2015) in the firmware
	( https://github.com/raspberrypi/firmware/tree/master/boot )
	to leave the core in HYP mode.
	(Issue #369 opened on Feb 18 by yann-morin-1998)
	*/
	str r0, r0_store
	str r1, r1_store
	str r2, r2_store
	mrs r0, cpsr
	and r1, r0, #0x1f
	cmp r1, #0x1a @ HYP-mode?
	bne codecopy
	ldr sp, =__hyp_stack @ hard to set later
	movw r0, #0x1d3 @ aif-masks set, SVC-mode, other bits zeroed
	@ rough write in cpsr doesn't work - see pseudo code in
	@ architecture reference manual:
	@"Pseudocode support for the Banked register transfer instructions"
	adr r1, codecopy
	msr ELR_hyp, r1
	msr SPSR_hyp, r0
	eret
r0_store:
	.int 0
r1_store:
	.int 0
r2_store:
	.int 0

	// copy loader/stub into upper memory
codecopy:
	cpsid aif
	ldr r13, =__svc_stack @ our stack (at startup)
#ifdef ENABLE_DEBUG_LEDS
	bl init_debug_led
#else
	bl disable_debug_led
#endif

#if 0
	ldr r0, =1000 @ led on - a second
	ldr r1, =1000 @ led off - a second
	mov r2, #3  @ 3 times
	bl debug_blink
	ldr r3, =3000 @ 3 s pause
	bl debug_wait
#endif
	ldr r4, =__load_start
	ldr	r5, =__load_end
	ldr	r6, =__code_begin
loop$:
	ldr r7,[r4], #4
	str	r7,[r6], #4
	cmp r4, r5
	bls	loop$

#if 0
	ldr r0, =1000 @ led on - a second
	ldr r1, =1000 @ led off - a second
	mov r2, #4  @ 4 times
	bl debug_blink
	ldr r3, =3000 @ 3 s pause
	bl debug_wait
#endif

	// Set up initial stacks
	// ARM stack is conventionally 'Decrement Before'-type
	// = SP points to last used, and is decremented before write

	@ldr r13, =__svc_stack @ our stack (at startup)

#if 0
	ldr r0, =__fiq_stack @ fast interrupt mode stack
	msr SP_fiq, r0
	ldr r0, =__usrsys_stack @ system and user mode stack
	msr SP_usr, r0
	ldr r0, =__irq_stack @ irq mode stack
	msr SP_irq, r0
	@ldr r0, =__svc_stack @ supervisor mode stack - skip: our mode
	@msr SP_svc, r0
	@ldr r0, =__mon_stack @ monitor mode stack - not from this mode
	@msr SP_mon, r0
	@ldr r0, =__hyp_stack @ hypervisor mode stack - not from this mode
	@msr SP_hyp, r0
	ldr r0, =__und_stack @ undefined instruction mode stack
	msr SP_und, r0
	ldr r0, =__abrt_stack @ abort mode stack
	msr SP_abt, r0
	dsb
#else
	// store our cpsr to r4
	mrs r6, cpsr
	mov r4, r6
	// cpsr with zeroed mode into r2 for easier mode change
	mvn r5, #0x0f @ clean up mode bits (load 0xfffffff0)
	and r6, r5 @ store mode bit masked cpsr in r6

	// handle FIQ mode
	orr r5, r6, #0x01 @ set FIQ mode
	msr cpsr, r5
	dsb
	isb
	ldr sp, =__fiq_stack @ fast interrupt mode stack

	// handle user and system modes - common sp and lr
	// but sys is privileged
	orr r5, r6, #0x0f @ set system mode
	msr cpsr, r5
	dsb
	isb
	ldr sp, =__usrsys_stack @ system and user mode stack

	//handle IRQ mode
	orr r5, r6, #0x02 @ set IRQ mode
	msr cpsr_c, r5
	dsb
	isb
	ldr sp, =__irq_stack @ irq mode stack

	//handle SVC mode - our mode: skip
	/*
	orr r5, r6, #0x03 @ set IRQ mode
	msr cpsr_c, r5
	dsb
	isb
	ldr sp, =__svc_stack @ irq mode stack
	*/

	/* handle monitor mode - unpredictable in svc-mode
	orr r5, r6, #0x06 @ set monitor mode
	msr cpsr_c, r5
	dsb
	isb
	ldr sp, =__mon_stack @ monitor mode stack
	*/

	/* handle hyp mode - unpredictable in svc-mode
	orr r5, r6, #0x0a @ set hyp mode
	msr cpsr_c, r5
	dsb
	isb
	ldr sp, =__hyp_stack @ hypervisor mode stack
	*/

	// handle und mode
	orr r5, r6, #0x0b @ set und mode
	msr cpsr_c, r5
	dsb
	isb
	ldr sp, =__und_stack @ undefined instruction mode stack

	// handle abort modes
	orr r5, r6, #0x07 @ set abort mode
	msr cpsr_c, r5
	dsb
	isb
	ldr sp, =__abrt_stack @ abort mode stack

	// return to our original mode
	msr cpsr, r4
	dsb
	isb
#endif

#if 0
	ldr r0, =1000 @ led on - a second
	ldr r1, =1000 @ led off - a second
	mov r2, #5  @ 5 times
	bl debug_blink
	ldr r3, =3000 @ 3 s pause
	bl debug_wait
#endif

	@ copy vectors - will be overwritten later
	ldr r4, =_Int_vectors
	mov	r5, #16
	mov	r6, #0
loop1$:
	ldr r7,[r4], #4
	str	r7,[r6], #4
	subs r5, r5, #1
	bne	loop1$

#if 0
	@ mov r3, #0x1000 @ 1 s pause
	@bl debug_wait
	ldr r0, =1000 @ led on - half a second
	ldr r1, =1000 @ led off - half a second
	mov r2, #6  @ 6 times
	bl debug_blink
	ldr r3, =3000 @ 3 s pause
	bl debug_wait
#endif
	ldr r0, r0_store
	ldr r1, r1_store
	ldr r2, r2_store
	ldr r3, =start1_fun
	blx r3
	// ldr pc, =start1_fun
	// bl start1_fun

boot_fault:
	mov r0, #0x500 @ led on - half a second
	mov r1, #0x500 @ led off - half a second
	mov r2, #5  @ 5 times
	bl debug_blink
	mov r3, #0x5000 @ 5 s pause
	bl debug_wait
	b boot_fault

@ dummy vectors for exceptions
.align 4
_Int_vectors:
	ldr pc, jumptbl    @ reset (ldr pc, [pc, #24])
	ldr pc, jumptbl+4  @ undef
	ldr pc, jumptbl+8  @ SVC
	ldr pc, jumptbl+12 @ prefetch
	ldr pc, jumptbl+16 @ data
	ldr pc, jumptbl+20 @ not used
	ldr pc, jumptbl+24 @ IRQ
	ldr pc, jumptbl+28 @ FIRQ
	jumptbl: @ 8 addresses
	.word _start
	.word gen_exc_4 @ undef
	.word gen_exc_0 @ SVC
	.word gen_exc_4 @ prefetch
	.word gen_exc_8 @ data
	.word gen_exc_0 @ aux
	.word gen_exc_4 @ IRQ
	.word gen_exc_4 @ FIQ

	// dummy exception handlers
gen_exc_0:
	cpsid aif
	mov r4, lr
1:
	ldr r0, =200 @ led on - half a second
	ldr r1, =200 @ led off - half a second
	mov r2, #5  @ 5 times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	ldr r0, =500 @ led on - 1 second
	ldr r1, =500 @ led off - half a second
	mov r2, #2  @ times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	mov r0, r4
	bl blink_value
	ldr r3, =5000 @ 5 s pause
	bl debug_wait
	b 1b @ eternal loop

gen_exc_2:
	cpsid aif
	mov r4, lr
2:
	ldr r0, =200 @ led on - half a second
	ldr r1, =200 @ led off - half a second
	mov r2, #5  @ 5 times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	ldr r0, =500 @ led on - 1 second
	ldr r1, =500 @ led off - half a second
	mov r2, #3  @ times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	mov r0, r4
	bl blink_value
	ldr r3, =5000 @ 5 s pause
	bl debug_wait
	b 2b @ eternal loop

gen_exc_4:
	cpsid aif
	mov r4, lr
3:
	ldr r0, =200 @ led on - half a second
	ldr r1, =200 @ led off - half a second
	mov r2, #5  @ 5 times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	ldr r0, =500 @ led on - 1 second
	ldr r1, =500 @ led off - half a second
	mov r2, #4  @ times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	mov r0, r4
	bl blink_value
	ldr r3, =5000 @ 5 s pause
	bl debug_wait
	b 3b @ eternal loop

gen_exc_8:
	cpsid aif
	mov r4, lr
4:
	ldr r0, =200 @ led on - half a second
	ldr r1, =200 @ led off - half a second
	mov r2, #5  @ 5 times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	ldr r0, =500 @ led on - 1 second
	ldr r1, =500 @ led off - half a second
	mov r2, #5  @ times
	bl debug_blink
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	mov r0, r4
	bl blink_value
	ldr r3, =5000 @ 5 s pause
	bl debug_wait
	b 4b @ eternal loop

@ 1 blink after "clock" = 0, 2 blinks = 1
@ value in r0
blink_value:
	push {r2 - r12, lr}
	mov r7, r0 @ value
	mov r6, #32 @ bitcount

bit_loop:
	@ blink "clock"
	ldr r0, =500 @ led on - half a second
	ldr r1, =500 @ led off - half a second
	mov r2, #1  @ once
	bl debug_blink

	lsls r7, #1
	bcs signal_one
signal_zero:
	@ blink "data"
	ldr r0, =200 @ led on
	ldr r1, =300 @ led off
	mov r2, #1  @ once
	bl debug_blink
	movw r3, #500
	bl debug_wait
	b next_bit
signal_one:
	ldr r0, =200 @ led on
	ldr r1, =300 @ led off
	mov r2, #2  @ twice
	bl debug_blink
next_bit:
	ldr r3, =1000 @ 1 s pause
	bl debug_wait
	sub r6, #1
	cmp r6, #0
	bne bit_loop
	pop {r2 - r12, lr}
	blx lr

disable_debug_led:
	ldr r3, =rpi2_debug_leds
	mov r2, #0
	str r2,[r3]
	bx lr

init_debug_led:
	mov r2, #150 @ delay count
	mov r3, #150 @ delay count
	ldr r1, =0x8000 @ 1 << 15
	mov r0, #0
	@mov r6, r0
	@movt r6, #0x3f20 @ GPIO base
	ldr r6, =0x3f200000
	str r0, [r6, #0x94] @ 0 -> GPPUD
	dsb
5:
	subs	r2, r2, #1 @ delay 150 cycles
	bne 5b
	str r1, [r6, #0x9c] @ 0 -> GPPUDCLK1
	dsb
6:
	subs	r3, r3, #1 @ delay 150 cycles
	bne 6b
	str r0, [r6, #0x94] @ 0 -> GPPUD
	str r0, [r6, #0x9c] @ 0 -> GPPUDCLK1
	dsb

	str r1, [r6, #0x2c] @ (1<<15) -> GPIO_CLRREG1
	dsb

	ldr r3, [r6, #0x10] @ read GPFSEL4
	dsb
	ldr r4, =0x00e00000
	bic r3, r4
	@bic	r3, #0xe00000
	mov r2, #0x1 @ or with 1 << 21 (output)
	lsl r2, #21
	orr r3, r2
	str r3, [r6, #0x10] @ write GPFSEL4
	dsb
	ldr r3, =rpi2_debug_leds
	mov r2, #1
	str r2,[r3]
	bx lr

systmr_wait:
	push {r0, r1, r2, r3}
	movw r1, 0x3004 @ sys timer low
	movt r1, 0x3f00
	ldr r3, [r1] @ start time
1:
	ldr r2, [r1] @ current time
	@ with 32-bit numbers (1<<32) - (start - curr) = curr - start
	sub r2, r3 @ with 32-bit numbers works
	cmp r0, r2
	bhi 1b
	pop {r0, r1, r2, r3}
	bx lr

debug_wait: @ ms count in r3
	push {r8, r9, r10, lr}
	mov r8, r3
	@lsl r8, #2
	bl debug_wait1
	pop {r8, r9, r10, lr}
	bx lr

debug_wait1: @ ms count in r8
#if 1
	push {r0, r1, lr}
	mov r0, r8
	movw r1, 0x3e8 @ 1000
	mul r0, r1
	bl systmr_wait
	pop {r0, r1, lr}
	bx lr
#else
	cmp	r8, #0
	bxeq	lr
	mov	r10, #0
loop4$:
	@ldr	r9, =0x0000061a
	ldr	r9, =0x0000028a
	@ldr	r9, =0x00000a28
loop5$:
	nop	{0}
	subs	r9, r9, #1
	bne loop5$
	add	r10, r10, #1
	cmp	r10, r8
	bne loop4$
	bx	lr
#endif

debug_blink: @ r0=on, r1=off, r2=count, 0x1000 ~ 1s
	push {r0, r1, r6, r7, r8, r9, r10, lr}
#ifdef ENABLE_DEBUG_LEDS
	ldr r6, =0x8000 @ 1 << 15
	@mov r7, #0
	@movt r7, #0x3f20
	ldr r7, =0x3f200000 @ GPIO base
loop6$:
	str r6, [r7,#0x20] @ (1<<15) -> GPIO_SETREG1
	dsb
	mov r8, r0 @ on
	bl debug_wait1
	str r6, [r7,#0x2c] @ (1<<15) -> GPIO_CLRREG1
	dsb
	mov r8, r1 @ off
	bl debug_wait1
	subs r2, #1
	bne loop6$
#endif
	pop {r0, r1, r6, r7, r8, r9, r10, lr}
	bx lr

