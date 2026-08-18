
    .section .text
    .globl _start

_start:
    la  x1, addr

    lr.d x0, 0(x1)
    sc.d x0, x0, 0(x1)

    lr.w x0, 0(x1)
    sc.w x0, x0, 0(x1)

    amoswap.d x0, x0, 0(x1)
    amoadd.d  x0, x0, 0(x1)
    amoxor.d  x0, x0, 0(x1)
    amoand.d  x0, x0, 0(x1)
    amoor.d   x0, x0, 0(x1)
    amomin.d  x0, x0, 0(x1)
    amomax.d  x0, x0, 0(x1)
    amominu.d x0, x0, 0(x1)
    amomaxu.d x0, x0, 0(x1)

    amoswap.w x0, x0, 0(x1)
    amoadd.w  x0, x0, 0(x1)
    amoxor.w  x0, x0, 0(x1)
    amoand.w  x0, x0, 0(x1)
    amoor.w   x0, x0, 0(x1)
    amomin.w  x0, x0, 0(x1)
    amomax.w  x0, x0, 0(x1)
    amominu.w x0, x0, 0(x1)
    amomaxu.w x0, x0, 0(x1)

    # AMO register aliasing tests
    # use AMOSWAP as representative

    # baseline *addr= 0xcafe, data to be swapped = 0xbeef
.macro prepare
    la  x1, addr
    li x31, 0xcafe
    sd x31, 0(x1)
    li  x3, 0xbeef
    li x30, 0xbeef
.endm

    prepare

    # test rs1 = rd (=x1)
    # Expected: *addr = 0xbeef, x1 = 0xcafe
    amoswap.d x1, x3, 0(x1)
    bne x1, x31, fail
    la  x1, addr
    ld  x29, 0(x1)
    bne x29, x30, fail

    prepare

    # test rs2 = rd
    # Expected: *addr = 0xbeef, x3 = 0xcafe
    amoswap.d x3, x3, 0(x1)
    bne x3, x31, fail
    ld  x29, 0(x1)
    bne x29, x30, fail

    prepare

    # test rs1 = rd (=x1)
    # Expected: *addr = 0xbeef, x1 = 0xcafe
    amoswap.w x1, x3, 0(x1)
    bne x1, x31, fail
    la  x1, addr
    ld  x29, 0(x1)
    bne x29, x30, fail

    prepare

    # test rs2 = rd
    # Expected: *addr = 0xbeef, x3 = 0xcafe
    amoswap.w x3, x3, 0(x1)
    bne x3, x31, fail
    ld  x29, 0(x1)
    bne x29, x30, fail

    prepare

    # test LR.D with rs1 = rd (=x1)
    # LR should reserve the original address even though x1 receives *addr.
    addi x2, x1, 0
    lr.d x1, 0(x1)
    bne x1, x31, fail
    sc.d x4, x3, 0(x2)
    bne x4, x0, fail
    ld  x29, 0(x2)
    bne x29, x30, fail

    # end of test, go to pass
    j pass

fail:
    addi a0, x0, 1
    j end
pass:
    addi a0, x0, 0
    j end
end:
    # End the smoke test using the temporary Phase 1 halt convention.
    ebreak


    .section .data
    .balign 8
addr:
    .dword 0xa0a0a0a
