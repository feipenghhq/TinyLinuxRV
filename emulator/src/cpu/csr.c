#include "cpu/csr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

// ----------------------------------------------
// Helper variable and macro
// ----------------------------------------------

// CSR address encoding
enum {
    CSR_SSTATUS       = 0x100,
    CSR_SIE           = 0x104,
    CSR_STVEC         = 0x105,
    CSR_SCOUNTEREN    = 0x106,
    CSR_SSCRATCH      = 0x140,
    CSR_SEPC          = 0x141,
    CSR_SCAUSE        = 0x142,
    CSR_STVAL         = 0x143,
    CSR_SIP           = 0x144,
    CSR_SATP          = 0x180,
    CSR_MSTATUS       = 0x300,
    CSR_MISA          = 0x301,
    CSR_MEDELEG       = 0x302,
    CSR_MIDELEG       = 0x303,
    CSR_MIE           = 0x304,
    CSR_MTVEC         = 0x305,
    CSR_MCOUNTEREN    = 0x306,
    CSR_MSCRATCH      = 0x340,
    CSR_MEPC          = 0x341,
    CSR_MCAUSE        = 0x342,
    CSR_MTVAL         = 0x343,
    CSR_MIP           = 0x344,
    CSR_MCOUNTINHIBIT = 0x320,
    CSR_MCYCLE        = 0xB00,
    CSR_MINSTRET      = 0xB02,
    CSR_MVENDORID     = 0xF11,
    CSR_MARCHID       = 0xF12,
    CSR_MIMPID        = 0xF13,
    CSR_MHARTID       = 0xF14,
    CSR_CYCLE         = 0xC00,
    CSR_TIME          = 0xC01,
    CSR_INSTRET       = 0xC02,
} csr_addr;

#define CSR_BIT(pos)               (UINT64_C(1) << (pos))
#define CSR_FIELD_MASK(pos, width) (((UINT64_C(1) << (width)) - 1) << (pos))

// mstatus fields
#define MSTATUS_SIE_POS  1
#define MSTATUS_MIE_POS  3
#define MSTATUS_SPIE_POS 5
#define MSTATUS_MPIE_POS 7
#define MSTATUS_SPP_POS  8
#define MSTATUS_MPP_POS  11
#define MSTATUS_TW_POS   21
#define MSTATUS_TSR_POS  22
#define MSTATUS_UXL_POS  32
#define MSTATUS_SXL_POS  34

#define MSTATUS_SIE  CSR_BIT(MSTATUS_SIE_POS)
#define MSTATUS_MIE  CSR_BIT(MSTATUS_MIE_POS)
#define MSTATUS_SPIE CSR_BIT(MSTATUS_SPIE_POS)
#define MSTATUS_MPIE CSR_BIT(MSTATUS_MPIE_POS)
#define MSTATUS_SPP  CSR_BIT(MSTATUS_SPP_POS)
#define MSTATUS_MPP  CSR_FIELD_MASK(MSTATUS_MPP_POS, 2)
#define MSTATUS_TW   CSR_BIT(MSTATUS_TW_POS)
#define MSTATUS_TSR  CSR_BIT(MSTATUS_TSR_POS)
#define MSTATUS_UXL  CSR_FIELD_MASK(MSTATUS_UXL_POS, 2)
#define MSTATUS_SXL  CSR_FIELD_MASK(MSTATUS_SXL_POS, 2)

#define MSTATUS_WRITE_MASK \
    (MSTATUS_SIE | MSTATUS_MIE | MSTATUS_SPIE | MSTATUS_MPIE | MSTATUS_SPP | MSTATUS_MPP | MSTATUS_TW | MSTATUS_TSR)

#define SSTATUS_READ_MASK  (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP | MSTATUS_UXL)
#define SSTATUS_WRITE_MASK (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP)

// Interrupt fields
#define INT_BIT_SSIP CSR_BIT(INT_SSIP)
#define INT_BIT_MSIP CSR_BIT(INT_MSIP)
#define INT_BIT_STIP CSR_BIT(INT_STIP)
#define INT_BIT_MTIP CSR_BIT(INT_MTIP)
#define INT_BIT_SEIP CSR_BIT(INT_SEIP)
#define INT_BIT_MEIP CSR_BIT(INT_MEIP)

#define INT_S_MASK   (INT_BIT_SSIP | INT_BIT_STIP | INT_BIT_SEIP)
#define INT_M_MASK   (INT_BIT_MSIP | INT_BIT_MTIP | INT_BIT_MEIP)
#define INT_ALL_MASK (INT_S_MASK | INT_M_MASK)

// mip: meip, mtip, msip are all read only in mip
#define MIP_WRITE_MASK INT_S_MASK

// sip: seip, stip are read only in sip
#define SIP_WRITE_MASK INT_BIT_SSIP
#define SIP_READ_MASK  INT_S_MASK

// mie
#define MIE_WRITE_MASK INT_ALL_MASK

// sie
#define SIE_WRITE_MASK INT_S_MASK
#define SIE_READ_MASK  INT_S_MASK

// mideleg
#define MIDELEG_WRITE_MASK INT_S_MASK

// medeleg field
#define EXC_INST_ADDR_MISALIGNED  0
#define EXC_INST_ACCESS_FAULT     1
#define EXC_ILLEGAL_INST          2
#define EXC_BREAKPOINT            3
#define EXC_LOAD_ADDR_MISALIGNED  4
#define EXC_LOAD_ACCESS_FAULT     5
#define EXC_STORE_ADDR_MISALIGNED 6
#define EXC_STORE_ACCESS_FAULT    7
#define EXC_ECALL_U               8
#define EXC_ECALL_S               9
#define EXC_ECALL_M               11
#define EXC_INST_PAGE_FAULT       12
#define EXC_LOAD_PAGE_FAULT       13
#define EXC_STORE_PAGE_FAULT      15

#define MEDELEG_WRITE_MASK                                                                            \
    (CSR_BIT(EXC_INST_ADDR_MISALIGNED) | CSR_BIT(EXC_INST_ACCESS_FAULT) | CSR_BIT(EXC_ILLEGAL_INST) | \
     CSR_BIT(EXC_BREAKPOINT) | CSR_BIT(EXC_LOAD_ADDR_MISALIGNED) | CSR_BIT(EXC_LOAD_ACCESS_FAULT) |   \
     CSR_BIT(EXC_STORE_ADDR_MISALIGNED) | CSR_BIT(EXC_STORE_ACCESS_FAULT) | CSR_BIT(EXC_ECALL_U) |    \
     CSR_BIT(EXC_ECALL_S))

// misa fields
#define MISA_EXT_A_POS 0
#define MISA_EXT_I_POS 8
#define MISA_EXT_M_POS 12
#define MISA_EXT_S_POS 18
#define MISA_EXT_U_POS 20
#define MISA_MXL_POS   62
#define XLEN_64        2

// mtvec fields
#define MTVEC_MODE_POS 0

// Counter enable/inhibit fields
#define COUNTER_CY_POS 0
#define COUNTER_TM_POS 1
#define COUNTER_IR_POS 2

#define COUNTEREN_WRITE_MASK     (CSR_BIT(COUNTER_CY_POS) | CSR_BIT(COUNTER_TM_POS) | CSR_BIT(COUNTER_IR_POS))
#define MCOUNTINHIBIT_WRITE_MASK (CSR_BIT(COUNTER_CY_POS) | CSR_BIT(COUNTER_IR_POS))

// CSR address encoding
#define CSR_ADDR_PRIV_POS   8
#define CSR_ADDR_PRIV_WIDTH 2

// Trap cause and vector fields
#define TRAP_CAUSE_INTERRUPT_POS 63
#define TRAP_CAUSE_CODE_POS      0
#define TRAP_CAUSE_CODE_WIDTH    63

#define TRAP_VECTOR_MODE_MASK     UINT64_C(0x3)
#define TRAP_VECTOR_MODE_VECTORED 1
#define TRAP_VECTOR_ENTRY_SIZE    4

#define TRAP_PC_ALIGN_MASK (~UINT64_C(0x3))

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

/**
 * Set default CSR value to indicate desired configuration.
 */
static void csr_default(csr_t *csr) {
    csr->csr_reg.misa = 0;
    // misa
    csr_field_set(&csr->csr_reg.misa, MISA_EXT_A_POS, 1, 1); // Atomic extension
    csr_field_set(&csr->csr_reg.misa, MISA_EXT_I_POS, 1, 1); // RV32I/64I base ISA
    csr_field_set(&csr->csr_reg.misa, MISA_EXT_M_POS, 1, 1); // Integer Multiply/Divide extension
    csr_field_set(&csr->csr_reg.misa, MISA_EXT_S_POS, 1, 1); // Supervisor mode implemented
    csr_field_set(&csr->csr_reg.misa, MISA_EXT_U_POS, 1, 1); // User mode implemented
    csr_field_set(&csr->csr_reg.misa, MISA_MXL_POS, 2, XLEN_64);
    // mstatus
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_UXL_POS, 2, XLEN_64);
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SXL_POS, 2, XLEN_64);
}

/**
 * Check whether the specified interrupt is pending and can be taken.
 *
 * - Delegation determines the privilege mode that handles the interrupt. A delegated interrupt is handled by S-mode
 *   and is masked while the CPU is running in a higher privilege mode.(M-mode)
 * - mstatus.xIE controls the global interrupt enable when the current privilege mode is the same as the interrupt
 *   target privilege. If the target privilege is higher than the current privilege, xIE does not need to be checked.
 * - An interrupt is masked if the current privilege mode is higher than the interrupt target privilege.
 */
static bool check_interrupt(csr_t *csr, priv_mode_t priv, int id) {
    uint64_t    intr;
    uint64_t    enable;
    uint64_t    delegate;
    uint64_t    fire = false;
    uint64_t    mstatus_mie, mstatus_sie;
    priv_mode_t target_priv;

    intr        = csr_field_get(csr->csr_reg.mip, id, 1);
    enable      = csr_field_get(csr->csr_reg.mie, id, 1);
    delegate    = csr_field_get(csr->csr_reg.mideleg, id, 1);
    mstatus_mie = csr_field_get(csr->csr_reg.mstatus, MSTATUS_MIE_POS, 1);
    mstatus_sie = csr_field_get(csr->csr_reg.mstatus, MSTATUS_SIE_POS, 1);
    // Interrupt delegation
    target_priv = delegate ? PRIV_S : PRIV_M;
    // If the target privilege is higher than the current privilege, xIE does not need to be checked.
    if (priv < target_priv) {
        fire = intr && enable;
    }
    // If the target privilege = current privilege, xIE need to be checked.
    else if (priv == target_priv) {
        fire = intr && enable && (target_priv == PRIV_M ? mstatus_mie : mstatus_sie);
    }
    // An interrupt is masked if the current privilege mode is higher than the interrupt target privilege
    else {
        fire = false;
    }
    return fire;
}

/**
 * mcounteren and scounteren control access to counter CSRs from lower privilege modes.
 * - S-mode access is controlled by mcounteren.
 * - U-mode access is controlled by both mcounteren and scounteren.
 * If access to a counter is disabled, reading the corresponding counter CSR raises an illegal instruction exception.
 */
static bool check_counteren(csr_t *csr, int pos, priv_mode_t priv) {
    int enable;

    assert(pos == COUNTER_CY_POS || pos == COUNTER_TM_POS || pos == COUNTER_IR_POS);

    // mcounteren might prevent S/U-mode access
    enable = (int)csr_field_get(csr->csr_reg.mcounteren, pos, 1);
    if ((priv == PRIV_S || priv == PRIV_U) && !enable) {
        return false;
    }
    // scounteren might prevent U-mode access
    enable = (int)csr_field_get(csr->csr_reg.scounteren, pos, 1);
    if (priv == PRIV_U && !enable) {
        return false;
    }
    return true;
}

// ----------------------------------------------
// Main function
// ----------------------------------------------

/**
 * Reset the CSR register
 */
void csr_init(csr_t *csr) {
    memset(csr, 0, sizeof(*csr));
    csr_default(csr);
}

/**
 * API for CPU to read and write CSR register
 */
int csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr,
               uint64_t time_value, priv_mode_t priv) {
    uint64_t *csr_reg       = NULL;
    bool      read_only     = false;
    bool      imp_read_only = false;
    uint64_t  write_value   = value;
    uint64_t  csr_non_writable;
    int       addr_priv;

    // Update the mtime shadow copy with the actual mtime value from CLINT.
    csr->csr_reg.mtime = time_value;

    // Check the privilege. Lower privilege can't access CSR that are only accessible by higher privilege
    addr_priv = (int)csr_field_get((uint64_t)addr, CSR_ADDR_PRIV_POS, CSR_ADDR_PRIV_WIDTH);
    // U-mode can only access User-level CSR
    if ((priv == PRIV_U) && (addr_priv > PRIV_U)) {
        return 1;
    }
    // S-mode can only access Supervisor/User-level CSR
    if ((priv == PRIV_S) && (addr_priv > PRIV_S)) {
        return 1;
    }

    // Decode the address
    switch (addr) {
        CSR_DECODE_RW(CSR_SSTATUS, mstatus)
        CSR_DECODE_RW(CSR_SIE, mie)
        CSR_DECODE_RW(CSR_STVEC, stvec)
        CSR_DECODE_RW(CSR_SCOUNTEREN, scounteren)
        CSR_DECODE_RW(CSR_SSCRATCH, sscratch)
        CSR_DECODE_RW(CSR_SEPC, sepc)
        CSR_DECODE_RW(CSR_SCAUSE, scause)
        CSR_DECODE_RW(CSR_STVAL, stval)
        CSR_DECODE_RW(CSR_SIP, mip)
        CSR_DECODE_RW(CSR_SATP, satp)
        CSR_DECODE_RW(CSR_MSTATUS, mstatus)
        CSR_DECODE_RW(CSR_MISA, misa)
        CSR_DECODE_RW(CSR_MEDELEG, medeleg)
        CSR_DECODE_RW(CSR_MIDELEG, mideleg)
        CSR_DECODE_RW(CSR_MIE, mie)
        CSR_DECODE_RW(CSR_MTVEC, mtvec)
        CSR_DECODE_RW(CSR_MCOUNTEREN, mcounteren)
        CSR_DECODE_RW(CSR_MSCRATCH, mscratch)
        CSR_DECODE_RW(CSR_MEPC, mepc)
        CSR_DECODE_RW(CSR_MCAUSE, mcause)
        CSR_DECODE_RW(CSR_MTVAL, mtval)
        CSR_DECODE_RW(CSR_MIP, mip)
        CSR_DECODE_RW(CSR_MCOUNTINHIBIT, mcountinhibit)
        CSR_DECODE_RW(CSR_MCYCLE, mcycle)
        CSR_DECODE_RW(CSR_MINSTRET, minstret)
        CSR_DECODE_RO(CSR_MVENDORID, mvendorid)
        CSR_DECODE_RO(CSR_MARCHID, marchid)
        CSR_DECODE_RO(CSR_MIMPID, mimpid)
        CSR_DECODE_RO(CSR_MHARTID, mhartid)
        CSR_DECODE_RO(CSR_CYCLE, mcycle)
        CSR_DECODE_RO(CSR_TIME, mtime)
        CSR_DECODE_RO(CSR_INSTRET, minstret)
    default: {
        LOG_ERROR("CPU: Access unimplemented CSR register: %x", addr);
        return 1;
    }
    }

    // Write to read only CSR. Should cause illegal instruction
    if (write_csr && read_only) {
        return 1;
    }

    // Process read operation first
    if (read_csr) {
        switch (addr) {
        case CSR_SSTATUS: // sstatus is a subset of mstatus so read should read from mstatus
            *rdata = csr->csr_reg.mstatus & SSTATUS_READ_MASK;
            break;
        case CSR_SIE: // sie is a subset of mie so read should read from mie. It is also affected by mideleg
            *rdata = csr->csr_reg.mie & SIE_READ_MASK & csr->csr_reg.mideleg;
            break;
        // special case for sip: sip is a subset of mip. so read should read from mip. It is also affected by mideleg
        // !NOTE: the mxip portion is read only but sxip portion is write able, a true good implementation should
        // !      be or the mip with the actual interrupt signal. (Read the spec!)
        // !      but here we don't have supervisor specific hardware interrupt so it is OK.
        case CSR_SIP:
            *rdata = csr->csr_reg.mip & SIP_READ_MASK & csr->csr_reg.mideleg;
            break;
        case CSR_CYCLE:
            if (check_counteren(csr, COUNTER_CY_POS, priv)) {
                *rdata = *csr_reg;
                break;
            } else {
                return 1;
            }
        case CSR_TIME:
            if (check_counteren(csr, COUNTER_TM_POS, priv)) {
                *rdata = *csr_reg;
                break;
            } else {
                return 1;
            }
        case CSR_INSTRET:
            if (check_counteren(csr, COUNTER_IR_POS, priv)) {
                *rdata = *csr_reg;
                break;
            } else {
                return 1;
            }
        default:
            *rdata = *csr_reg;
            break;
        }
    }

    // In our implementation, we make these CSR read only to Software
    switch (addr) {
    case CSR_MISA:
    case CSR_SATP:
        imp_read_only = true;
    }

    // process write operation
    if (csr_reg && write_csr && !imp_read_only) {

        // Special handling for some csr for write operation.
        // Mask out the non writable/unimplemented CSR field
        csr_non_writable = 0;
        switch (addr) {
        case CSR_MSTATUS:
            write_value      = value & MSTATUS_WRITE_MASK;
            csr_non_writable = *csr_reg & ~MSTATUS_WRITE_MASK;
            break;
        case CSR_MIE:
            write_value      = value & MIE_WRITE_MASK;
            csr_non_writable = *csr_reg & ~MIE_WRITE_MASK;
            break;
        case CSR_MIP:
            write_value      = value & MIP_WRITE_MASK;
            csr_non_writable = *csr_reg & ~MIP_WRITE_MASK;
            break;
        case CSR_MEDELEG:
            write_value      = value & MEDELEG_WRITE_MASK;
            csr_non_writable = *csr_reg & ~MEDELEG_WRITE_MASK;
            break;
        case CSR_MIDELEG:
            write_value      = value & MIDELEG_WRITE_MASK;
            csr_non_writable = *csr_reg & ~MIDELEG_WRITE_MASK;
            break;
        case CSR_MCYCLE:
            csr->mcycle_written = true;
            break;
        case CSR_MINSTRET:
            csr->minstret_written = true;
            break;
        case CSR_SCOUNTEREN:
        case CSR_MCOUNTEREN:
            write_value      = value & COUNTEREN_WRITE_MASK;
            csr_non_writable = *csr_reg & ~COUNTEREN_WRITE_MASK;
            break;
        case CSR_MCOUNTINHIBIT:
            write_value      = value & MCOUNTINHIBIT_WRITE_MASK;
            csr_non_writable = *csr_reg & ~MCOUNTINHIBIT_WRITE_MASK;
            break;
        case CSR_SSTATUS:
            write_value      = value & SSTATUS_WRITE_MASK;
            csr_non_writable = *csr_reg & ~SSTATUS_WRITE_MASK;
            break;
        case CSR_SIE:
            write_value      = value & SIE_WRITE_MASK & csr->csr_reg.mideleg;
            csr_non_writable = *csr_reg & ~(SIE_WRITE_MASK & csr->csr_reg.mideleg);
            break;
        case CSR_SIP:
            write_value      = value & SIP_WRITE_MASK & csr->csr_reg.mideleg;
            csr_non_writable = *csr_reg & ~(SIP_WRITE_MASK & csr->csr_reg.mideleg);
            break;
        }

        // the actual write operation
        switch (op) {
        case CSR_OP_RW: { // swap the csr and the value
            *csr_reg = csr_non_writable | write_value;
            break;
        }
        case CSR_OP_RS: { // set the corresponding bit
            *csr_reg = *csr_reg | write_value;
            break;
        }
        case CSR_OP_RC: { // clear the corresponding bit
            *csr_reg = (*csr_reg & ~write_value) | csr_non_writable;
            break;
        }
        }

        // post handling to make sure some field obay WARL
        switch (addr) {
        case CSR_MSTATUS: { // MPP can't be 2, if software write MPP as 2, change it to 0
            int mpp = (int)csr_field_get(*csr_reg, MSTATUS_MPP_POS, 2);
            if (mpp == 2) {
                csr_field_set(csr_reg, MSTATUS_MPP_POS, 2, 0);
            }
            break;
        }
        case CSR_MTVEC:
        case CSR_STVEC: { // mtvec.MODE/stvec.MODE can't be 2/3
            int mode = (int)csr_field_get(*csr_reg, MTVEC_MODE_POS, 2);
            if (mode == 2 || mode == 3) {
                csr_field_set(csr_reg, MTVEC_MODE_POS, 2, 0);
            }
            break;
        }
        case CSR_SEPC:
        case CSR_MEPC:
            *csr_reg = *csr_reg & TRAP_PC_ALIGN_MASK;
            break;
        }
    }

    return 0;
}

/**
 * Entering the trap
 * - determine which privilege mode will handle the trap
 * - update m(s)status, m(s)tval, m(s)cause, m(s)epc
 *   - in m(s)status: XPIE = XIE, XIE=0, XPP=previous privilege mode
 * - set pc to address defined in m(s)tvec
 */
uint64_t trap_enter(csr_t *csr, uint64_t cause, uint64_t tval, uint64_t pc, priv_mode_t *priv) {
    priv_mode_t target_priv;

    uint64_t pos_mask;
    uint64_t trap_vec_base;
    uint64_t trap_vec_mode;
    uint64_t trap_vec;
    uint64_t mstatus_mie, mstatus_sie;
    uint64_t interrupt;
    uint64_t cause_code;

    // parse the cause
    interrupt  = csr_field_get(cause, TRAP_CAUSE_INTERRUPT_POS, 1);
    cause_code = csr_field_get(cause, TRAP_CAUSE_CODE_POS, TRAP_CAUSE_CODE_WIDTH);

    // Check which privilege mode should handle the trap.
    // For exception raised in S/U mode, if the corresponding bit in medeleg is set, then it is delegated to S-mode
    // For interrupts raised in M mode, if it is delegated to S mode, than it will not trigger trap enter
    // For interrupts raised in S/U mode, if the corresponding bit in mideleg is set, then it is delegated to S-mode
    target_priv = PRIV_M;
    if (*priv < PRIV_M) {
        pos_mask = CSR_BIT(cause_code);
        if (interrupt) {
            if (csr->csr_reg.mideleg & pos_mask) {
                target_priv = PRIV_S;
            }
        } else {
            if (csr->csr_reg.medeleg & pos_mask) {
                target_priv = PRIV_S;
            }
        }
    }

    // for trap handled by M mode
    if (target_priv == PRIV_M) {
        // update mstatus
        mstatus_mie = csr_field_get(csr->csr_reg.mstatus, MSTATUS_MIE_POS, 1);
        csr_field_set(&csr->csr_reg.mstatus, MSTATUS_MPIE_POS, 1, mstatus_mie); // MPIE = MIE
        csr_field_set(&csr->csr_reg.mstatus, MSTATUS_MIE_POS, 1, 0);            // MIE = 0
        csr_field_set(&csr->csr_reg.mstatus, MSTATUS_MPP_POS, 2, *priv);        // MPP = previous privilege
        // update mtval
        csr->csr_reg.mtval = tval;
        // update mcause
        csr->csr_reg.mcause = cause;
        // update mepc
        // since we only support only IALIGN=32, the two low bits (mepc[1:0]) are always zero.
        csr->csr_reg.mepc = pc & TRAP_PC_ALIGN_MASK;
        // return to address defined by mtvec
        trap_vec_base = csr->csr_reg.mtvec & ~TRAP_VECTOR_MODE_MASK;
        trap_vec_mode = csr->csr_reg.mtvec & TRAP_VECTOR_MODE_MASK;
        // the new priv is M mode
        *priv = PRIV_M;
    }
    // for trap handled by S mode
    else {
        // update mstatus (sstatus)
        mstatus_sie = csr_field_get(csr->csr_reg.mstatus, MSTATUS_SIE_POS, 1);
        csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SPIE_POS, 1, mstatus_sie);    // SPIE = SIE
        csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SIE_POS, 1, 0);               // SIE = 0
        csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SPP_POS, 1, *priv == PRIV_S); // SPP = previous privilege
        // update stval
        csr->csr_reg.stval = tval;
        // update scause
        csr->csr_reg.scause = cause;
        // update sepc
        // since we only support only IALIGN=32, the two low bits (mepc[1:0]) are always zero.
        csr->csr_reg.sepc = pc & TRAP_PC_ALIGN_MASK;
        // return to address defined by mtvec
        trap_vec_base = csr->csr_reg.stvec & ~TRAP_VECTOR_MODE_MASK;
        trap_vec_mode = csr->csr_reg.stvec & TRAP_VECTOR_MODE_MASK;
        // the new priv is S mode
        *priv = PRIV_S;
    }

    // return the trap vector
    if ((trap_vec_mode == TRAP_VECTOR_MODE_VECTORED) && interrupt) {
        trap_vec = trap_vec_base + TRAP_VECTOR_ENTRY_SIZE * cause_code;
    } else {
        trap_vec = trap_vec_base;
    }
    return trap_vec;
}

/**
 * Exit trap from mret
 * - restore the privilege mode from MPP to be the privilege mode after mret
 * - restore mstatus.mie from mstatus.mpie
 * - set mstatus.mpie to 1
 * - set mstatus.mpp to 0
 * - set pc to address defined in mepc
 */
uint64_t trap_exit_mret(csr_t *csr, priv_mode_t *priv) {
    uint64_t mstatus_mpie;
    // restore the current privilege mode to MPP
    *priv = (priv_mode_t)csr_field_get(csr->csr_reg.mstatus, MSTATUS_MPP_POS, 2);
    // update mstatus
    // - restore mie and set mpie to 1
    mstatus_mpie = csr_field_get(csr->csr_reg.mstatus, MSTATUS_MPIE_POS, 1);
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_MIE_POS, 1, mstatus_mpie);
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_MPIE_POS, 1, 1);
    // - set mstatus.MPP to 0.
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_MPP_POS, 2, PRIV_U);
    // return mepc
    return csr->csr_reg.mepc & TRAP_PC_ALIGN_MASK;
}

/**
 * Exit trap from sret
 * - restore the privilege mode from SPP to be the privilege mode after sret
 * - restore mstatus.sie from mstatus.spie
 * - set mstatus.spie to 1
 * - set mstatus.spp to 0
 * - set pc to address defined in sepc
 */
uint64_t trap_exit_sret(csr_t *csr, priv_mode_t *priv) {
    uint64_t spie;
    // restore the current privilege mode to SPP
    *priv = (priv_mode_t)csr_field_get(csr->csr_reg.mstatus, MSTATUS_SPP_POS, 1);
    // update mstatus
    // - restore sie and set spie to 1
    spie = csr_field_get(csr->csr_reg.mstatus, MSTATUS_SPIE_POS, 1);
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SIE_POS, 1, spie);
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SPIE_POS, 1, 1);
    // - set SPP to 0.
    csr_field_set(&csr->csr_reg.mstatus, MSTATUS_SPP_POS, 1, PRIV_U);
    // return sepc
    return csr->csr_reg.sepc & TRAP_PC_ALIGN_MASK;
}

/**
 * Scan all the interrupt based on the order and check if there are interrupt pending and meet the condition to be
 * processed. The ordering is: MEIP > MSIP > MTIP > SEIP > SSIP > STIP
 *
 * The interrupt to be processed is captured in int_id.
 */
bool interrupt_pending_and_enabled(csr_t *csr, priv_mode_t priv, interrupt_code_t *int_id) {
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

/**
 * Check if there are interrupt pending. Used to wake up WFI
 */
bool interrupt_pending(csr_t *csr) {
    return (csr->csr_reg.mip != 0);
}

/**
 * Due to emulator limitation, we need to update some status in CSR at the beginning of the execution cycle to update
 * flags, set interrupt.
 */
void csr_begin_update(csr_t *csr, bool eip, bool sip, bool tip) {
    // Clear the flag bits
    csr->mcycle_written         = false;
    csr->minstret_written       = false;
    csr->starting_mcountinhibit = csr->csr_reg.mcountinhibit;

    // Update mip register
    csr_field_set(&csr->csr_reg.mip, INT_MEIP, 1, eip);
    csr_field_set(&csr->csr_reg.mip, INT_MSIP, 1, sip);
    csr_field_set(&csr->csr_reg.mip, INT_MTIP, 1, tip);
}

/**
 * Due to emulator limitation, we need to update some status in CSR at the end of the execution cycle.
 */
void csr_end_update(csr_t *csr, bool retired) {
    // update minstret
    if (!csr_field_get(csr->starting_mcountinhibit, COUNTER_IR_POS, 1) && !csr->minstret_written && retired) {
        csr->csr_reg.minstret++;
    }
    // update mcycle
    if (!csr_field_get(csr->starting_mcountinhibit, COUNTER_CY_POS, 1) && !csr->mcycle_written) {
        // assuming each instruction takes 1 clock for the emulator
        csr->csr_reg.mcycle++;
    }
    // clear the flag bits
    csr->minstret_written = false;
    csr->mcycle_written   = false;
}

/**
 * S-mode and U-mode can't execute mret, if they do, cpu should raise illegal instruction exception
 */
bool check_mret_privilege(priv_mode_t priv) {
    return (priv == PRIV_S) || (priv == PRIV_U);
}

/**
 * 1. If mstatus.TSR is set, execute sret will raise illegal instruction exception.
 * 2. U-mode can't execute sret, if they do, cpu should raise illegal instruction exception.
 */
bool check_sret_trap(csr_t *csr, priv_mode_t priv) {
    return ((priv == PRIV_S) && (csr->csr_reg.mstatus & MSTATUS_TSR)) || (priv == PRIV_U);
}

/**
 * - When TW=0, the WFI instruction may execute in modes less privileged than M when not prevented for some other
 *   reason.
 * - When TW=1, then if WFI is executed in any less-privileged mode, and it does not complete within an
 *   implementation-specific, bounded time limit, the WFI instruction causes an illegal instruction exception.
 * - An implementation may have WFI always raise an illegal-instruction exception in modes less privileged than M when
 *   TW=1
 * - When S-mode is implemented, then executing WFI in U-mode causes an illegal-instruction exception, regardless of the
 *   value of the TW bit, unless the instruction completes within an implementation-specific, bounded time limit.
 */
bool check_wfi_trap(csr_t *csr, priv_mode_t priv) {
    return (csr_field_get(csr->csr_reg.mstatus, MSTATUS_TW_POS, 1) && (priv == PRIV_S)) || (priv == PRIV_U);
}
