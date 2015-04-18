ENTRY(_start)

SECTIONS
{
	. = 0x0000;
	.text :
	{
		start*.o(.vector)
	}
    /* Starts at LOADER_ADDR. */
    . = 0x8000;
    __text_start = .;
    .text :
    {
		/* If special boot section */
        /* KEEP(*(.text.boot)) */
        *(.init)
        start1*.o(.text)
        __code_begin = .;
        *(.text)
    }
    /* align to page size */
    /*    . = ALIGN(4096); */
    __text_end = .;
 

 	__data_start = .;
    .data :
    {
        *(.data)
    }
    __data_end = .;
 
 
    __bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
        stacks = .;
		. = . + 512;	/* fiq stack size */
		__fiq_stack = .;
		. = . + 1024;	/* usr & sys stack size (start-up) */
		__usrsys_stack = .;
		. = . + 1024;	/* irq & svc stack size (serial) */
		__irqsvc_stack = .;
		. = . + 512;	/* mon stack size */
		__mon_stack = .;
		. = . + 512;	/* hyp stack size */
		__hyp_stack = .;
		. = . + 512;	/* und stack size */
		__und_stack = .;
		. = . + 8192;	/* abrt stack size (gdb-stub) */
		__abrt_stack = .;
    }
    __bss_end = .;
    
    __new_org = 0x1f000000;
    __end = .;
}
