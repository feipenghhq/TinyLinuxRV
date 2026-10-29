/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module EX (
    // pipeline control
    input                in_ID_valid,
    input ID2EX_PIPE_t   in_ID_data,
    output               in_ID_ready,
    output               out_EX_valid,
    output EX2MEM_PIPE_t out_EX_data,
    input                out_EX_ready,
    input                EX_flush,
    // AHB request
    output AHBLiteReq_t  memory_req,
    input [`XLEN-1:0]    MEM_hwdata, // write data from memory
    input                hready,
    // Jump
    output [`XLEN-1:0]   jump_pc,
    output               jump,
    // Data forwarding logic
    output [4:0]         ex_rd_addr,
    output               ex_rd_write,
    output [`XLEN-1:0]   ex_rd_wdata,
    // Load dependency check
    output               ex_mem_read,
    // block instruction fetch when there are memory request in EX
    output               block_fetch
);

    logic [`XLEN-1:0] alu_op1;
    logic [`XLEN-1:0] alu_op2;
    logic [`XLEN-1:0] alu_out;

    logic instr_addr_misaligned;

    logic mem_addr_misaligned;
    logic load_addr_misaligned;
    logic store_amo_addr_misaligned;

    logic [2:0]       hsize;
    logic [`XLEN-1:0] haddr;
    logic             mem_request_raw;
    logic             mem_request;

    logic             EX_stall;

    // ---------------------------------
    // Main components
    // ---------------------------------

    alu u_alu (
        .alu_op1    (alu_op1),
        .alu_op2    (alu_op2),
        .alu_opcode (in_ID_data.alu_ctrl.alu_opcode),
        .alu_32bit  (in_ID_data.alu_ctrl.alu_32bit),
        .alu_out    (alu_out)
    );

    bu u_bu (
        .EX_flush   (EX_flush),
        .jump_ctrl  (in_ID_data.jump_ctrl),
        .rs1_value  (in_ID_data.rs1_value),
        .rs2_value  (in_ID_data.rs2_value),
        .immediate  (in_ID_data.immediate),
        .pc         (in_ID_data.pc),
        .jump_pc    (jump_pc),
        .jump       (jump),
        .instr_addr_misaligned(instr_addr_misaligned)
    );

    // ---------------------------------
    // ALU operand select
    // ---------------------------------

    assign alu_op1 = in_ID_data.alu_ctrl.alu_op1_sel_zero ? 0 :
                     in_ID_data.alu_ctrl.alu_op1_sel_pc   ? in_ID_data.pc :
                     in_ID_data.rs1_value;

    assign alu_op2 = in_ID_data.alu_ctrl.alu_op2_sel_imm ? in_ID_data.immediate :
                     in_ID_data.rs2_value;

    // ---------------------------------
    // Data memory control
    // ---------------------------------

    // Dedicated AGU for memory access
    assign haddr = in_ID_data.rs1_value + in_ID_data.immediate;

    // HSIZE and addr alignment check
    always_comb begin
        mem_addr_misaligned = 0;
        hsize = 0;
        case (in_ID_data.mem_ctrl.mem_opcode[1:0])
            MEM_BYTE: begin
                hsize = 0;
            end
            MEM_HALF: begin
                hsize = 1;
                mem_addr_misaligned = haddr[0] != 0;
            end
            MEM_WORD: begin
                hsize = 2;
                mem_addr_misaligned = haddr[1:0] != 0;
            end
            MEM_DWORD: begin
                hsize = 3;
                mem_addr_misaligned = haddr[2:0] != 0;
            end
        endcase
    end

    assign load_addr_misaligned      = ~EX_flush & in_ID_data.mem_ctrl.mem_read  & mem_addr_misaligned;
    assign store_amo_addr_misaligned = ~EX_flush & in_ID_data.mem_ctrl.mem_write & mem_addr_misaligned;

    assign mem_request_raw = ~EX_flush & ~mem_addr_misaligned & (in_ID_data.mem_ctrl.mem_read | in_ID_data.mem_ctrl.mem_write);
    assign mem_request     = mem_request_raw & hready;
    assign block_fetch     = mem_request_raw;

    // AHBLite request
    assign memory_req.haddr  = haddr;
    assign memory_req.htrans = {mem_request, 1'b0};
    assign memory_req.hwrite = in_ID_data.mem_ctrl.mem_write;
    assign memory_req.hsize  = hsize;
    assign memory_req.hwdata = MEM_hwdata; // data is from MEM stage
    assign memory_req.hsel   = 1'b1;

    // ---------------------------------
    // Other logic
    // ---------------------------------

    assign ex_mem_read = in_ID_data.mem_ctrl.mem_read;

    assign ex_rd_addr  = in_ID_data.rd_addr;
    assign ex_rd_write = in_ID_data.rd_write;
    assign ex_rd_wdata = alu_out;

    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    assign EX_stall = mem_request_raw & ~hready;

    assign in_ID_ready = out_EX_ready & ~EX_stall;

    assign out_EX_valid = in_ID_valid & ~EX_flush;

    assign out_EX_data.pc          = in_ID_data.pc;
    assign out_EX_data.instruction = in_ID_data.instruction;
    assign out_EX_data.mem_ctrl    = in_ID_data.mem_ctrl;
    assign out_EX_data.haddr       = haddr[2:0];
    assign out_EX_data.rs2_value   = in_ID_data.rs2_value;
    assign out_EX_data.rd_write    = in_ID_data.rd_write;
    assign out_EX_data.rd_addr     = in_ID_data.rd_addr;
    // for jal/jalr, store pc + 4 to rd
    assign out_EX_data.alu_out     = jump ? in_ID_data.pc + 4 : alu_out;

    // exception
    assign out_EX_data.exception_valid = in_ID_data.exception_valid |
                                         instr_addr_misaligned | load_addr_misaligned | store_amo_addr_misaligned;
    assign out_EX_data.exception_cause = in_ID_data.exception_valid ? in_ID_data.exception_cause :
                                         instr_addr_misaligned      ? INST_ADDR_MISALIGNED :
                                         load_addr_misaligned       ? LOAD_ADDR_MISALIGNED :
                                                                      STORE_AMO_ADDR_MISALIGNED;

endmodule
