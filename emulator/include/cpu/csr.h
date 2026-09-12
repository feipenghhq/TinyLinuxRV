#ifndef CSR_H
#define CSR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CSR_OP_RW = 1,
    CSR_OP_RS = 2,
    CSR_OP_RC = 3,
} csr_op_t;

typedef struct {
    // Machine Mode
    uint64_t mstatus;
    uint64_t misa;
    uint64_t medeleg;
    uint64_t mideleg;
    uint64_t mie;
    uint64_t mtvec;
    uint64_t mcounteren;
    uint64_t mscratch;
    uint64_t mepc;
    uint64_t mcause;
    uint64_t mtval;
    uint64_t mip;
    uint64_t mvendorid;
    uint64_t marchid;
    uint64_t mimpid;
    uint64_t mhartid;
    // counter CSR
    // TBD

} csr_t;

typedef enum {
    INST_ADDR_MISALIGNED       = 0,
    INST_ACCESS_FAULT          = 1,
    ILLEGAL_INSTRUCTION        = 2,
    BREAKPOINT                 = 3,
    LOAD_ADDR_MISALIGNED       = 4,
    LOAD_ACCESS_FAULT          = 5,
    STORE_AMO_ADDR_MISALIGNED  = 6,
    STORE_AMO_ACCESS_FAULT     = 7,
    ECALL_FROM_U_MODE          = 8,
    ECALL_FROM_S_MODE          = 9,
    ECALL_FROM_M_MODE          = 11,
    INST_PAGE_FAULT            = 12,
    LOAD_PAGE_FAULT            = 13,
    STORE_AMO_PAGE_FAULT       = 15,
} exception_code_t;

void     csr_init(csr_t *csr);
int      csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr);
uint64_t trap_enter(csr_t *csr, uint64_t cause, uint64_t mtval, uint64_t pc);
uint64_t trap_exit(csr_t *csr);

#endif
