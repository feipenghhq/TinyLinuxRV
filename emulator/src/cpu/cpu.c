#include "cpu.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bus/bus.h"
#include "cpu/csr.h"
#include "cpu/decode.h"
#include "utils/log.h"

// ----------------------------------------------
// Defines and data types
// ----------------------------------------------

// 128 bit Data type
__extension__ typedef __int128          int128_t;
__extension__ typedef unsigned __int128 uint128_t;

// Instruction Opcode
typedef enum {
    OPCODE_LUI       = 0x37,
    OPCODE_AUIPC     = 0x17,
    OPCODE_JAL       = 0x6F,
    OPCODE_JALR      = 0x67,
    OPCODE_BRANCH    = 0x63,
    OPCODE_LOAD      = 0x03,
    OPCODE_STORE     = 0x23,
    OPCODE_OP_IMM    = 0x13,
    OPCODE_OP_IMM_32 = 0x1B,
    OPCODE_OP        = 0x33,
    OPCODE_OP_32     = 0x3B,
    OPCODE_MISC_MEM  = 0x0F,
    OPCODE_SYSTEM    = 0x73,
    OPCODE_AMO       = 0x2F,
} opcode_t;

// Fields shared by the instruction formats used by the executor.
typedef struct {
    uint8_t  opcode;
    uint8_t  rs1;
    uint8_t  rs2;
    uint8_t  rd;
    uint64_t imm; // Sign-extended immediate stored as an RV64 bit pattern.
    int      csr; // CSR address
} inst_dec_t;

// ----------------------------------------------
// Local Variable
// ----------------------------------------------

// Register Name
static const char *reg_name[] = {"zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
                                 "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
                                 "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

// ----------------------------------------------
// bit operation
// ----------------------------------------------

static inline uint64_t bitmask(int width) {
    return (UINT64_C(1) << width) - 1;
}

// extract the value like verilog value[hi:lo]
static inline uint64_t bits(uint64_t value, int hi, int lo) {
    return (value >> lo) & bitmask(hi - lo + 1);
}

// sign extend value to 64 bit. width indicate the width of the value
static inline uint64_t sext(uint64_t value, int width) {
    int n = 64 - width;
    return (uint64_t)(((int64_t)(value << n)) >> n);
}

// unsign extend value to 64 bit. width indicate the width of the value
static inline uint64_t uext(uint64_t value, int width) {
    int n = 64 - width;
    return ((value << n)) >> n;
}

// ----------------------------------------------
// mul//div operation
// ----------------------------------------------

static inline uint64_t mulh(uint64_t a, uint64_t b) {
    // !Note: Need to convert to int64_t first
    int128_t sa     = (int128_t)(int64_t)a;
    int128_t sb     = (int128_t)(int64_t)b;
    int128_t result = sa * sb;
    return (uint64_t)(result >> 64);
}

static inline uint64_t mulhu(uint64_t a, uint64_t b) {
    // !Note: Need to convert to int64_t first
    uint128_t sa = (uint128_t)a;
    uint128_t sb = (uint128_t)b;
    return (uint64_t)((sa * sb) >> 64);
}

static inline uint64_t mulhsu(uint64_t a, uint64_t b) {
    // !Note: Need to convert to int64_t first
    uint128_t sa = (uint128_t)(int64_t)a;
    return (uint64_t)((sa * b) >> 64);
}

static inline uint64_t mulw(uint64_t a, uint64_t b) {
    // !Note: Need to convert to int64_t first
    uint32_t al = (uint32_t)a;
    uint32_t bl = (uint32_t)b;
    return (uint64_t)sext(al * bl, 32);
}

static inline uint64_t div(uint64_t a, uint64_t b) {
    int64_t sa = (int64_t)a;
    int64_t sb = (int64_t)b;
    if (b == 0) {
        return UINT64_C(-1);
    } else if (sa == INT64_MIN && sb == -1) {
        return (uint64_t)INT64_MIN;
    } else {
        return (uint64_t)(sa / sb);
    }
}

static inline uint64_t divu(uint64_t a, uint64_t b) {
    if (b == 0) {
        return UINT64_MAX;
    } else {
        return a / b;
    }
}

static inline uint64_t rem(uint64_t a, uint64_t b) {
    int64_t sa = (int64_t)a;
    int64_t sb = (int64_t)b;
    if (b == 0) {
        return a;
    } else if (sa == INT64_MIN && sb == -1) {
        return 0;
    } else {
        return (uint64_t)(sa % sb);
    }
}

static inline uint64_t remu(uint64_t a, uint64_t b) {
    if (b == 0) {
        return a;
    } else {
        return a % b;
    }
}

static inline uint64_t divw(uint64_t a, uint64_t b) {
    int32_t sa = (int32_t)a;
    int32_t sb = (int32_t)b;
    int32_t result;
    if (sb == 0) {
        return UINT64_C(-1);
    } else if (sa == INT32_MIN && sb == -1) {
        return (uint64_t)INT32_MIN;
    } else {
        result = sa / sb;
        return sext((uint64_t)result, 32);
    }
}

static inline uint64_t divuw(uint64_t a, uint64_t b) {
    uint32_t al = (uint32_t)a;
    uint32_t bl = (uint32_t)b;
    if (bl == 0) {
        return UINT64_MAX;
    } else {
        return (uint64_t)sext(al / bl, 32);
    }
}

static inline uint64_t remw(uint64_t a, uint64_t b) {
    int32_t sa = (int32_t)a;
    int32_t sb = (int32_t)b;
    int32_t result;
    if (sb == 0) {
        return sext(a, 32);
    } else if (sa == INT32_MIN && sb == -1) {
        return 0;
    } else {
        result = sa % sb;
        return (uint64_t)sext((uint64_t)result, 32);
    }
}

static inline uint64_t remuw(uint64_t a, uint64_t b) {
    uint32_t al = (uint32_t)a;
    uint32_t bl = (uint32_t)b;
    if (bl == 0) {
        return sext((uint32_t)a, 32);
    } else {
        return (uint64_t)sext(al % bl, 32);
    }
}

// ----------------------------------------------
// instruction decode
// ----------------------------------------------

#define PC()  cpu->pc
#define RD()  cpu->regs[inst_dec.rd]
#define RS1() cpu->regs[inst_dec.rs1]
#define RS2() cpu->regs[inst_dec.rs2]
#define IMM() inst_dec.imm

#define SRS1() (int64_t)RS1()
#define SRS2() (int64_t)RS2()

#define IMM_I(i) sext(bits(i, 31, 20), 12)
#define IMM_S(i) sext(bits(i, 11, 7) | bits(i, 31, 25) << 5, 12)
#define IMM_B(i) sext(bits(i, 31, 31) << 12 | bits(i, 30, 25) << 5 | bits(i, 11, 8) << 1 | bits(i, 7, 7) << 11, 13)
#define IMM_U(i) sext(i & 0xFFFFF000, 32)
#define IMM_J(i) sext(bits(i, 31, 31) << 20 | bits(i, 30, 21) << 1 | bits(i, 20, 20) << 11 | bits(i, 19, 12) << 12, 21)

// decode the immediate value based on opcode
static uint64_t decode_imm(uint32_t inst, uint8_t opcode) {
    switch (opcode) {
    case (OPCODE_LOAD):
    case (OPCODE_OP_IMM):
    case (OPCODE_OP_IMM_32):
    case (OPCODE_JALR):
        return IMM_I(inst);

    case (OPCODE_AUIPC):
    case (OPCODE_LUI):
        return IMM_U(inst);

    case (OPCODE_STORE):
        return IMM_S(inst);

    case (OPCODE_JAL):
        return IMM_J(inst);

    case (OPCODE_BRANCH):
        return IMM_B(inst);

    case (OPCODE_SYSTEM): // CSR
        return uext(bits(inst, 19, 15), 5);
    }
    return 0;
}

// Extract register indices and reconstruct the immediate for each opcode type.
static void decode(uint32_t inst, inst_dec_t *inst_dec) {
    inst_dec->opcode = (uint8_t)bits(inst, 6, 0);
    inst_dec->rs1    = (uint8_t)bits(inst, 19, 15);
    inst_dec->rs2    = (uint8_t)bits(inst, 24, 20);
    inst_dec->rd     = (uint8_t)bits(inst, 11, 7);
    inst_dec->csr    = (int)bits(inst, 31, 20);
    inst_dec->imm    = decode_imm(inst, inst_dec->opcode);
}

// ----------------------------------------------
// Exception checking and handling
// ----------------------------------------------

// Instruction misaligned address for instruction fetch
#define CHECK_MA_FETCH(addr)                   \
    do {                                       \
        if ((addr & 0x3) != 0) {               \
            trap_cause = INST_ADDR_MISALIGNED; \
            trap_val   = addr;                 \
            goto raise_exception;              \
        }                                      \
    } while (0)

// Load/Store address misaligned address
#define CHECK_MA_LS(addr, size, cause)                 \
    do {                                               \
        bool ma_addr = false;                          \
        ma_addr |= (size == 2) && ((addr & 0x1) != 0); \
        ma_addr |= (size == 4) && ((addr & 0x3) != 0); \
        ma_addr |= (size == 8) && ((addr & 0x7) != 0); \
        if (ma_addr) {                                 \
            trap_cause = cause;                        \
            trap_val   = addr;                         \
            goto raise_exception;                      \
        }                                              \
    } while (0)

// ----------------------------------------------
// Instruction execution macro
// ----------------------------------------------

// Match and execute one instruction.
#define INSTPAT(name, op)                            \
    do {                                             \
        if ((inst & MASK_##name) == GOLDEN_##name) { \
            op;                                      \
            goto end_exec;                           \
        }                                            \
    } while (0)

// execute branch instruction
#define EXEC_BRANCH(cond)            \
    do {                             \
        if (cond) {                  \
            next_pc = PC() + IMM();  \
            CHECK_MA_FETCH(next_pc); \
        }                            \
    } while (0)

// Execute a load. ext selects either sign extension or no extension.
#define NOEXT(value, bits) (value)
#define EXEC_LOAD_HELPER(addr, size, ext, target, access_fault_cause) \
    do {                                                              \
        uint64_t _addr = (addr);                                      \
        uint64_t _size = (size);                                      \
        uint64_t data  = 0;                                           \
        CHECK_MA_LS(_addr, _size, LOAD_ADDR_MISALIGNED);              \
        if (bus_read(bus, _addr, size, &data) != 0) {                 \
            trap_cause = access_fault_cause;                          \
            trap_val   = _addr;                                       \
            goto raise_exception;                                     \
        }                                                             \
        target = ext(data, size * 8);                                 \
    } while (0)
#define EXEC_LOAD(addr, size, ext) EXEC_LOAD_HELPER(addr, size, ext, RD(), LOAD_ACCESS_FAULT)

// Execute a store.
#define EXEC_STORE(addr, data, size)                          \
    do {                                                      \
        uint64_t _addr = (addr);                              \
        uint64_t _size = (size);                              \
        CHECK_MA_LS(_addr, _size, STORE_AMO_ADDR_MISALIGNED); \
        if (bus_write(bus, _addr, size, &data) != 0) {        \
            trap_cause = STORE_AMO_ACCESS_FAULT;              \
            trap_val   = _addr;                               \
            goto raise_exception;                             \
        }                                                     \
    } while (0)

// Execute CSR instruction
#define EXEC_CSR(op, value, rd, rs1)                                                        \
    do {                                                                                    \
        uint64_t rdata;                                                                     \
        bool     read  = !((rd) == 0 && (op) == CSR_OP_RW);                                 \
        bool     write = !((rs1) == 0 && ((op) == CSR_OP_RS || (op) == CSR_OP_RC));         \
        if (csr_access(&cpu->csr, inst_dec.csr, (op), (value), &rdata, read, write) != 0) { \
            trap_cause = ILLEGAL_INSTRUCTION;                                               \
            trap_val   = inst;                                                              \
            goto raise_exception;                                                           \
        }                                                                                   \
        if ((rd) != 0) {                                                                    \
            RD() = rdata;                                                                   \
        }                                                                                   \
    } while (0)

// Helper Macro for LR/SC/AMO

// Need to record RS1 first because LR could override RS1 if RD = RS1
#define EXEC_LR(res, size)               \
    do {                                 \
        uint64_t lr_addr = RS1();        \
        EXEC_LOAD(RS1(), size, sext);    \
        res.valid      = true;           \
        res.addr_start = lr_addr;        \
        res.addr_end   = lr_addr + size; \
    } while (0)

// For SC, we also need to check address alignment.
// If the address does not hit, then just check the alignment, if hit, store instuction will check it
#define EXEC_SC(res, size)                                                         \
    do {                                                                           \
        CHECK_MA_LS(RS1(), size, STORE_AMO_ADDR_MISALIGNED);                       \
        if (!res.valid || RS1() < res.addr_start || RS1() > res.addr_end - size) { \
            res.valid = false;                                                     \
            RD()      = 1;                                                         \
        } else {                                                                   \
            res.valid = false;                                                     \
            EXEC_STORE(RS1(), RS2(), size);                                        \
            RD() = 0;                                                              \
        }                                                                          \
    } while (0)

// Need to record RS1 and RS2 first because AMO could override if RD = RS1 or RD = RS2
#define EXEC_AMO(size, op)                                                          \
    do {                                                                            \
        uint64_t amo_addr = RS1();                                                  \
        uint64_t amo_src  = RS2();                                                  \
        uint64_t read_value;                                                        \
        CHECK_MA_LS(RS1(), size, STORE_AMO_ADDR_MISALIGNED);                        \
        EXEC_LOAD_HELPER(amo_addr, size, sext, read_value, STORE_AMO_ACCESS_FAULT); \
        uint64_t result = (op);                                                     \
        EXEC_STORE(amo_addr, result, size);                                         \
        RD() = read_value;                                                          \
    } while (0)

#define CMP64U(a, b, op) ((a)op(b) ? (a) : (b))
#define CMP32U(a, b, op) sext(((uint32_t)(a)op(uint32_t)(b) ? (a) : (b)), 32)

#define CMP64S(a, b, op) ((int64_t)(a)op(int64_t)(b) ? (a) : (b))
#define CMP32S(a, b, op) sext(((int32_t)(a)op(int32_t)(b) ? (a) : (b)), 32)

// ----------------------------------------------
// Main CPU API
// ----------------------------------------------

/**
 * Initialize the CPU state to deterministic state.
 * Set PC to reset vector and clear registers to 0. Set halted to false.
 */
void cpu_init(cpu_t *cpu) {
    cpu->halted = false;
    cpu->pc     = RST_VEC;

    // initialize all the register to 0 to make sure the emulator is deterministic
    memset(cpu->regs, 0, sizeof(cpu->regs));
    // init reservation area
    cpu->res = (reservation_t){false, 0, 0};
    // initialize csr
    csr_init(&cpu->csr);

    LOG_INFO("Initialize CPU done");
}

/**
 * Execute a SINGLE instruction
 */
void cpu_execute(cpu_t *cpu, uint32_t inst, bus_t *bus) {
    inst_dec_t inst_dec;
    uint64_t   next_pc;

    uint64_t trap_cause = 0;
    uint64_t trap_val   = 0;

    decode(inst, &inst_dec);
    next_pc = PC() + 4; // precalculate next pc, for most of the instruction it is pc + 4
    switch (inst_dec.opcode) {
    case OPCODE_LUI:
        INSTPAT(LUI, RD() = IMM());
        goto illegal_instruction;

    case OPCODE_AUIPC:
        INSTPAT(AUIPC, RD() = PC() + IMM());
        goto illegal_instruction;

    case OPCODE_JAL:
        INSTPAT(JAL, next_pc = PC() + IMM(); CHECK_MA_FETCH(next_pc); RD() = PC() + 4);
        goto illegal_instruction;

    case OPCODE_JALR:
        INSTPAT(JALR, next_pc = (IMM() + RS1()) & ~UINT64_C(1); CHECK_MA_FETCH(next_pc); RD() = PC() + 4;);
        goto illegal_instruction;

    case OPCODE_BRANCH:
        INSTPAT(BEQ, EXEC_BRANCH(RS1() == RS2()));
        INSTPAT(BNE, EXEC_BRANCH(RS1() != RS2()));
        INSTPAT(BLT, EXEC_BRANCH(SRS1() < SRS2()));
        INSTPAT(BGE, EXEC_BRANCH(SRS1() >= SRS2()));
        INSTPAT(BLTU, EXEC_BRANCH(RS1() < RS2()));
        INSTPAT(BGEU, EXEC_BRANCH(RS1() >= RS2()));
        goto illegal_instruction;

    case OPCODE_LOAD:
        INSTPAT(LB, EXEC_LOAD(RS1() + IMM(), 1, sext));
        INSTPAT(LH, EXEC_LOAD(RS1() + IMM(), 2, sext));
        INSTPAT(LW, EXEC_LOAD(RS1() + IMM(), 4, sext));
        INSTPAT(LBU, EXEC_LOAD(RS1() + IMM(), 1, NOEXT));
        INSTPAT(LHU, EXEC_LOAD(RS1() + IMM(), 2, NOEXT));
        INSTPAT(LWU, EXEC_LOAD(RS1() + IMM(), 4, NOEXT));
        INSTPAT(LD, EXEC_LOAD(RS1() + IMM(), 8, NOEXT));
        goto illegal_instruction;

    case OPCODE_STORE:
        INSTPAT(SB, EXEC_STORE(RS1() + IMM(), RS2(), 1));
        INSTPAT(SH, EXEC_STORE(RS1() + IMM(), RS2(), 2));
        INSTPAT(SW, EXEC_STORE(RS1() + IMM(), RS2(), 4));
        INSTPAT(SD, EXEC_STORE(RS1() + IMM(), RS2(), 8));
        goto illegal_instruction;

    case OPCODE_OP_IMM:
        INSTPAT(ADDI, RD() = RS1() + IMM());
        INSTPAT(SLTI, RD() = SRS1() < (int64_t)(IMM()));
        INSTPAT(SLTIU, RD() = RS1() < IMM());
        INSTPAT(XORI, RD() = RS1() ^ IMM());
        INSTPAT(ORI, RD() = RS1() | IMM());
        INSTPAT(ANDI, RD() = RS1() & IMM());
        INSTPAT(SLLI, RD() = RS1() << (IMM() & 0x3F));
        INSTPAT(SRLI, RD() = RS1() >> (IMM() & 0x3F));
        INSTPAT(SRAI, RD() = (uint64_t)(SRS1() >> (IMM() & 0x3F)));
        goto illegal_instruction;

    case OPCODE_OP_IMM_32:
        INSTPAT(ADDIW, RD() = sext(RS1() + IMM(), 32));
        INSTPAT(SLLIW, RD() = sext((uint32_t)(RS1()) << (IMM() & 0x1F), 32));
        INSTPAT(SRLIW, RD() = sext((uint32_t)(RS1()) >> (IMM() & 0x1F), 32));
        INSTPAT(SRAIW, RD() = sext((uint32_t)((int32_t)RS1() >> (IMM() & 0x1F)), 32));
        goto illegal_instruction;

    case OPCODE_OP:
        INSTPAT(ADD, RD() = RS1() + RS2());
        INSTPAT(SUB, RD() = RS1() - RS2());
        INSTPAT(SLL, RD() = RS1() << (RS2() & 0x3F));
        INSTPAT(SLT, RD() = SRS1() < SRS2());
        INSTPAT(SLTU, RD() = RS1() < RS2());
        INSTPAT(XOR, RD() = RS1() ^ RS2());
        INSTPAT(SRL, RD() = RS1() >> (RS2() & 0x3F));
        INSTPAT(SRA, RD() = (uint64_t)((int64_t)RS1() >> (RS2() & 0x3F)));
        INSTPAT(OR, RD() = RS1() | RS2());
        INSTPAT(AND, RD() = RS1() & RS2());
        INSTPAT(MUL, RD() = RS1() * RS2());
        INSTPAT(MULH, RD() = mulh(RS1(), RS2()));
        INSTPAT(MULHU, RD() = mulhu(RS1(), RS2()));
        INSTPAT(MULHSU, RD() = mulhsu(RS1(), RS2()));
        INSTPAT(DIV, RD() = div(RS1(), RS2()));
        INSTPAT(DIVU, RD() = divu(RS1(), RS2()));
        INSTPAT(REM, RD() = rem(RS1(), RS2()));
        INSTPAT(REMU, RD() = remu(RS1(), RS2()));
        goto illegal_instruction;

    case OPCODE_OP_32:
        INSTPAT(ADDW, RD() = sext(RS1() + RS2(), 32));
        INSTPAT(SUBW, RD() = sext(RS1() - RS2(), 32));
        INSTPAT(SLLW, RD() = sext((uint32_t)(RS1()) << (RS2() & 0x1F), 32));
        INSTPAT(SRLW, RD() = sext((uint32_t)(RS1()) >> (RS2() & 0x1F), 32));
        INSTPAT(SRAW, RD() = sext((uint32_t)((int32_t)RS1() >> (RS2() & 0x1F)), 32));

        INSTPAT(MULW, RD() = mulw(RS1(), RS2()));
        INSTPAT(DIVW, RD() = divw(RS1(), RS2()));
        INSTPAT(DIVUW, RD() = divuw(RS1(), RS2()));
        INSTPAT(REMW, RD() = remw(RS1(), RS2()));
        INSTPAT(REMUW, RD() = remuw(RS1(), RS2()));
        goto illegal_instruction;

    case OPCODE_MISC_MEM:
        INSTPAT(FENCE, );
        INSTPAT(FENCEI, );
        goto illegal_instruction;

    case OPCODE_SYSTEM:
        INSTPAT(ECALL, trap_cause = ECALL_FROM_M_MODE; trap_val = 0; goto raise_exception);
        // Treat ebreak as the temporary halt convention
        INSTPAT(EBREAK, cpu->halted = true);
        // ZICSR
        INSTPAT(CSRRW, EXEC_CSR(CSR_OP_RW, RS1(), inst_dec.rd, inst_dec.rs1));
        INSTPAT(CSRRS, EXEC_CSR(CSR_OP_RS, RS1(), inst_dec.rd, inst_dec.rs1));
        INSTPAT(CSRRC, EXEC_CSR(CSR_OP_RC, RS1(), inst_dec.rd, inst_dec.rs1));
        INSTPAT(CSRRWI, EXEC_CSR(CSR_OP_RW, inst_dec.imm, inst_dec.rd, inst_dec.rs1));
        INSTPAT(CSRRSI, EXEC_CSR(CSR_OP_RS, inst_dec.imm, inst_dec.rd, inst_dec.rs1));
        INSTPAT(CSRRCI, EXEC_CSR(CSR_OP_RC, inst_dec.imm, inst_dec.rd, inst_dec.rs1));
        INSTPAT(MRET, next_pc = trap_exit(&cpu->csr));
        goto illegal_instruction;

    case OPCODE_AMO:
        INSTPAT(LR_D, EXEC_LR(cpu->res, 8));
        INSTPAT(LR_W, EXEC_LR(cpu->res, 4));
        INSTPAT(SC_D, EXEC_SC(cpu->res, 8));
        INSTPAT(SC_W, EXEC_SC(cpu->res, 4));
        // AMO
        INSTPAT(AMOSWAP_D, EXEC_AMO(8, amo_src));
        INSTPAT(AMOSWAP_W, EXEC_AMO(4, amo_src));
        INSTPAT(AMOADD_D, EXEC_AMO(8, amo_src + read_value));
        INSTPAT(AMOADD_W, EXEC_AMO(4, amo_src + read_value));
        INSTPAT(AMOAND_D, EXEC_AMO(8, amo_src & read_value));
        INSTPAT(AMOAND_W, EXEC_AMO(4, amo_src & read_value));
        INSTPAT(AMOOR_D, EXEC_AMO(8, amo_src | read_value));
        INSTPAT(AMOOR_W, EXEC_AMO(4, amo_src | read_value));
        INSTPAT(AMOXOR_D, EXEC_AMO(8, amo_src ^ read_value));
        INSTPAT(AMOXOR_W, EXEC_AMO(4, amo_src ^ read_value));
        INSTPAT(AMOMAXU_D, EXEC_AMO(8, CMP64U(amo_src, read_value, >)));
        INSTPAT(AMOMAXU_W, EXEC_AMO(4, CMP32U(amo_src, read_value, >)));
        INSTPAT(AMOMAX_D, EXEC_AMO(8, CMP64S(amo_src, read_value, >)));
        INSTPAT(AMOMAX_W, EXEC_AMO(4, CMP32S(amo_src, read_value, >)));
        INSTPAT(AMOMINU_D, EXEC_AMO(8, CMP64U(amo_src, read_value, <)));
        INSTPAT(AMOMINU_W, EXEC_AMO(4, CMP32U(amo_src, read_value, <)));
        INSTPAT(AMOMIN_D, EXEC_AMO(8, CMP64S(amo_src, read_value, <)));
        INSTPAT(AMOMIN_W, EXEC_AMO(4, CMP32S(amo_src, read_value, <)));
        goto illegal_instruction;

    default:
        goto illegal_instruction;
    }

illegal_instruction:
    LOG_DEBUG("CPU: Hit an invalid instruction at address: %lx, instruction: %x", cpu->pc, inst);
    trap_cause = ILLEGAL_INSTRUCTION;
    trap_val   = inst;

raise_exception:
    LOG_DEBUG("CPU: Raise an instruction at PC: %lx, instruction: %x. Cause: %ld. Val: %lx", cpu->pc, inst, trap_cause,
              trap_val);
    next_pc = trap_enter(&cpu->csr, trap_cause, trap_val, PC());

end_exec:
    cpu->regs[0] = 0; // restore reg[0] to zero
    PC()         = next_pc;
}

void cpu_print_regs(cpu_t *cpu) {
    fprintf(stderr, "Register Values:\n");
    for (int i = 0; i < 32; i++) {
        fprintf(stderr, "%5s (r%d) = 0x%016lx\n", reg_name[i], i, cpu->regs[i]);
    }
}
