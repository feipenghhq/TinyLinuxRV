#include "cpu/csr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

// TODO: CSR_MCOUNTEREN: Need to implement for S/U mode
// TODO: MEDELEG/MIDELEG: Need to implement the deleg logic

// ----------------------------------------------
// Helper variable and macro
// ----------------------------------------------

enum {
    // Supervisor Trap Setup (SRW)
    CSR_SSTATUS    = 0x100,
    CSR_SIE        = 0x104,
    CSR_STVEC      = 0x105,
    CSR_SCOUNTEREN = 0x106,
    // Supervisor Counter Setup (SRW)
    CSR_SCOUNTINHIBIT = 0x120,
    // Supervisor Trap handling (SRW)
    CSR_SSCRATCH = 0x140,
    CSR_SEPC     = 0x141,
    CSR_SCAUSE   = 0x142,
    CSR_STVAL    = 0x143,
    CSR_SIP      = 0x144,
    // Supervisor Protection and Translation
    CSR_STAP = 0x180,
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
    // Machine Counter Setup (MRW)
    CSR_MCOUNTINHIBIT = 0x320,
    // Machine  Counter/Timer (MRW)
    CSR_MCYCLE   = 0xB00,
    CSR_MINSTRET = 0xB02,
    // Machine Information Registers (MRO)
    CSR_MVENDORID = 0xF11,
    CSR_MARCHID   = 0xF12,
    CSR_MIMPID    = 0xF13,
    CSR_MHARTID   = 0xF14,
    // Unprivileged Counter/Timer (URO)
    CSR_CYCLE   = 0xC00,
    CSR_TIME    = 0xC01,
    CSR_INSTRET = 0xC02,
} csr_addr;

// Mask for MSTATUS
#define MSTATUS_SIE  (1ULL << 1)
#define MSTATUS_MIE  (1ULL << 3)
#define MSTATUS_SPIE (1ULL << 5)
#define MSTATUS_MPIE (1ULL << 7)
#define MSTATUS_SPP  (1ULL << 8)
#define MSTATUS_MPP  (3ULL << 11)
#define MSTATUS_TSR  (1ULL << 22)
#define MSTATUS_UXL  (3ULL << 32)
#define MSTATUS_SXL  (3ULL << 34)

#define MSTATUS_READ_MASK \
    (MSTATUS_SIE | MSTATUS_MIE | MSTATUS_SPIE | MSTATUS_MPIE | MSTATUS_SPP | MSTATUS_MPP | MSTATUS_TSR | MSTATUS_UXL | MSTATUS_SXL)
#define MSTATUS_WRITE_MASK (MSTATUS_SIE | MSTATUS_MIE | MSTATUS_SPIE | MSTATUS_MPIE | MSTATUS_SPP | MSTATUS_MPP | MSTATUS_TSR)

#define SSTATUS_READ_MASK  (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP | MSTATUS_UXL | MSTATUS_SXL)
#define SSTATUS_WRITE_MASK (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP)

// Mask for SIP
#define MIP_SSIP (1ULL << 1)
#define MIP_MSIP (1ULL << 3)
#define MIP_STIP (1ULL << 5)
#define MIP_MTIP (1ULL << 7)
#define MIP_SEIP (1ULL << 9)
#define MIP_MEIP (1ULL << 11)

#define SIP_WRITE_MASK (MIP_SSIP | MIP_STIP | MIP_SEIP)
#define SIP_READ_MASK  (MIP_SSIP | MIP_STIP | MIP_SEIP)

// Mask for SIE
#define MIE_SSIE (1ULL << 1)
#define MIE_STIE (1ULL << 5)
#define MIE_SEIE (1ULL << 9)

#define SIE_WRITE_MASK (MIE_SSIE | MIE_STIE | MIE_SEIE)
#define SIE_READ_MASK  (MIE_SSIE | MIE_STIE | MIE_SEIE)

#define CSR_DECODE_RW(addr, name)     \
    case addr:                        \
        csr_reg = &csr->csr_reg.name; \
        break;

#define CSR_DECODE_RO(addr, name)       \
    case addr:                          \
        read_only = true;               \
        csr_reg   = &csr->csr_reg.name; \
        break;

// ----------------------------------------------
// Local helper function
// ----------------------------------------------

static inline uint64_t csr_field_get(uint64_t reg, int pos, int width) {
    uint64_t mask = (UINT64_C(1) << width) - 1;
    return (reg >> pos) & mask;
}

static inline void csr_field_set(uint64_t *reg, int pos, int width, uint64_t value) {
    uint64_t mask          = (UINT64_C(1) << width) - 1;
    uint64_t mask_shifted  = mask << pos;
    uint64_t value_shifted = value << pos;

    *reg = (*reg & ~mask_shifted) | (value_shifted & mask_shifted);
}

static void csr_default(csr_t *csr) {
    csr->csr_reg.misa = 0;
    // MISA
    // MXL field need to be set to 2 to indicate 64 bit ISA
    csr_field_set(&csr->csr_reg.misa, 62, 2, 2);
    // Supported ISA
    csr_field_set(&csr->csr_reg.misa, 0, 1, 1);  // A
    csr_field_set(&csr->csr_reg.misa, 8, 1, 1);  // I
    csr_field_set(&csr->csr_reg.misa, 12, 1, 1); // M
    // MSTATUS
    // UXL set to 2 to indicate 64 bit ISA
    csr_field_set(&csr->csr_reg.mstatus, 32, 2, 2);
    // SXL set to 2 to indicate 64 bit ISA
    csr_field_set(&csr->csr_reg.mstatus, 34, 2, 2);
}

// ----------------------------------------------
// Main function
// ----------------------------------------------

void csr_init(csr_t *csr) {
    memset(csr, 0, sizeof(*csr));
    // -- MISA --
    csr_default(csr);
}

int csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr,
               uint64_t time_value) {
    uint64_t *csr_reg       = NULL;
    bool      read_only     = false;
    bool      imp_read_only = false;
    uint64_t  write_value   = value;

    csr->csr_reg.mtime = time_value;

    switch (addr) {
        // Supervisor Trap Setup (SRW)
        CSR_DECODE_RW(CSR_SSTATUS, mstatus)
        CSR_DECODE_RW(CSR_SIE, mie)
        CSR_DECODE_RW(CSR_STVEC, stvec)
        CSR_DECODE_RW(CSR_SCOUNTEREN, scounteren)
        // Supervisor Counter Setup (SRW)
        CSR_DECODE_RW(CSR_SCOUNTINHIBIT, scountinhibit)
        // Supervisor Trap handling (SRW)
        CSR_DECODE_RW(CSR_SSCRATCH, sscratch)
        CSR_DECODE_RW(CSR_SEPC, sepc)
        CSR_DECODE_RW(CSR_SCAUSE, scause)
        CSR_DECODE_RW(CSR_STVAL, stval)
        CSR_DECODE_RW(CSR_SIP, mip)
        // Supervisor Protection and Translation
        CSR_DECODE_RW(CSR_STAP, stap)
        // Machine Trap Setup (MRW)
        CSR_DECODE_RW(CSR_MSTATUS, mstatus)
        CSR_DECODE_RW(CSR_MISA, misa)
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
        // Machine Counter Setup (MRW)
        CSR_DECODE_RW(CSR_MCOUNTINHIBIT, mcountinhibit)
        // Machine  Counter/Timer (MRW)
        CSR_DECODE_RW(CSR_MCYCLE, mcycle)
        CSR_DECODE_RW(CSR_MINSTRET, minstret)
        // Machine Information Registers (MRO)
        CSR_DECODE_RO(CSR_MVENDORID, mvendorid)
        CSR_DECODE_RO(CSR_MARCHID, marchid)
        CSR_DECODE_RO(CSR_MIMPID, mimpid)
        CSR_DECODE_RO(CSR_MHARTID, mhartid)
        // Unprivileged Counter/Timer (URO)
        CSR_DECODE_RO(CSR_CYCLE, mcycle)
        CSR_DECODE_RO(CSR_TIME, mtime)
        CSR_DECODE_RO(CSR_INSTRET, minstret)
    default: {
        LOG_ERROR("CPU: Access imp CSR register: %x", addr);
        return 1;
    }
    }

    // write to read only CSR. Should cause illegal instruction
    if (csr_reg && write_csr && read_only) {
        return 1;
    }

    // process read operation first
    if (csr_reg && read_csr) {
        switch (addr) {
        // special case for mstatus: Only read implemented field
        case CSR_MSTATUS:
            *rdata = *csr_reg & MSTATUS_READ_MASK;
            break;
        // special case for mcounteren: Only implement IR/TM/CY field
        case CSR_MCOUNTEREN:
            *rdata = *csr_reg & 0x7;
            break;
        // special case for mcountinhibit: Only implement IR/CY field
        case CSR_MCOUNTINHIBIT:
            *rdata = *csr_reg & 0x5;
            break;
        // special case for sstatus: sstatus is a subset of mstatus. so read should read from mstatus
        case CSR_SSTATUS:
            *rdata = csr->csr_reg.mstatus & SSTATUS_READ_MASK;
            break;
        // special case for sie: sie is a subset of mie. so read should read from mie
        case CSR_SIE:
            *rdata = csr->csr_reg.mie & SIE_READ_MASK;
            break;
        // special case for sip: sip is a subset of mip. so read should read from mip
        // !NOTE: the mxip portion is read only but sxip portion is write able, a true good implementation should
        // !      be or the mip with the actual interrupt signal. (Read the spec!)
        // !      but here we don't have supervisor specific hardware interrupt so it is OK.
        case CSR_SIP:
            *rdata = csr->csr_reg.mip & SIP_READ_MASK;
            break;
        default:
            *rdata = *csr_reg;
            break;
        }
    }

    // In our implementation, we make these CSR read only to Software
    switch (addr) {
    case CSR_MISA:
        imp_read_only = true;
    }

    // special handling for some csr for write operation
    switch (addr) {
    case CSR_MSTATUS:
        write_value = value & MSTATUS_WRITE_MASK;
        break;
    case CSR_MCYCLE:
        csr->mcycle_written = true;
        break;
    case CSR_MINSTRET:
        csr->minstret_written = true;
        break;
    // Special case for sstatus. sstatus is a subset of mstatus. So writing to sstatus should really be writing to
    // mstatus, and we should only write the the field that can be access by sstatus
    // Similar for sie and sip
    case CSR_SSTATUS:
        write_value = value & SSTATUS_WRITE_MASK;
        break;
    case CSR_SIE:
        write_value = value & SIE_WRITE_MASK;
        break;
    case CSR_SIP:
        write_value = value & SIP_WRITE_MASK;
        break;
    }

    // process write operation
    if (csr_reg && write_csr && !imp_read_only) {
        switch (op) {
        // swap the csr and the value
        case CSR_OP_RW: {
            *csr_reg = write_value;
            break;
        }
        // set the corresponding bit
        case CSR_OP_RS: {
            *csr_reg |= write_value;
            break;
        }
        // clear the corresponding bit
        case CSR_OP_RC: {
            *csr_reg &= ~write_value;
            break;
        }
        }
    }

    return 0;
}

uint64_t trap_enter(csr_t *csr, uint64_t cause, uint64_t mtval, uint64_t pc, priv_mode_t *priv) {
    bool     target_priv_is_M = true; // by default we should handle trap using M-mode
    uint64_t bit_mask;

    uint64_t trap_vec_base;
    uint64_t trap_vec_mode;
    uint64_t trap_vec;
    uint64_t mie, sie;
    uint64_t interrupt;
    uint64_t cause_code;
    // parse the cause
    interrupt  = csr_field_get(cause, 63, 1);
    cause_code = csr_field_get(cause, 0, 63);

    // check which privilege mode should handle the interrupt
    if (*priv != PRIV_M) { // S-mode/U-mode can be delegated by S-mode handler
        bit_mask = 1 << cause_code;
        if (interrupt) {
            if (csr->csr_reg.mideleg & bit_mask) {
                target_priv_is_M = false;
            }
        } else {
            if (csr->csr_reg.medeleg & bit_mask) {
                target_priv_is_M = false;
            }
        }
    }

    // trap is handled by M mode
    if (target_priv_is_M) {
        // update mstatus
        mie = csr_field_get(csr->csr_reg.mstatus, 3, 1);
        csr_field_set(&csr->csr_reg.mstatus, 7, 1, mie);    // MPIE = MIE
        csr_field_set(&csr->csr_reg.mstatus, 3, 1, 0);      // MIE = 0
        csr_field_set(&csr->csr_reg.mstatus, 11, 2, *priv); // MPP = privilege before trap
        // update mtval
        csr->csr_reg.mtval = mtval;
        // update mcause
        csr->csr_reg.mcause = cause;
        // update mepc
        // since we only support only IALIGN=32, the two low bits (mepc[1:0]) are always zero.
        csr->csr_reg.mepc = pc & ~UINT64_C(0x3);
        // return to address defined by mtvec
        trap_vec_base = csr->csr_reg.mtvec & ~UINT64_C(0x3);
        trap_vec_mode = csr->csr_reg.mtvec & 0x3;
        // the new priv is M mode
        *priv = PRIV_M;
    }
    // trap is handled by S mode
    else {
        // update mstatus (sstatus)
        sie = csr_field_get(csr->csr_reg.sstatus, 1, 1);
        csr_field_set(&csr->csr_reg.mstatus, 5, 1, sie);         // SPIE = SIE
        csr_field_set(&csr->csr_reg.mstatus, 1, 1, 0);           // SIE = 0
        csr_field_set(&csr->csr_reg.mstatus, 8, 1, *priv & 0x1); // SPP = privilege before trap
        // update mtval
        csr->csr_reg.stval = mtval;
        // update mcause
        csr->csr_reg.scause = cause;
        // update mepc
        // since we only support only IALIGN=32, the two low bits (mepc[1:0]) are always zero.
        csr->csr_reg.sepc = pc & ~UINT64_C(0x3);
        // return to address defined by mtvec
        trap_vec_base = csr->csr_reg.stvec & ~UINT64_C(0x3);
        trap_vec_mode = csr->csr_reg.stvec & 0x3;
        // the new priv is S mode
        *priv = PRIV_S;
    }

    // return the trap vector
    if (trap_vec_mode == 1 && interrupt) {
        trap_vec = trap_vec_base + 4 * (uint64_t)cause_code;
    } else {
        trap_vec = trap_vec_base;
    }
    return trap_vec;
}

uint64_t trap_exit_mret(csr_t *csr, priv_mode_t *priv) {
    uint64_t mpie;
    // restore the current privilege mode to MPP
    *priv = (priv_mode_t)csr_field_get(csr->csr_reg.mstatus, 11, 2);
    // update mstatus
    // - restore mie and set mpie to 1
    mpie = csr_field_get(csr->csr_reg.mstatus, 7, 1);
    csr_field_set(&csr->csr_reg.mstatus, 3, 1, mpie);
    csr_field_set(&csr->csr_reg.mstatus, 7, 1, 1);
    // - set MPP to 0.
    csr_field_set(&csr->csr_reg.mstatus, 11, 2, 0);
    // return mepc
    return csr->csr_reg.mepc & ~UINT64_C(0x3);
}

uint64_t trap_exit_sret(csr_t *csr, priv_mode_t *priv) {
    uint64_t spie;
    // restore the current privilege mode to SPP
    *priv = (priv_mode_t)csr_field_get(csr->csr_reg.mstatus, 8, 1);
    // update mstatus
    // - restore sie and set spie to 1
    spie = csr_field_get(csr->csr_reg.mstatus, 5, 1);
    csr_field_set(&csr->csr_reg.mstatus, 1, 1, spie);
    csr_field_set(&csr->csr_reg.mstatus, 5, 1, 1);
    // - set SPP to 0.
    csr_field_set(&csr->csr_reg.mstatus, 8, 1, 0);
    // return sepc
    return csr->csr_reg.sepc & ~UINT64_C(0x3);
}

// check if an interrupt is pending and should be handled by interrupt handler
static bool check_interrupt(csr_t *csr, priv_mode_t priv, int id) {
    uint64_t    intr;
    uint64_t    enable;
    uint64_t    delegate;
    uint64_t    fire = false;
    uint64_t    mstatus_mie, mstatus_sie;
    priv_mode_t target_priv = PRIV_M;

    intr        = csr_field_get(csr->csr_reg.mip, id, 1);
    enable      = csr_field_get(csr->csr_reg.mie, id, 1);
    delegate    = csr_field_get(csr->csr_reg.mideleg, id, 1);
    mstatus_mie = csr_field_get(csr->csr_reg.mstatus, 3, 1);
    mstatus_sie = csr_field_get(csr->csr_reg.mstatus, 1, 1);

    if ((priv != PRIV_M) && delegate) {
        target_priv = PRIV_S;
    }
    // if the current priv is smaller then target_priv, then there is no need to check mstatus.xie
    if (priv < target_priv) {
        fire = intr & enable;
    }
    // current mode == target mode, need to check mstatus.xie
    else {
        if (target_priv == PRIV_M) {
            fire = intr & enable & mstatus_mie;
        } else {
            fire = intr & enable & mstatus_sie;
        }
    }

    return fire;
}

bool interrupt_pending_and_enabled(csr_t *csr, priv_mode_t priv, interrupt_code_t *int_id) {
    // MEIP > MSIP > MTIP > SEIP > SSIP > STIP
    int interrupt_id[] = {INT_MEIP, INT_MSIP, INT_MTIP, INT_SEIP, INT_SSIP, INT_STIP};
    for (int i = 0; i < 6; i++) {
        int id = interrupt_id[i];
        if (check_interrupt(csr, priv, id)) {
            *int_id = (interrupt_code_t)id;
            return true;
        }
    }
    return false;
}

// used to wake up WFI
bool interrupt_pending(csr_t *csr) {
    return (csr->csr_reg.mip != 0);
}

void csr_begin_update(csr_t *csr, bool eip, bool sip, bool tip) {
    // update csr internal status at the beginning of execution
    csr->mcycle_written         = false;
    csr->minstret_written       = false;
    csr->starting_mcountinhibit = csr->csr_reg.mcountinhibit;

    // Update mip register
    csr_field_set(&csr->csr_reg.mip, INT_MEIP, 1, eip);
    csr_field_set(&csr->csr_reg.mip, INT_MSIP, 1, sip);
    csr_field_set(&csr->csr_reg.mip, INT_MTIP, 1, tip);
}

void csr_end_update(csr_t *csr, bool retired) {

    if (!csr_field_get(csr->starting_mcountinhibit, 2, 1) && !csr->minstret_written && retired) {
        csr->csr_reg.minstret++;
    }

    if (!csr_field_get(csr->starting_mcountinhibit, 0, 1) && !csr->mcycle_written) {
        // assuming each instruction takes 1 clock for the emulator
        csr->csr_reg.mcycle++;
    }
    csr->minstret_written = false;
    csr->mcycle_written   = false;
}

// check if sret will trap or not
bool check_sret_trap(csr_t *csr, priv_mode_t priv) {
    return (priv == PRIV_S) && (csr->csr_reg.mstatus & MSTATUS_TSR);
}
