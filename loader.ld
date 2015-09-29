ENTRY(_start)

MEMORY
{
	LOAD (rwx) : ORIGIN = 0x00008000, LENGTH = 512k /* initial */
	EXEC (rwx) : ORIGIN = 0x1f000000, LENGTH = 512k /* runtime */
}

SECTIONS
{	
    /* Starts at LOADER_ADDR. */
    . = 0x8000;
    __text_start = .;
    .text :
    {
        *(.init)
        *start1.o(.text)
        *start1.o(.data)
        *start1.o(.bss)
        *(.text.startup)
    } >LOAD
    
    /* .text2 ALIGN(0x1000):  - the ">EXEC AT>LOAD" didn't like "ALIGN(0x1000)" */
    .text2 :
    {
    	. = ALIGN(.,0x8);
        *loader.o(.text)
        *rpi2.o(.text)
        *serial.o(.text)
        *util.o(EXCLUDE_FILE(*instr_util.o).text)
        *gdb.o(.text)
        *(.text)
    } >EXEC AT>LOAD
    __text_end = .;
    __load_start = LOADADDR(.text2);
    __code_begin = ADDR(.text2);
 

     .data :
    {
    	. = ALIGN(0x8); /* defaults to ALIGN(.,0x8) */
 		__data_start = .;
        *(.data)
    } >EXEC AT>LOAD
    __data_end = .;
 
 
    __bss_start = .;
    .bss :
    {
    	. = ALIGN(0x8);
 	    __bss_start = .;
        bss = .;
        *(.bss)
        stacks = .;
		. = . + 512;	/* fiq stack size */
		__fiq_stack = .;
		. = . + 1024;	/* usr & sys stack size (common) */
		__usrsys_stack = .;
		. = . + 16384;	/* svc stack size (start-up) */
		__svc_stack = .;
		. = . + 16384;	/* irq stack size (serial) */
		__irq_stack = .;
		. = . + 512;	/* mon stack size */
		__mon_stack = .;
		. = . + 512;	/* hyp stack size */
		__hyp_stack = .;
		. = . + 4096;	/* und stack size */
		__und_stack = .;
		. = . + 16384;	/* abrt stack size (gdb-stub) */
		__abrt_stack = .;
    } >EXEC AT>LOAD
    __bss_end = .;
    __load_end = LOADADDR(.bss) + SIZEOF(.bss);


	/* gcc-generated crap */
    .note :
    {
    	*(.note.*)
    } >LOAD
    
    .debug :
    {
    	*(.debug*)
    } >LOAD
    
    __end = .;
}
