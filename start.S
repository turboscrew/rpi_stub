.extern __code_begin
.extern __text_end
.extern start1

.globl _start

.section .vector
.align 4
	ldr pc, =0x1f000000
    ldr pc, =0 @ swi
    ldr pc, =0 @ prefetch_abort
    ldr pc, =0 @ data_abort
    ldr pc, =0 @ not_used
    ldr pc, =_serial_irq @ irq
    ldr pc, =0 @ fiq

.text
_start:

// Entry point for the kernel.
// r15 -> should begin execution at 0x8000.
// r0 -> 0x00000000
// r1 -> 0x00000C42
// r2 -> 0x00000100 - start of ATAGS
// preserve these registers as argument for kernel_main

ldr r4, =__code_begin
ldr	r5, =__text_end
ldr	r6, =0x1f000000
loop$:
ldr r7,[r4]
str	r7,[r6]

add	r4, r4, #1
add	r6, r6, #1
cmp r4, r5
bmi	loop$

ldr pc, =start1


