.section .text.trap
.globl mtrap_init
mtrap_init:
    // set trap entry to mtvec
    la t0, mtrap_entry
    csrw mtvec, t0

    // set mie field in mstatus
    li t0, 8
    csrs mstatus, t0

    // set msie, mtie, and meie field in mie
    li t0, 0x888
    csrs mie, t0

    ret

