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
    input  AHBLiteRsp_t inst_memory_rsp
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

    /*AUTOWIRE*/

    IF u_IF (
        .clk                     (clk),
        .rst_n                   (rst_n),
        .IF_flush                (1'b0),
        .out_IF_valid            (out_IF_valid),
        .out_IF_data             (out_IF_data),
        .out_IF_ready            (out_IF_ready),
        .strap_block_fetch_enable(strap_block_fetch_enable),
        .jump                    (1'b0),
        .jump_pc                 ('b0),
        .trap                    (1'b0),
        .trap_pc                 ('b0),
        .block_fetch             (1'b0),
        .memory_req              (inst_memory_req),
        .hready                  (inst_memory_rsp.hready)
    );

    pipeline #(
        .WIDTH($bits(IF2ID_PIPE_t))
    ) u_IF_ID_pipeline (
        .clk      (clk),
        .rst_n    (rst_n),
        .in_valid (out_IF_valid),
        .in_ready (out_IF_ready),
        .in_data  (out_IF_data),
        .out_valid(IF_ID_valid),
        .out_ready(IF_ID_ready),
        .out_data (IF_ID_data)
    );

    ID u_ID (
        .clk         (clk),
        .rst_n       (rst_n),
        .in_IF_valid (IF_ID_valid),
        .in_IF_data  (IF_ID_data),
        .in_IF_ready (IF_ID_ready),
        .out_ID_valid(out_ID_valid),
        .out_ID_data (out_ID_data),
        .out_ID_ready(out_ID_ready),
        .ID_flush    (1'b0),
        .memory_rsp  (inst_memory_rsp),
        .ex_rd_addr  ('b0),
        .ex_rd_write ('b0),
        .ex_rd_wdata ('b0),
        .mem_rd_addr ('b0),
        .mem_rd_write('b0),
        .mem_rd_wdata('b0),
        .ex_mem_read ('b0),
        .wb_rd_write ('b0),
        .wb_rd_addr  ('b0),
        .wb_rd_wdata ('b0)
    );

    pipeline #(
        .WIDTH($bits(ID2EX_PIPE_t))
    ) u_ID_EX_pipeline (
        .clk      (clk),
        .rst_n    (rst_n),
        .in_valid (out_ID_valid),
        .in_ready (out_ID_ready),
        .in_data  (out_ID_data),
        .out_valid(),
        .out_ready(1'b0),
        .out_data ()
    );

endmodule

// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-lists: nil
// verilog-cexp-indent: 4
// indent-tabs-mode: nil
// End:
