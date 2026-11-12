/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module MEM (
    // pipeline control
    input                in_EX_valid,
    input EX2MEM_PIPE_t  in_EX_data,
    output               in_EX_ready,
    output               out_MEM_valid,
    output MEM2WB_PIPE_t out_MEM_data,
    input                out_MEM_ready,
    input                MEM_flush,
    // LSU response
    input                lsu_rsp_valid,
    input [`XLEN-1:0]    lsu_rsp_data,
    input                load_access_fault,
    input                store_amo_access_fault,
    // Data forwarding logic
    output [4:0]         mem_rd_addr,
    output               mem_rd_write,
    output [`XLEN-1:0]   mem_rd_wdata
);

    logic             MEM_stall;
    logic [`XLEN-1:0] rd_wdata;

    // ---------------------------------
    // Data forwarding logic
    // ---------------------------------

    // final rd_wdata at MEM stage
    assign rd_wdata = in_EX_data.mem_read ? lsu_rsp_data : in_EX_data.rd_wdata;

    assign mem_rd_addr  = in_EX_data.rd_addr;
    assign mem_rd_write = in_EX_data.rd_write;
    assign mem_rd_wdata = rd_wdata;

    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    assign MEM_stall = in_EX_data.mem_access & ~lsu_rsp_valid;

    assign in_EX_ready = out_MEM_ready & ~MEM_stall;

    assign out_MEM_valid = in_EX_valid & ~MEM_flush & ~MEM_stall;

    assign out_MEM_data.pc = in_EX_data.pc;
    assign out_MEM_data.instruction = in_EX_data.instruction;
    assign out_MEM_data.rd_write = in_EX_data.rd_write;
    assign out_MEM_data.rd_addr = in_EX_data.rd_addr;
    assign out_MEM_data.rd_wdata = rd_wdata;

    // exception
    assign out_MEM_data.exception_valid = in_EX_data.exception_valid |
                                            load_access_fault | store_amo_access_fault;
    assign out_MEM_data.exception_cause = in_EX_data.exception_valid ? in_EX_data.exception_cause :
                                            load_access_fault ? LOAD_ACCESS_FAULT :
                                                                STORE_AMO_ACCESS_FAULT;


endmodule
