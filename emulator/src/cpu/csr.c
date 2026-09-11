#include "cpu/csr.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

// ----------------------------------------------
// Helper variable and macro
// ----------------------------------------------

enum {
    // Machine Trap Setup (MRW)
    CSR_MSTATUS    = 0x300,
    CSR_MISA       = 0x301,
    CSR_MEDELEG    = 0x302,
    CSR_MIDELEG    = 0x303,
    CSR_MIE        = 0x304,
    CSR_MTVEC      = 0x305,
    CSR_MCOUNTEREN = 0x306,
    // Machine Trap Handling (MRW)
    CSR_MSCRATCH = 0x340,
    CSR_MEPC     = 0x341,
    CSR_MCAUSE   = 0x342,
    CSR_MTVAL    = 0x343,
    CSR_MIP      = 0x344,
    // Machine Information Registers (MRO)
    CSR_MVENDORID = 0xF11,
    CSR_MARCHID   = 0xF12,
    CSR_MIMPID    = 0xF13,
    CSR_MHARTID   = 0xF14,
}

csr_addr;

#define CSR_DECODE_RW(addr, name) \
    case addr: {                  \
        csr_reg = &csr->name;     \
        *rdata  = csr->name;      \
        break;                    \
    }

#define CSR_DECODE_RO(addr, name) \
    case addr: {                  \
        *rdata = csr->name;       \
        break;                    \
    }

// special case for MSTATUS
// - MPP field always return 3 as we only implemented M mode right now
#define CSR_DECODE_RW_MSTATUS(addr, name) \
    case addr: {                          \
        csr_reg = &csr->name;             \
        *rdata  = csr->name | 0x1800;     \
        break;                            \
    }

static inline uint64_t csr_field_get(uint64_t *csr, int pos, int width) {
    uint64_t mask = (1U << width) - 1;
    return (*csr >> pos) & mask;
}

static inline void csr_field_set(uint64_t *csr, int pos, int width, uint64_t value) {
    uint64_t mask          = (1U << width) - 1;
    uint64_t mask_shifted  = mask << pos;
    uint64_t value_shifted = value << pos;

    *csr = (*csr & ~mask_shifted) | value_shifted;
}

// Note: Other special case not implemented
// - mepc: mepc[0] is always read as 0

// ----------------------------------------------
// Main function
// ----------------------------------------------

void csr_init(csr_t *csr) {
    memset(csr, 0, sizeof(*csr));
    // -- MISA --
    // MXL field need to be set to 2 to indicate 64 bit ISA
    csr_field_set(&csr->misa, 62, 2, 2);
    csr_field_set(&csr->misa, 8, 1, 1);
    csr_field_set(&csr->misa, 12, 1, 1);
    // -- MSTATUS --
    // MPP field should be set to 3 to indicate starting at Machine Mode.
}

int csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr) {
    uint64_t *csr_reg = NULL;

    switch (addr) {
        // Machine Trap Setup (MRW)
        CSR_DECODE_RW_MSTATUS(CSR_MSTATUS, mstatus)
        CSR_DECODE_RO(CSR_MISA, misa)   // make MISA RO
        CSR_DECODE_RW(CSR_MEDELEG, medeleg)
        CSR_DECODE_RW(CSR_MIDELEG, mideleg)
        CSR_DECODE_RW(CSR_MIE, mie)
        CSR_DECODE_RW(CSR_MTVEC, mtvec)
        // Machine Trap Handling (MRW)
        CSR_DECODE_RW(CSR_MCOUNTEREN, mcounteren)
        CSR_DECODE_RW(CSR_MSCRATCH, mscratch)
        CSR_DECODE_RW(CSR_MEPC, mepc)
        CSR_DECODE_RW(CSR_MCAUSE, mcause)
        CSR_DECODE_RW(CSR_MTVAL, mtval)
        CSR_DECODE_RW(CSR_MIP, mip)
        // Machine Information Registers (MRO)
        CSR_DECODE_RO(CSR_MVENDORID, mvendorid)
        CSR_DECODE_RO(CSR_MARCHID, marchid)
        CSR_DECODE_RO(CSR_MIMPID, mimpid)
        CSR_DECODE_RO(CSR_MHARTID, mhartid)
    default: {
        LOG_ERROR("CPU: Access imp CSR register: %x", addr);
        return 1;
    }
    }

    if (csr_reg && write_csr) {
        switch (op) {
        // swap the csr and the value
        case CSR_OP_RW: {
            *csr_reg = value;
            break;
        }
        // set the corresponding bit
        case CSR_OP_RS: {
            *csr_reg |= value;
            break;
        }
        // clear the corresponding bit
        case CSR_OP_RC: {
            *csr_reg &= ~value;
            break;
        }
        default: {
            LOG_ERROR("CPU: Unsupported CSR operation");
            return 1;
        }
        }
    }
    return 0;
}

uint64_t trap_enter(csr_t *csr, uint64_t cause, uint64_t mtval, uint64_t pc) {
    uint64_t trap_vec_base;
    uint64_t trap_vec_mode;
    uint64_t trap_vec;
    uint64_t mie;

    // update mstatus
    // Currently only support machine mode
    // - MPIE is updated with MIE
    // - MIE is updated to 0
    // - MPP is updated to the privilege mode before the trap happens
    mie = csr_field_get(&csr->mstatus, 3, 1);
    csr_field_set(&csr->mstatus, 7, 1, mie);
    csr_field_set(&csr->mstatus, 3, 1, 0);
    csr_field_set(&csr->mstatus, 11, 2, 3);

    // update mtval
    csr->mtval = mtval;

    // update mcause
    csr->mcause = cause;

    // update mepc
    csr->mepc = pc;

    // return the trap vector
    trap_vec_base = csr->mtvec & ~0x3U;
    trap_vec_mode = csr->mtvec & 0x3;
    if (trap_vec_mode == 0) {
        trap_vec = trap_vec_base;
    } else {
        trap_vec = trap_vec_base + 4 * (uint64_t)cause;
    }
    return trap_vec;
}

uint64_t trap_exit(csr_t *csr) {
    uint64_t mpie;

    // update mstatus
    // - restore mie and set mpie to 1
    mpie = csr_field_get(&csr->mstatus, 7, 1);
    csr_field_set(&csr->mstatus, 3, 1, mpie);
    csr_field_set(&csr->mstatus, 7, 1, 1);
    // - set MPP to 0. For now set it to 3 as we only support M mode
    csr_field_set(&csr->mstatus, 11, 2, 3);

    // return mepc
    return csr->mepc;
}
