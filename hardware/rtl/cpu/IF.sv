/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

module IF (
    input clk,
    input rst_n,
    // To ID directly
    output              out_IF_valid,
    output IF2ID_PIPE_t out_IF_data,
    input               out_IF_ready,
    input               IF_flush,
    // target pc
    input               jump,
    input  [`XLEN-1:0]  jump_pc,
    input               trap,
    input  [`XLEN-1:0]  trap_pc,
    // Instruction Memory
    output [`XLEN-1:0]  haddr,
    output [1:0]        htrans,
    output              hwrite,
    output [2:0]        hsize,
    output [`XLEN-1:0]  hwdata,
    output              hlock,
    input               hready,
    input [`XLEN-1:0]   hrdata,
    input               hresp
);

    logic [`XLEN-1:0] pc;
    logic run;
    logic ifq_rea_valid;
    logic ifq_req_ready;

    // ---------------------------------
    // PC update
    // ---------------------------------

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            pc <= `PC_RESET_VEC;
        end
        else begin
            // flush should unblock PC update from stall request
            if (run && (ifq_req_ready || IF_flush)) begin
                if (trap) pc <= trap_pc;
                else if (jump) pc <= jump_pc;
                else pc <= pc + 4;
            end
        end
    end

    // ---------------------------------
    // IFQ control logic
    // ---------------------------------

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            run <= 1'b0;
        end
        else begin
            run <= 1'b1;
        end
    end

    assign ifq_rea_valid = run & ~IF_flush;

    // ---------------------------------
    // IFQ
    // --------------------------------

    ifq #(.DEPTH(2)) u_ifq (
    .clk        (clk),
    .rst_n      (rst_n),
    .flush      (IF_flush),
    .req_valid  (ifq_rea_valid),
    .req_ready  (ifq_req_ready),
    .req_pc     (pc),
    .rsp_valid  (out_IF_valid),
    .rsp_ready  (out_IF_ready),
    .rsp_pc     (out_IF_data.pc),
    .rsp_instruction(out_IF_data.instruction),
    .rsp_error  (out_IF_data.inst_access_fault),
    .haddr      (haddr),
    .htrans     (htrans),
    .hwrite     (hwrite),
    .hsize      (hsize),
    .hwdata     (hwdata),
    .hlock      (hlock),
    .hready     (hready),
    .hrdata     (hrdata),
    .hresp      (hresp));

endmodule
