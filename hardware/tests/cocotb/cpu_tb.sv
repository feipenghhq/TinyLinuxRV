/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */


`include "cpu.svh"

module cpu_tb ();

    logic clk;
    logic rst_n;

    logic strap_block_fetch_enable;

    AHBLiteReq_t inst_memory_req;
    AHBLiteRsp_t inst_memory_rsp;

    AHBLiteReq_t data_memory_req;
    AHBLiteRsp_t data_memory_rsp;

    logic [`XLEN-1:0] inst_haddr;
    logic [1:0]       inst_htrans;
    logic             inst_hwrite;
    logic [2:0]       inst_hsize;
    logic [`XLEN-1:0] inst_hwdata;
    logic             inst_hsel;
    logic [`XLEN-1:0] inst_hrdata;
    logic             inst_hready;
    logic             inst_hresp;

    logic [`XLEN-1:0] data_haddr;
    logic [1:0]       data_htrans;
    logic             data_hwrite;
    logic [2:0]       data_hsize;
    logic [`XLEN-1:0] data_hwdata;
    logic             data_hsel;
    logic [`XLEN-1:0] data_hrdata;
    logic             data_hready;
    logic             data_hresp;

    logic             test_finish;
    logic [`XLEN-1:0] test_result;
    logic [`XLEN-1:0] fail_test;

    // ----------------------------------------------------
    // expends the AHBLite bus so cocotb can access them
    // ----------------------------------------------------

    assign inst_haddr   = inst_memory_req.haddr ;
    assign inst_htrans  = inst_memory_req.htrans;
    assign inst_hwrite  = inst_memory_req.hwrite;
    assign inst_hsize   = inst_memory_req.hsize ;
    assign inst_hwdata  = inst_memory_req.hwdata;
    assign inst_hsel    = inst_memory_req.hsel  ;
    assign inst_memory_rsp.hrdata = inst_hrdata;
    assign inst_memory_rsp.hready = inst_hready;
    assign inst_memory_rsp.hresp  = inst_hresp ;

    assign data_haddr   = data_memory_req.haddr ;
    assign data_htrans  = data_memory_req.htrans;
    assign data_hwrite  = data_memory_req.hwrite;
    assign data_hsize   = data_memory_req.hsize ;
    assign data_hwdata  = data_memory_req.hwdata;
    assign data_hsel    = data_memory_req.hsel  ;
    assign data_memory_rsp.hrdata = data_hrdata;
    assign data_memory_rsp.hready = data_hready;
    assign data_memory_rsp.hresp  = data_hresp ;

    // ----------------------------------------------------
    // Check test end
    // ----------------------------------------------------

    // Test write to syscon with poweroff command to indicate test end
    // That is writing to 0x00100000 with 0x1

    assign test_finish = (data_htrans == 2) && data_hwrite && data_haddr == 64'h00100000;
    // TODO: Only checked address. Need to check the data too. But AHB is 2 phase.

    // Test Result
    assign test_result = u_cpu.u_ID.u_regfile.register_file[10];

    // Failed test case
    assign fail_test = u_cpu.u_ID.u_regfile.register_file[11];

    // ----------------------------------------------------
    // Instantiate DUT
    // ----------------------------------------------------

    cpu u_cpu(.*);

endmodule
