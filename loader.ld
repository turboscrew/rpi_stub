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
    }
    __bss_end = .;
    
    __new_org = 0x1f000000;
    __end = .;
}
