OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    ram (rxw) : ORIGIN = 0x80000000, LENGTH = 1M
}

SECTIONS
{
    . = 0x80000000;

    .text.init : {*(.text.init .text.init.*)} > ram
    .text : {*(.text .text.*)} > ram
    .rodata : {*(.rodata .rodata.*)} > ram
    .data : {*(.data .data.*)} > ram
    .sdata : {
        /* crt0 loads this address into gp. The +0x800 placement lets gp's
           signed 12-bit offsets cover about 4 KiB of .sdata and .sbss. */
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
    } > ram

    /* Reserve zero-initialized data in RAM without storing zeros in the ELF. */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } > ram

    /* Keep the end of the image ready for future aligned allocations. */
    . = ALIGN(16);
    __image_end = .;

    STACK_SIZE = 16K;
    /* The stack starts at the top of RAM and grows toward lower addresses. */
    __stack_top    = ORIGIN(ram) + LENGTH(ram);
    __stack_bottom = __stack_top - STACK_SIZE;

    /* Fail at link time if the program image reaches the reserved stack. */
    ASSERT(__image_end <= __stack_bottom, "program image overlaps the stack")
}
