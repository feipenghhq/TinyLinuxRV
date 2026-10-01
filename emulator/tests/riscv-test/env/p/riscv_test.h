// See LICENSE for license details.

// !Note: RVTEST_HAS_CSR is introduced in TinyLinuxRV to make the test works when the CSR and the exception has not been
// !      implemented yet. The macro is defined at the Makefile

#ifndef _ENV_PHYSICAL_SINGLE_CORE_H
#define _ENV_PHYSICAL_SINGLE_CORE_H

#include "encoding.h"

//-----------------------------------------------------------------------
// Begin Macro
//-----------------------------------------------------------------------

#define RVTEST_RV64U \
    .macro init;     \
    .endm

#define RVTEST_RV64M       \
    .macro init;           \
    RVTEST_ENABLE_MACHINE; \
    .endm

#define RVTEST_RV64S          \
    .macro init;              \
    RVTEST_ENABLE_SUPERVISOR; \
    .endm

// clang-format off
#if __riscv_xlen == 64
# define CHECK_XLEN li a0, 1; slli a0, a0, 31; bgez a0, 1f; RVTEST_PASS; 1:
#else
# define CHECK_XLEN li a0, 1; slli a0, a0, 31; bltz a0, 1f; RVTEST_PASS; 1:
#endif
// clang-format on

#define INIT_XREG \
    li x1, 0;     \
    li x2, 0;     \
    li x3, 0;     \
    li x4, 0;     \
    li x5, 0;     \
    li x6, 0;     \
    li x7, 0;     \
    li x8, 0;     \
    li x9, 0;     \
    li x10, 0;    \
    li x11, 0;    \
    li x12, 0;    \
    li x13, 0;    \
    li x14, 0;    \
    li x15, 0;    \
    li x16, 0;    \
    li x17, 0;    \
    li x18, 0;    \
    li x19, 0;    \
    li x20, 0;    \
    li x21, 0;    \
    li x22, 0;    \
    li x23, 0;    \
    li x24, 0;    \
    li x25, 0;    \
    li x26, 0;    \
    li x27, 0;    \
    li x28, 0;    \
    li x29, 0;    \
    li x30, 0;    \
    li x31, 0;

// Not used yet
#define INIT_PMP
// #define INIT_PMP                                                        \
//   la t0, 1f;                                                            \
//   csrw mtvec, t0;                                                       \
//   /* Set up a PMP to permit all accesses */                             \
//   li t0, (1 << (31 + (__riscv_xlen / 64) * (53 - 31))) - 1;             \
//   csrw pmpaddr0, t0;                                                    \
//   li t0, PMP_NAPOT | PMP_R | PMP_W | PMP_X;                             \
//   csrw pmpcfg0, t0;                                                     \
//   .align 2;                                                             \
// 1:
//

#ifdef RVTEST_HAS_CSR

#define INIT_RNMI                      \
    la    t0, 1f;                      \
    csrw  mtvec, t0;                   \
    csrwi CSR_MNSTATUS, MNSTATUS_NMIE; \
    .align 2;                          \
    1:

#define INIT_SATP    \
    la    t0, 1f;    \
    csrw  mtvec, t0; \
    csrwi satp, 0;   \
    .align 2;        \
    1:

#define DELEGATE_NO_TRAPS \
    csrwi mie, 0;         \
    la    t0, 1f;         \
    csrw  mtvec, t0;      \
    csrwi medeleg, 0;     \
    csrwi mideleg, 0;     \
    .align 2;             \
    1:

#define RVTEST_ENABLE_SUPERVISOR              \
    li   a0, MSTATUS_MPP &(MSTATUS_MPP >> 1); \
    csrs mstatus, a0;                         \
    li   a0, SIP_SSIP | SIP_STIP;             \
    csrs mideleg, a0;

#define RVTEST_ENABLE_MACHINE \
    li   a0, MSTATUS_MPP;     \
    csrs mstatus, a0;

#else

#define INIT_RNMI
#define INIT_SATP
#define DELEGATE_NO_TRAPS
#define RVTEST_ENABLE_SUPERVISOR
#define RVTEST_ENABLE_MACHINE

#endif

#define EXTRA_TVEC_USER
#define EXTRA_TVEC_MACHINE
#define EXTRA_INIT
#define EXTRA_INIT_TIMER
#define FILTER_TRAP
#define FILTER_PAGE_FAULT

// clang-format off
#define RVTEST_CODE_BEGIN                                               \
        .section .text.init;                                            \
        .align  6;                                                      \
        .weak stvec_handler;                                            \
        .weak mtvec_handler;                                            \
        .globl _start;                                                  \
_start:                                                                 \
        /* reset vector */                                              \
        j reset_vector;                                                 \
        .align 2;                                                       \
reset_vector:                                                           \
        INIT_XREG;                                                      \
        INIT_RNMI;                                                      \
        INIT_SATP;                                                      \
        INIT_PMP;                                                       \
        DELEGATE_NO_TRAPS;                                              \
        li TESTNUM, 0;                                                  \
        la t0, mtvec_handler;                                           \
        csrw mtvec, t0;                                                 \
        CHECK_XLEN;                                                     \
        /* if an stvec_handler is defined, delegate exceptions to it */ \
        la t0, stvec_handler;                                           \
        beqz t0, 1f;                                                    \
        csrw stvec, t0;                                                 \
        li t0, (1 << CAUSE_LOAD_PAGE_FAULT) |                           \
               (1 << CAUSE_STORE_PAGE_FAULT) |                          \
               (1 << CAUSE_FETCH_PAGE_FAULT) |                          \
               (1 << CAUSE_MISALIGNED_FETCH) |                          \
               (1 << CAUSE_USER_ECALL) |                                \
               (1 << CAUSE_BREAKPOINT);                                 \
        csrw medeleg, t0;                                               \
1:      csrwi mstatus, 0;                                               \
        init;                                                           \
        EXTRA_INIT;                                                     \
        EXTRA_INIT_TIMER;                                               \
        la t0, 1f;                                                      \
        csrw mepc, t0;                                                  \
        csrr a0, mhartid;                                               \
        mret;                                                           \
1:
// clang-format on

//-----------------------------------------------------------------------
// End Macro
//-----------------------------------------------------------------------

#define RVTEST_CODE_END unimp

//-----------------------------------------------------------------------
// Pass/Fail Macro
//-----------------------------------------------------------------------

// write to syscon with poweroff command to indicate test end
#define TEST_END       \
    li t0, 0x00100000; \
    li t1, 0x1;        \
    sw t1, 0(t0)

// write 0 to a0 to indicate test pass
#define RVTEST_PASS \
    fence;          \
    li a0, 0;       \
    TEST_END

// write 1 to a0 to indicate test pass
// write TESTNUM to a1 to indicate the failed test
#define TESTNUM gp
#define RVTEST_FAIL \
    fence;          \
    li a0, 1;       \
    mv a1, TESTNUM; \
    TEST_END

//-----------------------------------------------------------------------
// Data Section Macro
//-----------------------------------------------------------------------

#define EXTRA_DATA

#define RVTEST_DATA_BEGIN    \
    EXTRA_DATA.align 4;      \
    .global begin_signature; \
    begin_signature:

#define RVTEST_DATA_END    \
    .align 4;              \
    .global end_signature; \
    end_signature:

#endif
