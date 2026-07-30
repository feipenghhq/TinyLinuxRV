    .section .text
    .globl _start

_start:
    addi x1, x0, 5      # x1 = 5
    addi x2, x0, 7      # x2 = 7
    add  x3, x1, x2     # x3 = 12

halt:
    jal  x0, halt       # infinite loop
