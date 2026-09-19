// See LICENSE for license details.

// !Note: RVTEST_HAS_CSR is defined at the Makefile

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

#define RVTEST_ENABLE_MACHINE \
    li   a0, MSTATUS_MPP;     \
    csrs mstatus, a0;

#define RVTEST_ENABLE_SUPERVISOR              \
    li   a0, MSTATUS_MPP &(MSTATUS_MPP >> 1); \
    csrs mstatus, a0;                         \
    li   a0, SIP_SSIP | SIP_STIP;             \
    csrs mideleg, a0;

// clang-format off
#define MEDELEG_MASK \
    ((1 << CAUSE_LOAD_PAGE_FAULT)  | \
     (1 << CAUSE_STORE_PAGE_FAULT) | \
     (1 << CAUSE_FETCH_PAGE_FAULT) | \
     (1 << CAUSE_MISALIGNED_FETCH) | \
     (1 << CAUSE_USER_ECALL)       | \
     (1 << CAUSE_BREAKPOINT))
// clang-format on

// clang-format off
#ifdef RVTEST_HAS_CSR
#define RVTEST_CSR_INIT       \
    la     t0, mtvec_handler; \
    csrw   mtvec, t0;         \
    /* if an stvec_handler is defined, delegate exceptions to it */ \
    la     t0, stvec_handler; \
    beqz   t0, 1f;            \
    csrw   stvec, t0;         \
    li     t0, MEDELEG_MASK;  \
    csrw   medeleg, t0;       \
1 :                           \
    init;                     \
    la t0, 1f;                \
    csrw   mepc, t0;          \
    mret;                     \
1:

// clang-format on
#else
#define RVTEST_CSR_INIT
#endif

// clang-format off
#define RVTEST_CODE_BEGIN \
    .section .text.init;  \
    .align 6;             \
    .globl _start;        \
    .weak  mtvec_handler; \
    .weak  stvec_handler; \
    _start:               \
    RVTEST_CSR_INIT
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

#define RVTEST_PASS \
    fence;          \
    li a0, 0;       \
    TEST_END

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
