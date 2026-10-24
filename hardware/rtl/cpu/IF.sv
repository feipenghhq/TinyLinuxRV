/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

module IF (
    input                           clk,
    input                           rst_n,
    // pipeline control
    output                          out_IF_valid,
    output IF2ID_PIPE_t             out_IF_data,
    input                           out_IF_ready,
    input                           IF_flush,
    // strap
    input                           strap_block_fetch_enable,
    // target pc
    input                           jump,
    input               [`XLEN-1:0] jump_pc,
    input                           trap,
    input               [`XLEN-1:0] trap_pc,
    // Instruction Memory
    input                           block_fetch,               // block instruction fetch
    output AHBLiteReq_t             memory_req,
    input                           hready
);


    logic [`XLEN-1:0] pc;

    logic run;
    logic instruction_read;

    // ---------------------------------
    // Instruction memory control
    // ---------------------------------

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) run <= 1'b0;
        else run <= 1'b1;
    end

    // Only assert the read when IF is not stalled and AHB bus is ready to accept the request
    assign instruction_read  = run && out_IF_ready & ~IF_flush & hready & ~(block_fetch & strap_block_fetch_enable);

    // AHBLite request
    assign memory_req.haddr = pc;
    assign memory_req.htrans = {instruction_read, 1'b0};
    assign memory_req.hwrite = 1'b0;
    assign memory_req.hsize = 3'b010;  // instruction is 32 bits
    assign memory_req.hwdata = 0;
    assign memory_req.hsel = 1'b1;

    // ---------------------------------
    // PC update
    // --------------------------------
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            pc <= `PC_RESET_VEC;
        end
        else begin
            // flush should unblock PC update from stall request
            if (out_IF_ready || IF_flush) begin
                if (trap) pc <= trap_pc;
                else if (jump) pc <= jump_pc;
                else pc <= pc + 4;
            end
        end
    end

    // ---------------------------------
    // Pipeline control
    // --------------------------------

    assign out_IF_valid = memory_req.htrans[1] & hready;
    assign out_IF_data.pc = pc;

endmodule
