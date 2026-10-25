/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */


`include "cpu.svh"

module cpu (
    input clk,
    input rst_n,

    input strap_block_fetch_enable,

    output AHBLiteReq_t inst_memory_req,
    input  AHBLiteRsp_t inst_memory_rsp,

    output AHBLiteReq_t data_memory_req,
    input  AHBLiteRsp_t data_memory_rsp
);

    logic        out_IF_valid;
    logic        out_IF_ready;
    IF2ID_PIPE_t out_IF_data;

    logic        IF_ID_valid;
    logic        IF_ID_ready;
    IF2ID_PIPE_t IF_ID_data;

    logic        out_ID_valid;
    logic        out_ID_ready;
    ID2EX_PIPE_t out_ID_data;

    logic        ID_EX_valid;
    logic        ID_EX_ready;
    ID2EX_PIPE_t ID_EX_data;

    logic         out_EX_valid;
    logic         out_EX_ready;
    EX2MEM_PIPE_t out_EX_data;

    logic         EX_MEM_valid;
    logic         EX_MEM_ready;
    EX2MEM_PIPE_t EX_MEM_data;

    logic         out_MEM_valid;
    logic         out_MEM_ready;
    MEM2WB_PIPE_t out_MEM_data;

    logic         MEM_WB_valid;
    logic         MEM_WB_ready;
    MEM2WB_PIPE_t MEM_WB_data;

    logic [`XLEN-1:0] jump_pc;
    logic             jump;

    logic [4:0]       ex_rd_addr;
    logic             ex_rd_write;
    logic [`XLEN-1:0] ex_rd_wdata;
    logic             ex_mem_read;
    logic             block_fetch;

    logic [4:0]       mem_rd_addr;
    logic             mem_rd_write;
    logic [`XLEN-1:0] mem_rd_wdata;

    logic [`XLEN-1:0] MEM_hwdata;

    logic               wb_rd_write;
    logic [4:0]         wb_rd_addr;
    logic [`XLEN-1:0]   wb_rd_wdata;

    /*AUTOWIRE*/

    IF u_IF (
        .clk                      (clk),
        .rst_n                    (rst_n),
        .IF_flush                 (1'b0),
        .out_IF_valid             (out_IF_valid),
        .out_IF_data              (out_IF_data),
        .out_IF_ready             (out_IF_ready),
        .strap_block_fetch_enable (strap_block_fetch_enable),
        .jump                     (jump),
        .jump_pc                  (jump_pc),
        .trap                     (1'b0),
        .trap_pc                  (64'b0),
        .block_fetch              (block_fetch),
        .memory_req               (inst_memory_req),
        .hready                   (inst_memory_rsp.hready)
    );

    pipeline #(
        .WIDTH($bits(IF2ID_PIPE_t))
    ) u_IF_ID_pipeline (
        .clk       (clk),
        .rst_n     (rst_n),
        .in_valid  (out_IF_valid),
        .in_ready  (out_IF_ready),
        .in_data   (out_IF_data),
        .out_valid (IF_ID_valid),
        .out_ready (IF_ID_ready),
        .out_data  (IF_ID_data)
    );

    ID u_ID (
        .clk          (clk),
        .rst_n        (rst_n),
        .in_IF_valid  (IF_ID_valid),
        .in_IF_data   (IF_ID_data),
        .in_IF_ready  (IF_ID_ready),
        .out_ID_valid (out_ID_valid),
        .out_ID_data  (out_ID_data),
        .out_ID_ready (out_ID_ready),
        .ID_flush     (1'b0),
        .memory_rsp   (inst_memory_rsp),
        .ex_rd_addr   (ex_rd_addr),
        .ex_rd_write  (ex_rd_write),
        .ex_rd_wdata  (ex_rd_wdata),
        .mem_rd_addr  (mem_rd_addr),
        .mem_rd_write (mem_rd_write),
        .mem_rd_wdata (mem_rd_wdata),
        .ex_mem_read  (ex_mem_read),
        .wb_rd_write  (wb_rd_write),
        .wb_rd_addr   (wb_rd_addr),
        .wb_rd_wdata  (wb_rd_wdata)
    );

    pipeline #(
        .WIDTH($bits(ID2EX_PIPE_t))
    ) u_ID_EX_pipeline (
        .clk       (clk),
        .rst_n     (rst_n),
        .in_valid  (out_ID_valid),
        .in_ready  (out_ID_ready),
        .in_data   (out_ID_data),
        .out_valid (ID_EX_valid),
        .out_ready (ID_EX_ready),
        .out_data  (ID_EX_data)
    );

    EX u_EX (
        // pipeline control
        .in_ID_valid  (ID_EX_valid),
        .in_ID_data   (ID_EX_data),
        .in_ID_ready  (ID_EX_ready),
        .out_EX_valid (out_EX_valid),
        .out_EX_data  (out_EX_data),
        .out_EX_ready (out_EX_ready),
        .EX_flush     (1'b0),
        .memory_req   (data_memory_req),
        .MEM_hwdata   (MEM_hwdata),
        .hready       (data_memory_rsp.hready),
        .jump_pc      (jump_pc),
        .jump         (jump),
        .ex_rd_addr   (ex_rd_addr),
        .ex_rd_write  (ex_rd_write),
        .ex_rd_wdata  (ex_rd_wdata),
        .ex_mem_read  (ex_mem_read),
        .block_fetch  (block_fetch)
    );

    pipeline #(
        .WIDTH($bits(EX2MEM_PIPE_t))
    ) u_EX_MEM_pipeline (
        .clk       (clk),
        .rst_n     (rst_n),
        .in_valid  (out_EX_valid),
        .in_ready  (out_EX_ready),
        .in_data   (out_EX_data),
        .out_valid (EX_MEM_valid),
        .out_ready (EX_MEM_ready),
        .out_data  (EX_MEM_data)
    );

    MEM u_MEM(
        .in_EX_valid   (EX_MEM_valid),
        .in_EX_data    (EX_MEM_data),
        .in_EX_ready   (EX_MEM_ready),
        .out_MEM_valid (out_MEM_valid),
        .out_MEM_data  (out_MEM_data),
        .out_MEM_ready (out_MEM_ready),
        .MEM_flush     (1'b0),
        .memory_rsp    (data_memory_rsp),
        .MEM_hwdata    (MEM_hwdata),
        .mem_rd_addr   (mem_rd_addr),
        .mem_rd_write  (mem_rd_write),
        .mem_rd_wdata  (mem_rd_wdata)
    );

    pipeline #(
        .WIDTH($bits(MEM2WB_PIPE_t))
    ) u_MEM_WB_pipeline (
        .clk       (clk),
        .rst_n     (rst_n),
        .in_valid  (out_MEM_valid),
        .in_ready  (out_MEM_ready),
        .in_data   (out_MEM_data),
        .out_valid (MEM_WB_valid),
        .out_ready (MEM_WB_ready),
        .out_data  (MEM_WB_data)
    );

    WB u_WB (
        .in_MEM_valid (MEM_WB_valid),
        .in_MEM_data  (MEM_WB_data),
        .in_MEM_ready (MEM_WB_ready),
        .wb_rd_write  (wb_rd_write),
        .wb_rd_addr   (wb_rd_addr),
        .wb_rd_wdata  (wb_rd_wdata)
    );


endmodule

// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-lists: nil
// verilog-cexp-indent: 4
// indent-tabs-mode: nil
// End:
