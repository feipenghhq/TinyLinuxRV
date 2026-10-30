/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`ifndef CPU_H
`define CPU_H

`include "riscv.svh"

// ---------------------------------
// ARCH
// ---------------------------------

// RISC-V 64 bit
`define XLEN            64

// Register Number
`define REG_NUM         32

// PC reset number
`define PC_RESET_VEC    64'h80000000


// ---------------------------------
// AHB-Lite Bus
// ---------------------------------

typedef struct packed {
    logic [`XLEN-1:0] haddr;
    logic [1:0]       htrans;
    logic             hwrite;
    logic [2:0]       hsize;
    logic [`XLEN-1:0] hwdata;
    logic             hsel;
} AHBLiteReq_t;

typedef struct packed {
    logic [`XLEN-1:0] hrdata;
    logic             hready;
    logic             hresp;
} AHBLiteRsp_t;


// ---------------------------------
// CPU control
// ---------------------------------

// ALU opcode
// bit[2:0] - Use the same encoding with the func3 field to encode ALU operation
// bit[3]   - Used to identify ADD/SUB and SRL/SRA which has the same func3 encoding
`define ALU_OP_RANGE 3:0
typedef enum logic[`ALU_OP_RANGE]{
    ALU_ADD  = 4'b0000,
    ALU_SUB  = 4'b1000,
    ALU_SLL  = 4'b0001,
    ALU_SLT  = 4'b0010,
    ALU_SLTU = 4'b0011,
    ALU_XOR  = 4'b0100,
    ALU_SRL  = 4'b0101,
    ALU_SRA  = 4'b1101,
    ALU_OR   = 4'b0110,
    ALU_AND  = 4'b0111
} ALU_OP_t;

// ALU Control
typedef struct packed {
    logic    alu_op1_sel_zero;  // for LUI
    logic    alu_op1_sel_pc;    // for AUIPC
    logic    alu_op2_sel_imm;
    ALU_OP_t alu_opcode;
    logic    alu_32bit;         // indicate 32 bit operation (XXXW)
} ALU_CTRL_t;

// MUL/DIV Control
typedef struct packed {
    logic       mul;
    logic       div;
    logic [1:0] opcode;
    logic       is_32bit; // indicate 32 bit operation (XXXW)
} MUL_DIV_CTRL_t;


// Memory opcode
// Encoding is same in instruction func3 field
// bit 2 indicate signed/unsigned
`define MEM_OP_RANGE   2:0

typedef enum logic[1:0] {
    MEM_BYTE  = 2'b00,
    MEM_HALF  = 2'b01,
    MEM_WORD  = 2'b10,
    MEM_DWORD = 2'b11
} MEM_OP_t;

// Memory Control
typedef struct packed {
    logic       mem_read;
    logic       mem_write;
    logic [2:0] mem_opcode;
} MEM_CTRL_t;

// Branch opcode
// Same as Func3 encoding
`define BRANCH_OP_RANGE    2:0

// Jump Control
typedef struct packed {
    logic       jal;
    logic       jalr;
    logic       branch;
    logic [2:0] branch_opcode;
} JUMP_CTRL_t;

// ---------------------------------
// Pipeline Stages
// ---------------------------------

// IF/ID
typedef struct packed {
    logic [`XLEN-1:0] pc;
} IF2ID_PIPE_t;

// ID/EX
typedef struct packed {
    logic [`XLEN-1:0] pc;
    logic [31:0]      instruction;
    logic [`XLEN-1:0] immediate;
    ALU_CTRL_t        alu_ctrl;
    MUL_DIV_CTRL_t    mul_div_ctrl;
    MEM_CTRL_t        mem_ctrl;
    JUMP_CTRL_t       jump_ctrl;
    logic [`XLEN-1:0] rs1_value;
    logic [`XLEN-1:0] rs2_value;
    logic             rd_write;
    logic [4:0]       rd_addr;
    // exception
    logic       exception_valid;
    logic [4:0] exception_cause;
} ID2EX_PIPE_t;

// EX/MEM
typedef struct packed {
    logic [`XLEN-1:0] pc;
    logic [31:0]      instruction;
    MEM_CTRL_t        mem_ctrl;
    logic [2:0]       haddr;
    logic [`XLEN-1:0] rs2_value; // for write data
    logic             rd_write;
    logic [4:0]       rd_addr;
    logic [`XLEN-1:0] alu_out; // for write data
    // exception
    logic       exception_valid;
    logic [4:0] exception_cause;
} EX2MEM_PIPE_t;

// MEM/WB
typedef struct packed {
    logic [`XLEN-1:0] pc;
    logic [31:0]      instruction;
    logic             rd_write;
    logic [4:0]       rd_addr;
    logic [`XLEN-1:0] rd_wdata;
    // exception
    logic       exception_valid;
    logic [4:0] exception_cause;
} MEM2WB_PIPE_t;

`endif
