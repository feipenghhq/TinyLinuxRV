/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */


`include "cpu.svh"

module cpu (
    input clk,
    input rst_n,

    input strap_block_fetch_enable,

    output [`XLEN-1:0] inst_haddr,
    output [1:0]       inst_htrans,
    output             inst_hwrite,
    output [2:0]       inst_hsize,
    output [`XLEN-1:0] inst_hwdata,
    output             inst_hlock,
    input  [`XLEN-1:0] inst_hrdata,
    input              inst_hready,
    input              inst_hresp,

    output [`XLEN-1:0] data_haddr,
    output [1:0]       data_htrans,
    output             data_hwrite,
    output [2:0]       data_hsize,
    output [`XLEN-1:0] data_hwdata,
    output             data_hsel,
    output             data_hlock,
    input  [`XLEN-1:0] data_hrdata,
    input              data_hready,
    input              data_hresp
);


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
    logic             ex_lr;

    logic [4:0]       mem_rd_addr;
    logic             mem_rd_write;
    logic [`XLEN-1:0] mem_rd_wdata;

    logic             lsu_req_read;
    logic             lsu_req_write;
    logic             lsu_req_lr;
    logic             lsu_req_sc;
    logic             lsu_req_amo;
    logic             lsu_req_ready;
    logic [`XLEN-1:0] lsu_req_addr;
    logic [`XLEN-1:0] lsu_req_wdata;
    logic [2:0]       lsu_req_mem_opcode;
    logic [4:0]       lsu_req_amo_opcode;
    logic             lsu_req_is32bit;

    logic             lsu_rsp_valid;
    logic [`XLEN-1:0] lsu_rsp_data;
    logic             load_access_fault;
    logic             store_amo_access_fault;
    logic             sc_check_pass;
    logic             load_addr_misaligned;
    logic             store_amo_addr_misaligned;
    logic [`XLEN-1:0] amo_rdata;

    logic             wb_rd_write;
    logic [4:0]       wb_rd_addr;
    logic [`XLEN-1:0] wb_rd_wdata;

    logic             IF_flush;
    logic             ID_flush;

    /*AUTOWIRE*/

    // ---------------------------------
    // Glue logic for flush
    // ---------------------------------
    assign IF_flush = jump;
    assign ID_flush = jump;

    // ---------------------------------
    // Pipeline Stage
    // ---------------------------------

    IF u_IF (
        .clk                      (clk),
        .rst_n                    (rst_n),
        .IF_flush                 (IF_flush),
        .out_IF_valid             (IF_ID_valid),
        .out_IF_data              (IF_ID_data),
        .out_IF_ready             (IF_ID_ready),
        .jump                     (jump),
        .jump_pc                  (jump_pc),
        .trap                     (1'b0),
        .trap_pc                  (64'b0),
        .haddr                    (inst_haddr),
        .htrans                   (inst_htrans),
        .hwrite                   (inst_hwrite),
        .hsize                    (inst_hsize),
        .hwdata                   (inst_hwdata),
        .hlock                    (inst_hlock),
        .hready                   (inst_hready),
        .hrdata                   (inst_hrdata),
        .hresp                    (inst_hresp)
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
        .ID_flush     (ID_flush),
        .ex_rd_addr   (ex_rd_addr),
        .ex_rd_write  (ex_rd_write),
        .ex_rd_wdata  (ex_rd_wdata),
        .mem_rd_addr  (mem_rd_addr),
        .mem_rd_write (mem_rd_write),
        .mem_rd_wdata (mem_rd_wdata),
        .ex_mem_read  (ex_mem_read),
        .ex_lr        (ex_lr),
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
        .clk          (clk),
        .rst_n        (rst_n),
        // pipeline control
        .in_ID_valid  (ID_EX_valid),
        .in_ID_data   (ID_EX_data),
        .in_ID_ready  (ID_EX_ready),
        .out_EX_valid (out_EX_valid),
        .out_EX_data  (out_EX_data),
        .out_EX_ready (out_EX_ready),
        .EX_flush     (1'b0),
        .lsu_req_read (lsu_req_read),
        .lsu_req_write(lsu_req_write),
        .lsu_req_lr   (lsu_req_lr),
        .lsu_req_sc   (lsu_req_sc),
        .lsu_req_amo  (lsu_req_amo),
        .lsu_req_ready(lsu_req_ready),
        .lsu_req_addr (lsu_req_addr),
        .lsu_req_wdata(lsu_req_wdata),
        .lsu_req_mem_opcode(lsu_req_mem_opcode),
        .lsu_req_amo_opcode(lsu_req_amo_opcode),
        .lsu_req_is32bit(lsu_req_is32bit),
        .sc_check_pass(sc_check_pass),
        .load_addr_misaligned(load_addr_misaligned),
        .store_amo_addr_misaligned(store_amo_addr_misaligned),
        .amo_rdata    (amo_rdata),
        .jump_pc      (jump_pc),
        .jump         (jump),
        .ex_rd_addr   (ex_rd_addr),
        .ex_rd_write  (ex_rd_write),
        .ex_rd_wdata  (ex_rd_wdata),
        .ex_mem_read  (ex_mem_read),
        .ex_lr        (ex_lr)
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

    assign data_hsel = 1'b1;

    // LSU spans the EX request and MEM response stages.
    lsu u_lsu (
        .clk                       (clk),
        .rst_n                     (rst_n),
        .lsu_req_read              (lsu_req_read),
        .lsu_req_write             (lsu_req_write),
        .lsu_req_lr                (lsu_req_lr),
        .lsu_req_sc                (lsu_req_sc),
        .lsu_req_amo               (lsu_req_amo),
        .lsu_req_addr              (lsu_req_addr),
        .lsu_req_wdata             (lsu_req_wdata),
        .lsu_req_mem_opcode        (lsu_req_mem_opcode),
        .lsu_req_amo_opcode        (lsu_req_amo_opcode),
        .lsu_req_is32bit           (lsu_req_is32bit),
        .lsu_req_ready             (lsu_req_ready),
        .lsu_rsp_valid             (lsu_rsp_valid),
        .lsu_rsp_data              (lsu_rsp_data),
        .load_access_fault         (load_access_fault),
        .store_amo_access_fault    (store_amo_access_fault),
        .sc_check_pass             (sc_check_pass),
        .load_addr_misaligned      (load_addr_misaligned),
        .store_amo_addr_misaligned (store_amo_addr_misaligned),
        .amo_rdata                 (amo_rdata),
        .haddr                     (data_haddr),
        .htrans                    (data_htrans),
        .hwrite                    (data_hwrite),
        .hsize                     (data_hsize),
        .hwdata                    (data_hwdata),
        .hlock                     (data_hlock),
        .hready                    (data_hready),
        .hrdata                    (data_hrdata),
        .hresp                     (data_hresp)
    );

    MEM u_MEM(
        .in_EX_valid   (EX_MEM_valid),
        .in_EX_data    (EX_MEM_data),
        .in_EX_ready   (EX_MEM_ready),
        .out_MEM_valid (out_MEM_valid),
        .out_MEM_data  (out_MEM_data),
        .out_MEM_ready (out_MEM_ready),
        .MEM_flush     (1'b0),
        .lsu_rsp_valid (lsu_rsp_valid),
        .lsu_rsp_data  (lsu_rsp_data),
        .load_access_fault(load_access_fault),
        .store_amo_access_fault(store_amo_access_fault),
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
