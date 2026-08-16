
    .section .text
    .globl _start

_start:
    # This is an instruction-recognition smoke test. It deliberately checks
    # only that every RV64I instruction implemented by the emulator can be
    # decoded and executed without hitting the invalid-instruction path.

    # U-type instructions
    lui   x2, 1
    auipc x2, 1

    # Integer register-immediate instructions
    addi  x2, x0, 1
    slti  x2, x0, 1
    sltiu x2, x0, 1
    xori  x2, x0, 1
    ori   x2, x0, 1
    andi  x2, x0, 1
    slli  x2, x0, 1
    srli  x2, x0, 1
    srai  x2, x0, 1

    # RV64I word register-immediate instructions
    addiw x2, x0, 1
    slliw x2, x0, 1
    srliw x2, x0, 1
    sraiw x2, x0, 1

    # Integer register-register instructions
    add  x2, x0, x0
    sub  x2, x0, x0
    sll  x2, x0, x0
    slt  x2, x0, x0
    sltu x2, x0, x0
    xor  x2, x0, x0
    srl  x2, x0, x0
    sra  x2, x0, x0
    or   x2, x0, x0
    and  x2, x0, x0

    # RV64I word register-register instructions
    addw  x2, x0, x0
    subw  x2, x0, x0
    sllw  x2, x0, x0
    srlw  x2, x0, x0
    sraw  x2, x0, x0

    # Load and store instructions. Keep the effective addresses naturally
    # aligned because alignment behavior is outside this smoke test's scope.
    la  x3, scratch
    lb  x2, 1(x3)
    lh  x2, 2(x3)
    lw  x2, 4(x3)
    ld  x2, 8(x3)
    lbu x2, 1(x3)
    lhu x2, 2(x3)
    lwu x2, 4(x3)

    sb x2, 1(x3)
    sh x2, 2(x3)
    sw x2, 4(x3)
    sd x2, 8(x3)

    # Every branch targets its immediately following instruction. This keeps
    # the smoke-test path linear whether a branch is taken or not.
    addi x1, x0, 1
    beq  x0, x1, 1f
1:
    bne  x0, x0, 2f
2:
    blt  x1, x0, 3f
3:
    bge  x0, x1, 4f
4:
    bltu x1, x0, 5f
5:
    bgeu x0, x1, 6f
6:
    # JAL targets the next instruction. Build the JALR target explicitly from
    # the current PC so no additional ISA extension is required.
    jal x5, jal_target

jal_target:
    auipc x6, 0
    addi  x6, x6, 12
    jalr  x5, 0(x6)

jalr_target:
    # Memory ordering and environment instructions. In this phase FENCE and
    # ECALL are recognition-only no-ops in the emulator.
    fence
    ecall

    # End the smoke test using the temporary Phase 1 halt convention.
    addi a0, x0, 0
    ebreak

    .balign 8
scratch:
    .dword 0x0123456789abcdef
    .dword 0xfedcba9876543210
