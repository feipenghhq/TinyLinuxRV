/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`ifndef CPU_H
`define CPU_H

`include "riscv.svh"

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

// Branch opcode
// Same as Func3 encoding
`define BRANCH_OP_RANGE    2:0

// CSR opcode
// Same as Func3 encoding
typedef enum logic[1:0] {
    CSR_NOP = 2'b00,
    CSR_RW  = 2'b01,
    CSR_RS  = 2'b10,
    CSR_RC  = 2'b11
} CSR_OP_t;


// Shared CPU control.
// For all instruction except for AMO
// - opcode[2:0] = func3;
// - opcode[3] = instruction[30] (selects SUB/SRA for ALU operations).
// For AMO
// - opcode[4:0] = instruction[31:27]
typedef struct packed {
    logic [4:0] opcode;
    logic       is_32bit;
    logic       alu_op1_sel_zero;
    logic       alu_op1_sel_pc;
    logic       use_immediate;
    logic       mul;
    logic       div;
    logic       jal;
    logic       jalr;
    logic       branch;
    logic       lr;
    logic       sc;
    logic       amo;
    logic [2:0] mem_opcode;
    logic       mem_read;
    logic       mem_write;
    logic       csr_read;
    logic       csr_write;
    logic [1:0] csr_write_opcode;
    logic [11:0] csr_address;
    logic       mret;
    logic       sret;
} CPU_CTRL_t;

// ---------------------------------
// Pipeline Stages
// ---------------------------------

// IF/ID
typedef struct packed {
    logic [`XLEN-1:0] pc;
    logic [31:0]      instruction;
    logic             inst_access_fault;
} IF2ID_PIPE_t;

// ID/EX
typedef struct packed {
    logic [`XLEN-1:0] pc;
    logic [31:0]      instruction;
    logic [`XLEN-1:0] immediate;
    CPU_CTRL_t        cpu_ctrl;
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
    logic             mem_read;
    logic             mem_access;
    logic             rd_write;
    logic [4:0]       rd_addr;
    logic [`XLEN-1:0] rd_wdata;
    logic             csr_read;
    logic             csr_write;
    logic [1:0]       csr_write_opcode;
    logic [11:0]      csr_address;
    logic [`XLEN-1:0] csr_wdata;
    logic             mret;
    logic             sret;
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
    logic             csr_read;
    logic             csr_write;
    logic [1:0]       csr_write_opcode;
    logic [11:0]      csr_address;
    logic [`XLEN-1:0] csr_wdata;
    logic             mret;
    logic             sret;
    // exception
    logic       exception_valid;
    logic [4:0] exception_cause;
} MEM2WB_PIPE_t;

`endif
