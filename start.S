.extern __code_begin
.extern __text_end
.extern __bss_end
.extern __fiq_stack
.extern __usrsys_stack
.extern __svc_stack
.extern __irq_stack
.extern __mon_stack
.extern __hyp_stack
.extern __und_stack
.extern __abrt_stack
.extern __new_org
.extern start1

.globl _start

.text
_start:

// Entry point for the kernel.
// r15 -> should begin execution at 0x8000.
// r0 -> 0x00000000
// r1 -> 0x00000C42
// r2 -> 0x00000100 - start of ATAGS
// preserve these registers as argument for kernel_main

cpsid aif @ disable interrupts

// copy loader/stub into upper memory
ldr r4, =__code_begin
ldr	r5, =__bss_end
ldr	r6, =__new_org
loop$:
ldr r7,[r4], #4
str	r7,[r6], #4
cmp r4, r5
bmi	loop$

// Set up initial stacks
ldr r13, =__svc_stack @ our stack (at startup)

// store our cpsr to r4
mrs r6, cpsr
mov r4, r6

// cpsr with zeroed mode into r2 for easier mode change
ldr r5, =0xfffffff0 @ clean up mode bits
and r6, r5 @ store mode bit masked cpsr in r6

// handle FIQ mode
orr r5, r6, #0x01 @ set FIQ mode
msr cpsr, r5
ldr r13, =__fiq_stack @ fast interrupt mode stack

// handle user and system modes - common sp and lr
// but sys is privileged
orr r5, r6, #0x0f @ set system mode
msr cpsr, r5
ldr r13, =__usrsys_stack @ system and user mode stack

//handle IRQ mode
orr r1, r2, #0x03 @ set IRQ mode
msr cpsr_c, r5
ldr r13, =__irq_stack @ irq mode stack

// handle monitor mode
/* probably won't work due to non-secure mode
orr r1, r2, #0x06 @ set monitor mode
msr cpsr_c, r5
ldr r13, =__mon_stack @ monitor mode stack
*/

// handle hyp mode
orr r1, r2, #0x0a @ set hyp mode
msr cpsr_c, r5
ldr r13, =__hyp_stack @ hypervisor mode stack

// handle und mode
orr r1, r2, #0x0b @ set und mode
msr cpsr_c, r5
ldr r13, =__und_stack @ undefined instruction mode stack

// handle abort modes
orr r5, r6, #0x07 @ set abort mode
msr cpsr_c, r5
ldr r13, =__abrt_stack @ abort mode stack

// return to our original mode
msr cpsr, r4

@ copy vectors
ldr r4, =_Int_vectors
mov	r5, #16
mov	r6, #0
loop1$:
ldr r7,[r4], #4
str	r7,[r6], #4
subs r5, r5, #1
bne	loop1$

cpsie aif @ enable interrupts
ldr pc, =start1

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
	.word gen_exc_4
	.word gen_exc_0
	.word gen_exc_4
	.word gen_exc_8
	.word _start @ not used
	.word gen_exc_4
	.word gen_exc_4

	// dummy exception handlers
	gen_exc_0:
	subs pc, lr, #0 @ return from exception

	gen_exc_2:
	subs pc, lr, #2 @ return from exception (Thumb only)

	gen_exc_4:
	subs pc, lr, #4 @ return from exception

	gen_exc_8:
	subs pc, lr, #8 @ return from exception

