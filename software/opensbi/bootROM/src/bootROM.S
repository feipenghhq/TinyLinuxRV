
.section .init
.globl _boot:
_boot:
    li a0, 0            # hartid = 0
    li a1, DTB_ADDR     # device tree
    li t0, OPENSBI_ADDR
    jr t0
