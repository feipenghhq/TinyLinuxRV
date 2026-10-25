/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module WB (
    // pipeline control
    input                in_MEM_valid,
    input MEM2WB_PIPE_t  in_MEM_data,
    output               in_MEM_ready,
    // Register write back
    output               wb_rd_write,
    output [4:0]         wb_rd_addr,
    output [`XLEN-1:0]   wb_rd_wdata
);

    assign wb_rd_write = in_MEM_data.rd_write;
    assign wb_rd_addr  = in_MEM_data.rd_addr;
    assign wb_rd_wdata = in_MEM_data.rd_wdata;

    assign in_MEM_ready = 1'b1;

endmodule
