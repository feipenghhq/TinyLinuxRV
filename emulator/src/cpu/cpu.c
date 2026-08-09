#include "cpu.h"
#include "memory.h"
#include "log.h"

// Instruction field
typedef struct {
    uint8_t     opcode;
    uint8_t     rs1;
    uint8_t     rs2;
    uint8_t     rd;
    uint64_t    imm;   // immediate number
} inst_dec_t;

// Sign extend a value. bits indicates the width of the value
static inline uint64_t sext(uint64_t value, int bits) {
    return (uint64_t)(((int64_t) (value << (64 - bits))) >> (64 -bits));
}

// Decode an instruction to different fields
static void decode(uint32_t inst, inst_dec_t *inst_dec) {
    uint8_t opcode;
    opcode = inst & 0x7F;
    inst_dec->opcode = opcode;
    inst_dec->rs1 = (inst >> 15) & 0x1F;
    inst_dec->rs2 = (inst >> 20) & 0x1F;
    inst_dec->rd  = (inst >> 7)  & 0x1F;
    inst_dec->imm = 0;

    switch(opcode) {
        case(0x03): // fall-through
        case(0x13): // fall-through
        case(0x1B): // fall-through
        case(0x67): { // I-type immediate
            inst_dec->imm = sext((inst >> 20) & 0xFFF, 12);
            break;
        }
        case(0x17): // fall-through
        case(0x37): {
            inst_dec->imm = inst & 0xFFFFF000;
            break;
        }
        case(0x23): { // S-type store
            inst_dec->imm = sext(((inst >> 25) << 5) | ((inst >> 7) & 0x1F), 12);
            break;
        }
        case(0x6F): { // J-type
            inst_dec->imm = sext(
                    (((uint64_t)(inst >> 31) & 0x1)   << 20) |
                    (((uint64_t)(inst >> 21) & 0x3FF) << 1)  |
                    (((uint64_t)(inst >> 20) & 0x1)   << 11) |
                    (((uint64_t)(inst >> 12) & 0xFF)  << 12),
                    21);
            break;
        }
        case(0x63): { // B-type
            inst_dec->imm = sext(
                (((uint64_t)(inst >> 31) & 0x1)  << 12) |
                (((uint64_t)(inst >> 7)  & 0x1)  << 11) |
                (((uint64_t)(inst >> 25) & 0x3F) << 5)  |
                (((uint64_t)(inst >> 8)  & 0xF)  << 1),
                13);
            break;
        }
    }
}

// Convert the instruction pattern to mask
static int parse_pattern(const char *pattern, uint32_t *mask, uint32_t *golden) {
    *mask = 0;
    *golden = 0;
    for (; *pattern != '\0'; pattern++) {
        switch (*pattern) {
            case ' ':  // fall-through case
            case '_':  break;   // skip space and _
            case '?': {
                *mask = *mask << 1;
                *golden = *golden << 1;
                break;
            }
            case '0': {
                *mask = (*mask << 1) | 1;
                *golden = *golden << 1;
                break;
            }
            case '1': {
                *mask = (*mask << 1) | 1;
                *golden = (*golden << 1) | 1;
                break;
            }
            default: {
                LOG_ERROR("Incorrect pattern: %s", pattern);
                return -1;
            }
        }
    }
    return 0;
}

#define PC()  cpu->pc
#define RD()  cpu->regs[inst_dec.rd]
#define RS1() cpu->regs[inst_dec.rs1]
#define RS2() cpu->regs[inst_dec.rs2]
#define IMM() inst_dec.imm

// Macro to add the new instruction, use to execute an instruction
#define ADD_INST(name, pattern, op)                 \
    do {                                            \
        uint32_t mask, golden;                      \
        if (parse_pattern(pattern, &mask, &golden) != 0) { \
            return -1;                              \
        }                                           \
        if ((mask & inst) == golden) {              \
            LOG_DEBUG("EXECUTE %-8s: PC=%lx INST=%08x", name, cpu->pc, inst); \
            op;                                     \
            cpu->regs[0] = 0;                       \
            return 0;                               \
        }                                           \
    } while(0)

// instruction without branch
#define NO_BRANCH_OP(op) do {op; PC() += 4;} while(0)
#define ADD_INST_NB(name, pattern, op) ADD_INST(name, pattern, NO_BRANCH_OP(op))

// memory read instruction
#define noext(value, bits) (value)
#define LOAD_MEM_OP(size, ext)  \
    do {                        \
        uint64_t data = 0;      \
        if (memory_read(memory, RS1() + IMM(), size, &data) != 0) {  \
            cpu->halted = true; \
            return -1;          \
        }                       \
        RD() = ext(data, size * 8); \
        PC() += 4;              \
    } while(0)
#define ADD_INST_LOAD(name, pattern, size, ext) ADD_INST(name, pattern, LOAD_MEM_OP(size, ext))

// memory write instruction
#define STORE_MEM_OP(size)      \
    do {                        \
        const uint64_t value = RS2(); \
        if (memory_write(memory, RS1() + IMM(), size, &value) != 0) {  \
            cpu->halted = true; \
            return -1;          \
        }                       \
        PC() += 4;              \
    } while(0)
#define ADD_INST_STORE(name, pattern, size) ADD_INST(name, pattern, STORE_MEM_OP(size))

/**
 * Initialize the CPU state to deterministic state.
 * Set PC to reset vector and clear registers to 0. Set halted to false.
 */
void cpu_init(cpu_t *cpu) {
    cpu->halted = false;
    cpu->pc = RST_VEC;

    // initialize all the register to 0 to make sure the emulator is deterministic
    for (int i = 0; i < 32; i++) {
        cpu->regs[i] = 0;
    }
    LOG_INFO("Initialize CPU done");
}

/**
 * Execute a SINGLE instruction
 */
int cpu_execute(cpu_t *cpu, uint32_t inst, memory_t *memory) {
    inst_dec_t inst_dec;

    // Decode the instruction
    decode(inst, &inst_dec);

    // I-type instruction
    ADD_INST_NB("ADDI",  "????_????_????_????_?000_????_?001_0011", RD() = RS1() + IMM());
    ADD_INST_NB("SLTI",  "????_????_????_????_?010_????_?001_0011", if (((int64_t) RS1()) < ((int64_t) IMM())) RD() = 1; else RD() = 0);
    ADD_INST_NB("SLTIU", "????_????_????_????_?011_????_?001_0011", if (RS1() < IMM()) RD() = 1; else RD() = 0);
    ADD_INST_NB("XORI",  "????_????_????_????_?100_????_?001_0011", RD() = RS1() ^ IMM());
    ADD_INST_NB("ORI",   "????_????_????_????_?110_????_?001_0011", RD() = RS1() | IMM());
    ADD_INST_NB("ANDI",  "????_????_????_????_?111_????_?001_0011", RD() = RS1() & IMM());
    ADD_INST_NB("SLLI",  "0000_00??_????_????_?001_????_?001_0011", RD() = RS1() << (IMM() & 0x3F));
    ADD_INST_NB("SRLI",  "0000_00??_????_????_?101_????_?001_0011", RD() = RS1() >> (IMM() & 0x3F));
    ADD_INST_NB("SRAI",  "0100_00??_????_????_?101_????_?001_0011", RD() = (uint64_t)((int64_t) RS1()) >> (IMM() & 0x3F));

    ADD_INST_NB("ADDIW", "????_????_????_????_?000_????_?001_1011", RD() = sext(RS1() + IMM(), 32));
    ADD_INST_NB("SLLIW", "0000_000?_????_????_?001_????_?001_1011", RD() = sext(           ((uint32_t) RS1()) << (IMM() & 0x1F), 32));
    ADD_INST_NB("SRLIW", "0000_000?_????_????_?101_????_?001_1011", RD() = sext(           ((uint32_t) RS1()) >> (IMM() & 0x1F), 32));
    ADD_INST_NB("SRAIW", "0100_000?_????_????_?101_????_?001_1011", RD() = sext((uint64_t) (( int32_t) RS1()) >> (IMM() & 0x1F),32));

    // R-type instruction
    ADD_INST_NB("ADD",  "0000_000?_????_????_?000_????_?011_0011", RD() = RS1() + RS2());
    ADD_INST_NB("SUB",  "0100_000?_????_????_?000_????_?011_0011", RD() = RS1() - RS2());
    ADD_INST_NB("SLL",  "0000_000?_????_????_?001_????_?011_0011", RD() = RS1() << (RS2() & 0x3F));
    ADD_INST_NB("SLT",  "0000_000?_????_????_?010_????_?011_0011", if ((int64_t) RS1() < (int64_t) RS2()) RD() = 1; else RD() = 0);
    ADD_INST_NB("SLTU", "0000_000?_????_????_?011_????_?011_0011", if (RS1() < RS2()) RD() = 1; else RD() = 0);
    ADD_INST_NB("XOR",  "0000_000?_????_????_?100_????_?011_0011", RD() = RS1() ^ RS2());
    ADD_INST_NB("SRL",  "0000_000?_????_????_?101_????_?011_0011", RD() = RS1() >> (RS2() & 0x3F));
    ADD_INST_NB("SRA",  "0100_000?_????_????_?101_????_?011_0011", RD() = (uint64_t)((int64_t) RS1()) >> (RS2() & 0x3F));
    ADD_INST_NB("OR",   "0000_000?_????_????_?110_????_?011_0011", RD() = RS1() | RS2());
    ADD_INST_NB("AND",  "0000_000?_????_????_?111_????_?011_0011", RD() = RS1() & RS2());

    ADD_INST_NB("ADDW",  "0000_000?_????_????_?000_????_?011_1011", RD() = sext(RS1() + RS2(), 32));
    ADD_INST_NB("SUBW",  "0100_000?_????_????_?000_????_?011_1011", RD() = sext(RS1() - RS2(), 32));
    ADD_INST_NB("SLLW",  "0000_000?_????_????_?001_????_?011_1011", RD() = sext((uint64_t) ((uint32_t) RS1()) << (uint32_t)(RS2() & 0x1F), 32));
    ADD_INST_NB("SRLW",  "0000_000?_????_????_?101_????_?011_1011", RD() = sext((uint64_t) ((uint32_t) RS1()) >> (uint32_t)(RS2() & 0x1F), 32));
    ADD_INST_NB("SRAW",  "0100_000?_????_????_?101_????_?011_1011", RD() = sext((uint64_t) (( int32_t) RS1()) >> (uint32_t)(RS2() & 0x1F), 32));

    // U-type
    ADD_INST_NB("LUI",   "????_????_????_????_????_????_?011_0111", RD() = sext(IMM(), 32));
    ADD_INST_NB("AUIPC", "????_????_????_????_????_????_?001_0111", RD() = PC() + sext(IMM(), 32));

    // S-type
    ADD_INST_LOAD("LB",  "????_????_????_????_?000_????_?000_0011", 1, sext);
    ADD_INST_LOAD("LH",  "????_????_????_????_?001_????_?000_0011", 2, sext);
    ADD_INST_LOAD("LW",  "????_????_????_????_?010_????_?000_0011", 4, sext);
    ADD_INST_LOAD("LBU", "????_????_????_????_?100_????_?000_0011", 1, noext);
    ADD_INST_LOAD("LHU", "????_????_????_????_?101_????_?000_0011", 2, noext);
    ADD_INST_LOAD("LWU", "????_????_????_????_?110_????_?000_0011", 4, noext);
    ADD_INST_LOAD("LD",  "????_????_????_????_?011_????_?000_0011", 8, noext);

    ADD_INST_STORE("SB",  "????_????_????_????_?000_????_?010_0011", 1);
    ADD_INST_STORE("SH",  "????_????_????_????_?001_????_?010_0011", 2);
    ADD_INST_STORE("SW",  "????_????_????_????_?010_????_?010_0011", 4);
    ADD_INST_STORE("SD",  "????_????_????_????_?011_????_?010_0011", 8);

    // J-type
    ADD_INST("JAL",  "????_????_????_????_????_????_?110_1111", RD() = PC() + 4; PC() += sext(IMM(), 21));
    ADD_INST("JALR", "????_????_????_????_?000_????_?110_0111", uint64_t tgt = PC() + 4; PC() = (sext(IMM(), 12) + RS1()); PC() &= ~0x1ULL; RD() = tgt);

    ADD_INST("BEQ",  "????_????_????_????_?000_????_?110_0011", if (RS1() == RS2()) PC() += IMM(); else PC() += 4);
    ADD_INST("BNE",  "????_????_????_????_?001_????_?110_0011", if (RS1() != RS2()) PC() += IMM(); else PC() += 4);
    ADD_INST("BLT",  "????_????_????_????_?100_????_?110_0011", if ((int64_t) RS1() <  (int64_t) RS2()) PC() += IMM(); else PC() += 4);
    ADD_INST("BGE",  "????_????_????_????_?101_????_?110_0011", if ((int64_t) RS1() >= (int64_t) RS2()) PC() += IMM(); else PC() += 4);
    ADD_INST("BLTU", "????_????_????_????_?110_????_?110_0011", if (RS1() <  RS2()) PC() += IMM(); else PC() += 4);
    ADD_INST("BGEU", "????_????_????_????_?111_????_?110_0011", if (RS1() >= RS2()) PC() += IMM(); else PC() += 4);

    // system
    ADD_INST_NB("FENCE", "????_????_????_????_?000_????_?000_1111", );
    ADD_INST_NB("ECALL", "0000_0000_0000_0000_0000_0000_0111_0011", );

    // Treat ebreak as the temporary Phase 1 halt convention
    ADD_INST_NB("EBREAK", "0000_0000_0001_0000_0000_0000_0111_0011", cpu->halted = true);

    // Hit an invalid instruction if program execute this point.
    cpu->halted = true;
    LOG_ERROR("EXECUTE: Invalid instruction at address: %lx, instruction: %x", cpu->pc, inst);

    return -1;
}
