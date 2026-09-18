
    .section .text
    .globl _start

_start:
    mul    x1, x0, x0
    mulh   x1, x0, x0
    mulhsu x1, x0, x0
    mulhu  x1, x0, x0
    div    x1, x0, x0
    divu   x1, x0, x0
    rem    x1, x0, x0
    remu   x1, x0, x0

    mulw   x1, x0, x0
    divw   x1, x0, x0
    divuw  x1, x0, x0
    remw   x1, x0, x0
    remuw  x1, x0, x0

    addi a0, x0, 0

    # write to syscon with poweroff command to indicate test end
    li t0, 0x00100000
    li t1, 0x1
    sw t1, 0(t0)
