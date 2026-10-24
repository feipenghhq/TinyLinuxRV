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

// Decoder related defines




// Logic Instruction Func3
`define DEC_LOGIC_ADD       3'b000
`define DEC_LOGIC_SUB       3'b000
`define DEC_LOGIC_SLL       3'b001
`define DEC_LOGIC_SLT       3'b010
`define DEC_LOGIC_SLTU      3'b011
`define DEC_LOGIC_XOR       3'b100
`define DEC_LOGIC_SRL       3'b101
`define DEC_LOGIC_SRA       3'b101
`define DEC_LOGIC_OR        3'b110
`define DEC_LOGIC_AND       3'b111

// Branch Instruction Func3
`define DEC_BRANCH_BEQ      3'b000
`define DEC_BRANCH_BNE      3'b001
`define DEC_BRANCH_BLT      3'b100
`define DEC_BRANCH_BGE      3'b101
`define DEC_BRANCH_BLTU     3'b110
`define DEC_BRANCH_BGEU     3'b111

// RV32M Standard Extension
`define DEC_RV32M_MUL       3'b000
`define DEC_RV32M_MULH      3'b001
`define DEC_RV32M_MULHSU    3'b010
`define DEC_RV32M_MULHU     3'b011
`define DEC_RV32M_DIV       3'b100
`define DEC_RV32M_DIVU      3'b101
`define DEC_RV32M_REM       3'b110
`define DEC_RV32M_REMU      3'b111

// MRET
`define DEC_SYSTEM_MRET     25'b0011000000100000000000000

`endif
