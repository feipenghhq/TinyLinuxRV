// See LICENSE for license details.

#ifndef _ENV_PHYSICAL_SINGLE_CORE_H
#define _ENV_PHYSICAL_SINGLE_CORE_H

//-----------------------------------------------------------------------
// Begin Macro
//-----------------------------------------------------------------------

#define RVTEST_RV64U

#define RVTEST_CODE_BEGIN                                               \
        .section .text.init;                                            \
        .align  6;                                                      \
        .globl _start;                                                  \
_start:

//-----------------------------------------------------------------------
// End Macro
//-----------------------------------------------------------------------

#define RVTEST_CODE_END                                                 \
        unimp

//-----------------------------------------------------------------------
// Pass/Fail Macro
//-----------------------------------------------------------------------

#define RVTEST_PASS                                                     \
        fence;                                                          \
        li a0, 0;                                                       \
        ebreak

#define TESTNUM gp
#define RVTEST_FAIL                                                     \
        fence;                                                          \
        li a0, 1;                                                       \
        mv a1, TESTNUM;                                                 \
        ebreak

//-----------------------------------------------------------------------
// Data Section Macro
//-----------------------------------------------------------------------

#define EXTRA_DATA

#define RVTEST_DATA_BEGIN                                               \
        EXTRA_DATA                                                      \
        .align 4; .global begin_signature; begin_signature:

#define RVTEST_DATA_END .align 4; .global end_signature; end_signature:

#endif
