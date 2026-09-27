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
    .data : {
        _data_lma = .;
        _data = .;
        *(.data .data.*)
    } > ram
    .sdata : {
        /* crt0 loads this address into gp. The +0x800 placement lets gp's
           signed 12-bit offsets cover about 4 KiB of .sdata and .sbss. */
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
        _edata = .;
    } > ram

    /* Reserve zero-initialized data in RAM without storing zeros in the ELF. */
    .bss (NOLOAD) : {
        _bss = .;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        _ebss = .;
    } > ram

    /* Keep the end of the image ready for future aligned allocations. */
    . = ALIGN(16);
    _image_end = .;

    STACK_SIZE = 16K;
    /* The stack starts at the top of RAM and grows toward lower addresses. */
    _stack_top    = ORIGIN(ram) + LENGTH(ram);
    _stack_bottom = _stack_top - STACK_SIZE;

    /* Fail at link time if the program image reaches the reserved stack. */
    ASSERT(_image_end <= _stack_bottom, "program image overlaps the stack")
}
