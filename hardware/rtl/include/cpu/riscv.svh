/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`ifndef RISCV_H
`define RISCV_H

// ----------------------------------------------
// Instructions
// ----------------------------------------------

// Instruction Field
`define DEC_OPCODE_FIELD    6:0
`define DEC_RD_FIELD        11:7
`define DEC_FUNC3_FIELD     14:12
`define DEC_RS1_FIELD       19:15
`define DEC_RS2_FIELD       24:20
`define DEC_FUNC7_FIELD     31:25
`define DEC_CSR_ADDR_FIELD  31:20

// Instruction Field Range
`define DEC_OPCODE_RANGE    6:0
`define DEC_RD_RANGE        4:0
`define DEC_FUNC3_RANGE     2:0
`define DEC_RS1_RANGE       4:0
`define DEC_RS2_RANGE       4:0
`define DEC_FUNC7_RANGE     6:0
`define DEC_CSR_ADDR_FIELD  31:20

// Instruction Type (OPCODE)
typedef enum bit[6:0] {
    OPCODE_LOGIC_R64 = 7'b0110011,
    OPCODE_LOGIC_R32 = 7'b0111011,
    OPCODE_LOGIC_I64 = 7'b0010011,
    OPCODE_LOGIC_I32 = 7'b0011011,
    OPCODE_STORE     = 7'b0100011,
    OPCODE_LOAD      = 7'b0000011,
    OPCODE_BRANCH    = 7'b1100011,
    OPCODE_JALR      = 7'b1100111,
    OPCODE_JAL       = 7'b1101111,
    OPCODE_AUIPC     = 7'b0010111,
    OPCODE_LUI       = 7'b0110111,
    OPCODE_AMO       = 7'b0101111,
    OPCODE_SYSTEM    = 7'b1110011
} OPCODE_t;

// CSR opcode
// Same as Func3 encoding
`define CORE_CSR_ADDR_RANGE   11:0
`define CORE_CSR_OP_RANGE     1:0
`define CORE_CSR_NOP          2'b00
`define CORE_CSR_RW           2'b01
`define CORE_CSR_RS           2'b10
`define CORE_CSR_RC           2'b11

// ----------------------------------------------
// Exception
// ----------------------------------------------

typedef enum logic[4:0] {
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
    STORE_AMO_PAGE_FAULT      = 15
} exception_code_t;

`endif
