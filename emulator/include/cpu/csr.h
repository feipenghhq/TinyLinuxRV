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

void csr_init(csr_t *csr);
int csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr);

#endif
