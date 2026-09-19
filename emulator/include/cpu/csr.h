#ifndef CSR_H
#define CSR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CSR_OP_RW = 1,
    CSR_OP_RS = 2,
    CSR_OP_RC = 3,
} csr_op_t;

typedef enum {
    PRIV_U = 0,
    PRIV_S = 1,
    PRIV_M = 3,
} priv_mode_t;

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
    uint64_t mcountinhibit;
    uint64_t mcycle;
    uint64_t minstret;
    uint64_t mvendorid;
    uint64_t marchid;
    uint64_t mimpid;
    uint64_t mhartid;
    // Supervisor Mode
    uint64_t sstatus; // sstatus is a dummy csr register sstatus is subset of mstatus
    uint64_t sie;     // sie is a dummy csr register sstatus is subset of mie
    uint64_t stvec;
    uint64_t scounteren;
    uint64_t scountinhibit;
    uint64_t sscratch;
    uint64_t sepc;
    uint64_t scause;
    uint64_t stval;
    uint64_t sip; // sip is a dummy csr register sstatus is subset of mip
    uint64_t stap;
    // Not a CSR register but required from C implementation perspective.
    uint64_t mtime; // a copy of the mtime register in CLINT. Get updated, when csr_access is called
} csr_reg_t;

typedef struct {
    csr_reg_t csr_reg;
    uint64_t  starting_mcountinhibit; // save mcountinhibit as the mcycle/minstret is based on the value before written
    bool      mcycle_written;
    bool      minstret_written;
} csr_t;

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

void     csr_init(csr_t *csr);
int      csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr,
                    uint64_t time_value);
uint64_t trap_enter(csr_t *csr, uint64_t cause, uint64_t mtval, uint64_t pc, priv_mode_t *priv);
uint64_t trap_exit_mret(csr_t *csr, priv_mode_t *priv);
uint64_t trap_exit_sret(csr_t *csr, priv_mode_t *priv);
bool     interrupt_pending_and_enabled(csr_t *csr, priv_mode_t priv, interrupt_code_t *id);
bool     interrupt_pending(csr_t *csr);
void     csr_begin_update(csr_t *csr, bool eip, bool sip, bool tip);
void     csr_end_update(csr_t *csr, bool retired);
bool     check_sret_trap(csr_t *csr, priv_mode_t priv);

#endif
