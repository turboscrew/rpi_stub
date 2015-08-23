ENTRY(_start)

MEMORY
{
	LOAD (rwx) : ORIGIN = 0x00008000, LENGTH = 256k /* initial */
	EXEC (rwx) : ORIGIN = 0x1f000000, LENGTH = 256k /* runtime */
}

SECTIONS
{	
    /* Starts at LOADER_ADDR. */
    . = 0x8000;
    __text_start = .;
    .text :
    {
        *(.init)
        /* start1.o */
    } >LOAD
    
    .note :
    {
    	*(.note.*)
    } >LOAD
    
    .text2 :
    {
        __code_begin = .;
        /* *(EXCLUDE_FILE(*start1.o) .text) */
        *(.text)
    } >LOAD /* >EXEC AT>LOAD */
    __text_end = .;
 

 	__data_start = .;
    .data :
    {
        *(.data)
    } >LOAD /* >EXEC AT>LOAD */
    __data_end = .;
 
 
    __bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
        stacks = .;
		. = . + 512;	/* fiq stack size */
		__fiq_stack = .;
		. = . + 1024;	/* usr & sys stack size (common) */
		__usrsys_stack = .;
		. = . + 2048;	/* svc stack size (start-up) */
		__svc_stack = .;
		. = . + 2048;	/* irq stack size (serial) */
		__irq_stack = .;
		. = . + 512;	/* mon stack size */
		__mon_stack = .;
		. = . + 512;	/* hyp stack size */
		__hyp_stack = .;
		. = . + 512;	/* und stack size */
		__und_stack = .;
		. = . + 8192;	/* abrt stack size (gdb-stub) */
		__abrt_stack = .;
    } >LOAD /* >EXEC AT>LOAD */
    __bss_end = .;
    
    __new_org = 0x1f000000;
    __end = .;
}
