

#include "cpu.h"
#include "memory.h"
#include "log.h"

// instruction field
typedef struct {
    uint8_t     opcode;
    uint8_t     rs1;
    uint8_t     rs2;
    uint8_t     rd;
    uint32_t    imm;   // immediate number
} inst_dec_t;

/**
 * Decode an instruction to different field
 */
static void decode(uint32_t inst, inst_dec_t* inst_dec) {
    uint8_t opcode;
    opcode = inst & 0x7F;
    inst_dec->opcode = opcode;
    inst_dec->rs1 = (inst >> 15) & 0x1F;
    inst_dec->rs2 = (inst >> 20) & 0x1F;
    inst_dec->rd  = (inst >> 7)  & 0x1F;
    inst_dec->imm = 0;

    switch(opcode) {
        case(0x13): {inst_dec->imm = (inst >> 20) & 0xFFF;} // I-type immediate
        // TBD
    }
}

/**
 * Convert the pattern to mask
 */
static int parse_pattern(const char* pattern, uint32_t* mask, uint32_t* golden) {
    uint32_t _mask = 0;
    uint32_t _golden = 0;
    for (; *pattern != '\0'; pattern++) {
        switch (*pattern) {
            case(' '):  // fall-through case
            case('_'):  break;   // skip space and _
            case('?'): {
                _mask = (_mask << 1) | 0;
                _golden = (_golden << 1) | 0;
                break;
            }
            case('0'): {
                _mask = (_mask << 1) | 1;
                _golden = (_golden << 1) | 0;
                break;
            }
            case('1'): {
                _mask = (_mask << 1) | 1;
                _golden = (_golden << 1) | 1;
                break;
            }
            default: {
                LOG_ERROR("Incorrect pattern: %s", pattern);
                return -1;
            }
        }
    }
    *mask = _mask;
    *golden = _golden;
    return 0;
}

// Macro to add the new instruction, use in the big switch/case statement
#define ADD_INST(name, pattern, op)                 \
    do {                                            \
        uint32_t mask, golden;                      \
        if (parse_pattern(pattern, &mask, &golden) != 0) { \
            return -1;                              \
        }                                           \
        if ((mask & inst) == golden) {              \
            LOG_DEBUG("EXECUTE %s: PC=%lx INST=%08x", name, cpu->pc, inst); \
            op;                                     \
            cpu->regs[0] = 0;                       \
            return 0;                               \
        }                                           \
    } while(0)

// Macro to complete cpu operation for non-branch instruction
#define NON_BRANCH_OP(op) op; cpu->pc += 4

#define RD()  cpu->regs[inst_dec.rd]
#define RS1() cpu->regs[inst_dec.rs1]
#define RS2() cpu->regs[inst_dec.rs2]

/**
 * init the cpu
 */
void init_cpu(cpu_t* cpu) {
    cpu->halted = false;
    cpu->pc = RST_VEC;

    // initialize all the register to 0 to make sure the emulator is deterministic
    for (int i = 0; i < 32; i++) {
        cpu->regs[i] = 0;
    }
    LOG_INFO("Initialize CPU done");
}

/**
 * Execute a single instruction
 */
int execute(uint32_t inst, cpu_t* cpu, memory_t* memory) {
    inst_dec_t inst_dec;
    // decode the instruction
    decode(inst, &inst_dec);

    // R-type instruction
    ADD_INST("add", "0000_000?_????_????_?000_????_?011_0011", NON_BRANCH_OP(RD() = RS1() + RS2()));

    // check for ebreak. Currently using ebreak as a signal to stop the emulator
    ADD_INST("ebreak", "0000_0000_0001_0000_0000_0000_0111_0011", cpu->halted = true);

    // if program execute to this point, then we hit an invalid instruction
    cpu->halted = true;
    LOG_ERROR("EXECUTE: Invalid instruction at address: %lx, instruction: %x", cpu->pc, inst);
    return -1;
}
