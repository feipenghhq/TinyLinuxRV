#include "cpu.h"
#include "decode.h"
#include "memory.h"
#include "log.h"

// Instruction Opcode
#define OPCODE_LUI 0x37
#define OPCODE_AUIPC 0x17
#define OPCODE_JAL 0x6F
#define OPCODE_JALR 0x67
#define OPCODE_BRANCH 0x63
#define OPCODE_LOAD 0x03
#define OPCODE_STORE 0x23
#define OPCODE_OP_IMM 0x13
#define OPCODE_OP_IMM_32 0x1B
#define OPCODE_OP 0x33
#define OPCODE_OP_32 0x3B
#define OPCODE_FENCE 0x0F
#define OPCODE_SYSTEM 0x73

// Fields shared by the instruction formats used by the executor.
typedef struct
{
    uint8_t opcode;
    uint8_t rs1;
    uint8_t rs2;
    uint8_t rd;
    uint64_t imm; // Sign-extended immediate stored as an RV64 bit pattern.
} inst_dec_t;

// Sign-extend the low "bits" bits of value to the 64-bit register width.
static inline uint64_t sext(uint64_t value, int bits)
{
    return (uint64_t)(((int64_t)(value << (64 - bits))) >> (64 - bits));
}

// RV64 W instructions always sign-extend their 32-bit result to XLEN.
static inline uint64_t sext32bit(uint64_t value)
{
    return sext(value, 32);
}

// Interpret an RV64 register bit pattern as a signed 64-bit value.
static inline int64_t signed64(uint64_t value)
{
    return (int64_t)value;
}

// RV64 shifts use only the low 6 bits of the shift amount.
static inline uint64_t sra64(uint64_t value, uint64_t shamt)
{
    return (uint64_t)(signed64(value) >> (shamt & UINT64_C(0x3F)));
}

static inline uint64_t srl64(uint64_t value, uint64_t shamt)
{
    return value >> (shamt & UINT64_C(0x3F));
}

static inline uint64_t sll64(uint64_t value, uint64_t shamt)
{
    return value << (shamt & UINT64_C(0x3F));
}

// RV64 W shifts operate on the low 32 bits, use a 5-bit shift amount,
// and sign-extend the 32-bit result back to the register width.
static inline uint64_t sra32(uint64_t value, uint64_t shamt)
{
    int32_t word = (int32_t)value;
    uint32_t result = (uint32_t)(word >> (shamt & UINT64_C(0x1F)));
    return sext32bit(result);
}

static inline uint64_t srl32(uint64_t value, uint64_t shamt)
{
    return sext32bit((uint32_t)(value) >> (shamt & UINT64_C(0x1F)));
}

static inline uint64_t sll32(uint64_t value, uint64_t shamt)
{
    return sext32bit((uint32_t)(value) << (shamt & UINT64_C(0x1F)));
}

// Extract register indices and reconstruct the immediate for each opcode type.
static void decode(uint32_t inst, inst_dec_t *inst_dec)
{
    uint8_t opcode;
    opcode = inst & 0x7F;
    inst_dec->opcode = opcode;
    inst_dec->rs1 = (inst >> 15) & 0x1F;
    inst_dec->rs2 = (inst >> 20) & 0x1F;
    inst_dec->rd = (inst >> 7) & 0x1F;
    inst_dec->imm = 0;

    switch (opcode)
    {
    case (OPCODE_LOAD):      // fall-through
    case (OPCODE_OP_IMM):    // fall-through
    case (OPCODE_OP_IMM_32): // fall-through
    case (OPCODE_JALR):
    { // I-type immediate
        inst_dec->imm = sext((inst >> 20) & 0xFFF, 12);
        break;
    }
    case (OPCODE_AUIPC): // fall-through
    case (OPCODE_LUI):
    {
        inst_dec->imm = inst & 0xFFFFF000;
        break;
    }
    case (OPCODE_STORE):
    { // S-type store
        inst_dec->imm = sext(((inst >> 25) << 5) | ((inst >> 7) & 0x1F), 12);
        break;
    }
    case (OPCODE_JAL):
    { // J-type
        inst_dec->imm = sext(
            (((uint64_t)(inst >> 31) & 0x1) << 20) |
                (((uint64_t)(inst >> 21) & 0x3FF) << 1) |
                (((uint64_t)(inst >> 20) & 0x1) << 11) |
                (((uint64_t)(inst >> 12) & 0xFF) << 12),
            21);
        break;
    }
    case (OPCODE_BRANCH):
    { // B-type
        inst_dec->imm = sext(
            (((uint64_t)(inst >> 31) & 0x1) << 12) |
                (((uint64_t)(inst >> 7) & 0x1) << 11) |
                (((uint64_t)(inst >> 25) & 0x3F) << 5) |
                (((uint64_t)(inst >> 8) & 0xF) << 1),
            13);
        break;
    }
    }
}

// Short aliases used by instruction bodies. These macros intentionally depend
// on the cpu and inst_dec local variables in cpu_execute().
#define PC() cpu->pc
#define RD() cpu->regs[inst_dec.rd]
#define RS1() cpu->regs[inst_dec.rs1]
#define RS2() cpu->regs[inst_dec.rs2]
#define IMM() inst_dec.imm

// Match and execute one instruction. A successful instruction commits next_pc,
// restores the architectural x0 invariant, and returns from cpu_execute().
#define ADD_INST(name, op)                                                  \
    do                                                                      \
    {                                                                       \
        if ((inst & MASK_##name) == GOLDEN_##name)                          \
        {                                                                   \
            LOG_DEBUG("EXECUTE %-8s: PC=%lx INST=%08x", #name, PC(), inst); \
            op;                                                             \
            cpu->regs[0] = 0;                                               \
            PC() = next_pc;                                                 \
            return 0;                                                       \
        }                                                                   \
    } while (0)

// Execute a load. ext selects either sign extension or no extension.
#define noext(value, bits) (value)
#define EXEC_LOAD(size, ext)                                      \
    do                                                            \
    {                                                             \
        uint64_t data = 0;                                        \
        if (memory_read(memory, RS1() + IMM(), size, &data) != 0) \
        {                                                         \
            cpu->halted = true;                                   \
            return -1;                                            \
        }                                                         \
        RD() = ext(data, size * 8);                               \
    } while (0)

// Execute a store using the low "size" bytes of rs2.
#define EXEC_STORE(size)                                            \
    do                                                              \
    {                                                               \
        const uint64_t value = RS2();                               \
        if (memory_write(memory, RS1() + IMM(), size, &value) != 0) \
        {                                                           \
            cpu->halted = true;                                     \
            return -1;                                              \
        }                                                           \
    } while (0)

/**
 * Initialize the CPU state to deterministic state.
 * Set PC to reset vector and clear registers to 0. Set halted to false.
 */
void cpu_init(cpu_t *cpu)
{
    cpu->halted = false;
    cpu->pc = RST_VEC;

    // initialize all the register to 0 to make sure the emulator is deterministic
    for (int i = 0; i < 32; i++)
    {
        cpu->regs[i] = 0;
    }
    LOG_INFO("Initialize CPU done");
}

/**
 * Execute a SINGLE instruction
 */
int cpu_execute(cpu_t *cpu, uint32_t inst, memory_t *memory)
{
    inst_dec_t inst_dec;
    uint64_t next_pc;

    // Decode once, then assume sequential execution unless a jump or a taken
    // branch replaces next_pc.
    decode(inst, &inst_dec);
    next_pc = PC() + 4;

    switch (inst_dec.opcode)
    {
    case OPCODE_LUI:
    {
        ADD_INST(LUI, RD() = sext(IMM(), 32));
        break;
    }
    case OPCODE_AUIPC:
    {
        ADD_INST(AUIPC, RD() = PC() + sext(IMM(), 32));
        break;
    }
    case OPCODE_JAL:
    {
        ADD_INST(JAL, RD() = PC() + 4; next_pc = PC() + sext(IMM(), 21));
        break;
    }
    case OPCODE_JALR:
    {
        ADD_INST(JALR, uint64_t target = (sext(IMM(), 12) + RS1()) & ~UINT64_C(1); RD() = next_pc; next_pc = target);
        break;
    }
    case OPCODE_BRANCH:
    {
        ADD_INST(BEQ, if (RS1() == RS2()) next_pc = PC() + IMM(););
        ADD_INST(BNE, if (RS1() != RS2()) next_pc = PC() + IMM(););
        ADD_INST(BLT, if (signed64(RS1()) < signed64(RS2())) next_pc = PC() + IMM(););
        ADD_INST(BGE, if (signed64(RS1()) >= signed64(RS2())) next_pc = PC() + IMM(););
        ADD_INST(BLTU, if (RS1() < RS2()) next_pc = PC() + IMM(););
        ADD_INST(BGEU, if (RS1() >= RS2()) next_pc = PC() + IMM(););
        break;
    }
    case OPCODE_LOAD:
    {
        ADD_INST(LB, EXEC_LOAD(1, sext));
        ADD_INST(LH, EXEC_LOAD(2, sext));
        ADD_INST(LW, EXEC_LOAD(4, sext));
        ADD_INST(LBU, EXEC_LOAD(1, noext));
        ADD_INST(LHU, EXEC_LOAD(2, noext));
        ADD_INST(LWU, EXEC_LOAD(4, noext));
        ADD_INST(LD, EXEC_LOAD(8, noext));
        break;
    }
    case OPCODE_STORE:
    {
        ADD_INST(SB, EXEC_STORE(1));
        ADD_INST(SH, EXEC_STORE(2));
        ADD_INST(SW, EXEC_STORE(4));
        ADD_INST(SD, EXEC_STORE(8));
        break;
    }
    case OPCODE_OP_IMM:
    {
        ADD_INST(ADDI, RD() = RS1() + IMM());
        ADD_INST(SLTI, if (signed64(RS1()) < signed64(IMM())) RD() = 1; else RD() = 0);
        ADD_INST(SLTIU, if (RS1() < IMM()) RD() = 1; else RD() = 0);
        ADD_INST(XORI, RD() = RS1() ^ IMM());
        ADD_INST(ORI, RD() = RS1() | IMM());
        ADD_INST(ANDI, RD() = RS1() & IMM());
        ADD_INST(SLLI, RD() = RS1() << (IMM() & 0x3F));
        ADD_INST(SRLI, RD() = RS1() >> (IMM() & 0x3F));
        ADD_INST(SRAI, RD() = sra64(RS1(), IMM()));
        break;
    }
    case OPCODE_OP_IMM_32:
    {
        ADD_INST(ADDIW, RD() = sext32bit(RS1() + IMM()));
        ADD_INST(SLLIW, RD() = sll32(RS1(), IMM()));
        ADD_INST(SRLIW, RD() = srl32(RS1(), IMM()));
        ADD_INST(SRAIW, RD() = sra32(RS1(), IMM()));
        break;
    }
    case OPCODE_OP:
    {
        ADD_INST(ADD, RD() = RS1() + RS2());
        ADD_INST(SUB, RD() = RS1() - RS2());
        ADD_INST(SLL, RD() = sll64(RS1(), RS2()));
        ADD_INST(SLT, RD() = signed64(RS1()) < signed64(RS2()));
        ADD_INST(SLTU, RD() = RS1() < RS2());
        ADD_INST(XOR, RD() = RS1() ^ RS2());
        ADD_INST(SRL, RD() = srl64(RS1(), RS2()));
        ADD_INST(SRA, RD() = sra64(RS1(), RS2()));
        ADD_INST(OR, RD() = RS1() | RS2());
        ADD_INST(AND, RD() = RS1() & RS2());
        break;
    }
    case OPCODE_OP_32:
    {
        ADD_INST(ADDW, RD() = sext32bit(RS1() + RS2()));
        ADD_INST(SUBW, RD() = sext32bit(RS1() - RS2()));
        ADD_INST(SLLW, RD() = sll32(RS1(), RS2()));
        ADD_INST(SRLW, RD() = srl32(RS1(), RS2()));
        ADD_INST(SRAW, RD() = sra32(RS1(), RS2()));
        break;
    }
    case OPCODE_FENCE:
    {
        ADD_INST(FENCE, );
        ADD_INST(FENCEI, );
        break;
    }
    case OPCODE_SYSTEM:
    {
        ADD_INST(ECALL, );
        // Treat ebreak as the temporary Phase 1 halt convention
        ADD_INST(EBREAK, cpu->halted = true);
        break;
    }
    default:
        break;
    }

    // Hit an invalid instruction if program execute this point.
    cpu->halted = true;
    LOG_ERROR("EXECUTE: Invalid instruction at address: %lx, instruction: %x", cpu->pc, inst);
    return -1;
}
