/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module EX (
    input                clk,
    input                rst_n,
    // pipeline control
    input                in_ID_valid,
    input ID2EX_PIPE_t   in_ID_data,
    output               in_ID_ready,
    output               out_EX_valid,
    output EX2MEM_PIPE_t out_EX_data,
    input                out_EX_ready,
    input                EX_flush,
    // LSU request
    output               lsu_req_read,
    output               lsu_req_write,
    output               lsu_req_lr,
    output               lsu_req_sc,
    output               lsu_req_amo,
    input                lsu_req_ready,
    output [`XLEN-1:0]   lsu_req_addr,
    output [`XLEN-1:0]   lsu_req_wdata,
    output [2:0]         lsu_req_mem_opcode,
    output [4:0]         lsu_req_amo_opcode,
    output               lsu_req_is32bit,
    // LSU status
    input                sc_check_pass,
    input                load_addr_misaligned,
    input                store_amo_addr_misaligned,
    // AMO read data
    input  [`XLEN-1:0]   amo_rdata,
    // Jump
    output [`XLEN-1:0]   jump_pc,
    output               jump,
    // Data forwarding logic
    output [4:0]         ex_rd_addr,
    output               ex_rd_write,
    output [`XLEN-1:0]   ex_rd_wdata,
    // Load dependency check
    output               ex_mem_read,
    output               ex_lr
);

    logic [`XLEN-1:0] alu_op1;
    logic [`XLEN-1:0] alu_op2;
    logic [`XLEN-1:0] alu_out;
    logic [`XLEN-1:0] mul_out;
    logic [`XLEN-1:0] div_out;

    logic lsu_stall;
    logic mul_stall;
    logic div_stall;
    logic EX_stall;

    // exception
    logic instr_addr_misaligned;

    // ---------------------------------
    // Main components
    // ---------------------------------

    alu u_alu (
        .alu_op1    (alu_op1),
        .alu_op2    (alu_op2),
        .alu_opcode (in_ID_data.cpu_ctrl.opcode[3:0]),
        .alu_32bit  (in_ID_data.cpu_ctrl.is_32bit),
        .alu_out    (alu_out)
    );

    multiplier u_multiplier (
        .clk      (clk),
        .rst_n    (rst_n),
        .req      (in_ID_data.cpu_ctrl.mul),
        .opcode   (in_ID_data.cpu_ctrl.opcode[1:0]),
        .is_32bit (in_ID_data.cpu_ctrl.is_32bit),
        .a        (alu_op1),      // rs0
        .b        (alu_op2),      // rs2
        .o        (mul_out),      // rd
        .stall    (mul_stall)
    );

    divider u_divider (
        .clk      (clk),
        .rst_n    (rst_n),
        .flush    (1'b0),         // FIXME: Need to add flush
        .req      (in_ID_data.cpu_ctrl.div),
        .opcode   (in_ID_data.cpu_ctrl.opcode[1:0]),
        .is_32bit (in_ID_data.cpu_ctrl.is_32bit),
        .a        (alu_op1),      // rs0
        .b        (alu_op2),      // rs2
        .o        (div_out),      // rd
        .stall    (div_stall)
    );

    bu u_bu (
        .EX_flush   (EX_flush),
        .jal        (in_ID_data.cpu_ctrl.jal),
        .jalr       (in_ID_data.cpu_ctrl.jalr),
        .branch     (in_ID_data.cpu_ctrl.branch),
        .branch_opcode(in_ID_data.cpu_ctrl.opcode[2:0]),
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

    assign alu_op1 = in_ID_data.cpu_ctrl.alu_op1_sel_zero ? 0 :
                     in_ID_data.cpu_ctrl.alu_op1_sel_pc   ? in_ID_data.pc :
                     in_ID_data.rs1_value;

    assign alu_op2 = in_ID_data.cpu_ctrl.alu_op2_sel_imm ? in_ID_data.immediate :
                     in_ID_data.rs2_value;

    // ---------------------------------
    // LSU request
    // ---------------------------------

    assign lsu_req_read  = ~EX_flush & in_ID_data.cpu_ctrl.mem_read;
    assign lsu_req_write = ~EX_flush & in_ID_data.cpu_ctrl.mem_write;
    assign lsu_req_lr    = ~EX_flush & in_ID_data.cpu_ctrl.lr;
    assign lsu_req_sc    = ~EX_flush & in_ID_data.cpu_ctrl.sc;
    assign lsu_req_amo   = ~EX_flush & in_ID_data.cpu_ctrl.amo;

    // Dedicated address generation for memory access
    assign lsu_req_addr    = (in_ID_data.cpu_ctrl.amo | in_ID_data.cpu_ctrl.lr | in_ID_data.cpu_ctrl.sc) ?
                            in_ID_data.rs1_value : in_ID_data.rs1_value + in_ID_data.immediate;;
    assign lsu_req_wdata   = in_ID_data.rs2_value;
    assign lsu_req_mem_opcode  = in_ID_data.cpu_ctrl.mem_opcode;
    assign lsu_req_amo_opcode  = in_ID_data.cpu_ctrl.opcode;
    assign lsu_req_is32bit = in_ID_data.cpu_ctrl.is_32bit;

    // lsu stall request in EX stage
    assign lsu_stall = (lsu_req_read | lsu_req_write | lsu_req_lr | lsu_req_sc | lsu_req_amo) & ~lsu_req_ready;

    // ---------------------------------
    // Data forwarding logic
    // ---------------------------------

    assign ex_mem_read = in_ID_data.cpu_ctrl.mem_read | in_ID_data.cpu_ctrl.lr | in_ID_data.cpu_ctrl.amo;
    assign ex_lr = in_ID_data.cpu_ctrl.lr;

    assign ex_rd_addr  = in_ID_data.rd_addr;
    assign ex_rd_write = in_ID_data.rd_write;

    // final EX stage rd wdata (not considering the jump)
    assign ex_rd_wdata = in_ID_data.cpu_ctrl.sc ? {{`XLEN-1{1'b0}}, ~sc_check_pass} :
                        in_ID_data.cpu_ctrl.amo ? amo_rdata :
                        in_ID_data.cpu_ctrl.mul ? mul_out :
                        in_ID_data.cpu_ctrl.div ? div_out :
                        alu_out;

    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    assign EX_stall = lsu_stall | mul_stall | div_stall;

    assign in_ID_ready = out_EX_ready & ~EX_stall;

    assign out_EX_valid = in_ID_valid & ~EX_flush;

    assign out_EX_data.pc          = in_ID_data.pc;
    assign out_EX_data.mem_access  = lsu_req_read | lsu_req_write | lsu_req_lr | lsu_req_sc;
    assign out_EX_data.mem_read    = lsu_req_read | lsu_req_lr;
    assign out_EX_data.rd_write    = in_ID_data.rd_write;
    assign out_EX_data.rd_addr     = in_ID_data.rd_addr;
    // for jal/jalr, store pc + 4 to rd
    assign out_EX_data.rd_wdata    = jump ? in_ID_data.pc + 4 : ex_rd_wdata;

    // exception
    assign out_EX_data.exception_valid = in_ID_data.exception_valid | instr_addr_misaligned |
                                        load_addr_misaligned | store_amo_addr_misaligned;

    assign out_EX_data.exception_cause = in_ID_data.exception_valid ? in_ID_data.exception_cause :
                                        instr_addr_misaligned ? INST_ADDR_MISALIGNED :
                                        load_addr_misaligned  ? LOAD_ADDR_MISALIGNED :
                                        STORE_AMO_ADDR_MISALIGNED;

endmodule
