.extern __code_begin
.extern __text_end
.extern __bss_end
.extern __fiq_stack
.extern __usrsys_stack
.extern __irqsvc_stack
.extern __mon_stack
.extern __hyp_stack
.extern __und_stack
.extern __abrt_stack
.extern start1

.globl _start

/*
.section .vector
.align 4
	ldr pc, =_start
    ldr pc, =0 @ swi
    ldr pc, =0 @ prefetch_abort
    ldr pc, =0 @ data_abort
    ldr pc, =0 @ not_used
    ldr pc, =0 @ irq
    ldr pc, =0 @ fiq
*/

.text
_start:

// Entry point for the kernel.
// r15 -> should begin execution at 0x8000.
// r0 -> 0x00000000
// r1 -> 0x00000C42
// r2 -> 0x00000100 - start of ATAGS
// preserve these registers as argument for kernel_main

// copy loader/stub into upper memory
ldr r4, =__code_begin
ldr	r5, =__bss_end
ldr	r6, =0x1f000000
loop$:
ldr r7,[r4]
str	r7,[r6]

add	r4, r4, #4
add	r6, r6, #4
cmp r4, r5
bmi	loop$

// Set up initial stacks
ldr r13, =__irqsvc_stack @ our stack (at startup)

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
//handle IRQ and SVC modes - common sp and lr
orr r1, r2, #0x03 @ set SVC mode
msr cpsr_c, r5
ldr r13, =__irqsvc_stack @ irq and svc mode stack
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

ldr pc, =start1

