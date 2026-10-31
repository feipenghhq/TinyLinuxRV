/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module ID (
    input clk,
    input rst_n,

    // pipeline control
    input               in_IF_valid,
    input IF2ID_PIPE_t  in_IF_data,
    output              in_IF_ready,
    output              out_ID_valid,
    output ID2EX_PIPE_t out_ID_data,
    input               out_ID_ready,
    input               ID_flush,
    // AHB response
    input AHBLiteRsp_t  memory_rsp,
    // Data forwarding logic
    input [4:0]         ex_rd_addr,
    input               ex_rd_write,
    input [`XLEN-1:0]   ex_rd_wdata,
    input [4:0]         mem_rd_addr,
    input               mem_rd_write,
    input [`XLEN-1:0]   mem_rd_wdata,
    // load dependency check
    input               ex_mem_read,
    // Register write back
    input               wb_rd_write,
    input [4:0]         wb_rd_addr,
    input [`XLEN-1:0]   wb_rd_wdata
);

    CPU_CTRL_t        cpu_ctrl;

    logic             rd_write;
    logic [4:0]       rd_addr;
    logic             rs1_read;
    logic [4:0]       rs1_addr;
    logic [`XLEN-1:0] rs1_rdata;
    logic             rs2_read;
    logic [4:0]       rs2_addr;
    logic [`XLEN-1:0] rs2_rdata;

    logic             instruction_access_fault;
    logic             instruction_pre_valid;
    logic             instruction_valid;
    logic [31:0]      instruction;
    logic [31:0]      instruction_buffer;
    logic [31:0]      instruction_post;
    logic             illegal_instruction;
    logic [`XLEN-1:0] immediate;

    logic             rs1_nonzero;
    logic             rs1_match_ex;
    logic             rs1_match_mem;
    logic             rs2_nonzero;
    logic             rs2_match_ex;
    logic             rs2_match_mem;
    logic [`XLEN-1:0] final_rs1;
    logic [`XLEN-1:0] final_rs2;

    logic             load_dependency;
    logic             ID_stage_valid;
    logic             ID_stage_stalled;

    // ---------------------------------
    // Main components
    // ---------------------------------

    // instruction skip buffer
    skid_buffer #(.WIDTH(32))
    u_inst_skid_buffer(
        .clk        (clk),
        .rst_n      (rst_n),
        .in_valid   (instruction_pre_valid),
        .in_data    (memory_rsp.hrdata[31:0]),
        /* verilator lint_off PINCONNECTEMPTY */
        .in_ready   (),
        /* verilator lint_on PINCONNECTEMPTY */
        .out_valid  (instruction_valid),
        .out_data   (instruction),
        .out_ready  (in_IF_ready)
    );

    // decoder
    decoder u_decoder(
        .instruction (instruction_post),
        .rd_write    (rd_write),
        .rd_addr     (rd_addr),
        .rs1_read    (rs1_read),
        .rs1_addr    (rs1_addr),
        .rs2_read    (rs2_read),
        .rs2_addr    (rs2_addr),
        .cpu_ctrl    (cpu_ctrl),
        .immediate   (immediate),
        .illegal_instruction(illegal_instruction)
    );

    // register file
    regfile u_regfile(
        .clk        (clk),
        .reg_write  (wb_rd_write),
        .reg_addr   (wb_rd_addr),
        .reg_wdata  (wb_rd_wdata),
        .rs1_addr   (rs1_addr),
        .rs1_value  (rs1_rdata),
        .rs2_addr   (rs2_addr),
        .rs2_value  (rs2_rdata)
    );

    // ---------------------------------
    // AHBLite Resp Control
    // ---------------------------------

    // instruction from memory is valid when:
    //  1. IF stage is valid & HREADY=1 (meaning IF has sent request and the read data is back)
    //  2. AHB-Lite response is OK (HRESP = 0)
    //  3. ID stage is not flushed
    assign instruction_pre_valid    = in_IF_valid & memory_rsp.hready & ~memory_rsp.hresp & ~ID_flush;
    assign instruction_access_fault = in_IF_valid & memory_rsp.hready &  memory_rsp.hresp & ~ID_flush;

    always_ff @(posedge clk) begin
        ID_stage_stalled <= in_IF_valid & ~in_IF_ready;
        if (!ID_stage_stalled) instruction_buffer <= instruction;
    end
    assign instruction_post = ID_stage_stalled ? instruction_buffer : instruction;

    // ---------------------------------
    // Data forwarding MUX
    // ---------------------------------

    assign rs1_nonzero   = (rs1_addr != 0);
    assign rs2_nonzero   = (rs2_addr != 0);
    assign rs1_match_ex  = rs1_nonzero & rs1_read & (rs1_addr == ex_rd_addr)  & ex_rd_write;
    assign rs2_match_ex  = rs2_nonzero & rs2_read & (rs2_addr == ex_rd_addr)  & ex_rd_write;
    assign rs1_match_mem = rs1_nonzero & rs1_read & (rs1_addr == mem_rd_addr) & mem_rd_write;
    assign rs2_match_mem = rs2_nonzero & rs2_read & (rs2_addr == mem_rd_addr) & mem_rd_write;

    assign final_rs1 = rs1_match_ex  ? ex_rd_wdata  :
                       rs1_match_mem ? mem_rd_wdata :
                       rs1_rdata;
    assign final_rs2 = rs2_match_ex  ? ex_rd_wdata  :
                       rs2_match_mem ? mem_rd_wdata :
                       rs2_rdata;

    // ---------------------------------
    // Load dependency check
    // ---------------------------------

    // When EX stage is a Load instruction and ID stage instruction depends on the result of the load:
    // - We cannot forward a result from Load instruction from EX stage to ID stage as the read data is not ready yet in EX
    // - We need to Stall ID stage for 1 cycle to let the memory read instruction goto the MEM stage to get the data ready
    assign load_dependency = ID_stage_valid & (rs1_match_ex | rs2_match_ex) & ex_mem_read;

    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    // ID stage itself is valid when: 1) Get valid instruction, 2) Instruction is decoded successfully, 3) Not flushed
    assign ID_stage_valid = instruction_valid & ~illegal_instruction & ~ID_flush;

    // ID stage is ready when 1) Instruction is valid, 2) EX stage is ready, 3) No load dependency
    // On an ID flush, release the stall caused by waiting for the instruction memory response (HREADY=1)
    // When ID is flushed, IF is also flushed as well, so IF/ID valid will be in cleared next cycle.
    // IF continues inserting bubbles into ID while waiting for the HREADY=1.
    // IF/ID valid therefore remains zero when the response arrives, and the instruction is discarded.
    assign in_IF_ready = instruction_valid & out_ID_ready & ~load_dependency | ID_flush;

    // ID is valid when 1）ID stage is valid, 2) not stalled due to load_dependency
    assign out_ID_valid = ID_stage_valid & ~load_dependency;

    // ctrl and data
    assign out_ID_data.pc          = in_IF_data.pc;
    assign out_ID_data.instruction = instruction;
    assign out_ID_data.immediate   = immediate;
    assign out_ID_data.cpu_ctrl    = cpu_ctrl;
    assign out_ID_data.rs1_value   = final_rs1;
    assign out_ID_data.rs2_value   = final_rs2;
    assign out_ID_data.rd_write    = rd_write;
    assign out_ID_data.rd_addr     = rd_addr;

    // exception
    assign out_ID_data.exception_valid = instruction_access_fault | illegal_instruction;
    assign out_ID_data.exception_cause = instruction_access_fault ? INST_ACCESS_FAULT : ILLEGAL_INSTRUCTION;

endmodule
