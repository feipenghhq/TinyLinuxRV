#ifndef RISCV_H
#define RISCV_H

typedef enum {
    PRIV_U = 0,
    PRIV_S = 1,
    PRIV_M = 3,
} priv_mode_t;

typedef enum {
    INST_ADDR_MISALIGNED      = 0,
    INST_ACCESS_FAULT         = 1,
    ILLEGAL_INSTRUCTION       = 2,
    BREAKPOINT                = 3,
    LOAD_ADDR_MISALIGNED      = 4,
    LOAD_ACCESS_FAULT         = 5,
    STORE_AMO_ADDR_MISALIGNED = 6,
    STORE_AMO_ACCESS_FAULT    = 7,
    ECALL_FROM_U_MODE         = 8,
    ECALL_FROM_S_MODE         = 9,
    ECALL_FROM_M_MODE         = 11,
    INST_PAGE_FAULT           = 12,
    LOAD_PAGE_FAULT           = 13,
    STORE_AMO_PAGE_FAULT      = 15,
} exception_code_t;

typedef enum {
    INT_USIP = 0,
    INT_SSIP = 1,
    INT_MSIP = 3,
    INT_UTIP = 4,
    INT_STIP = 5,
    INT_MTIP = 7,
    INT_UEIP = 8,
    INT_SEIP = 9,
    INT_MEIP = 11,
} interrupt_code_t;

#endif
