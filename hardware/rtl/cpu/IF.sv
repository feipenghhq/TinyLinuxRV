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
    logic ifq_req_valid;
    logic ifq_req_ready;

    logic [`XLEN-1:0] pc_in_data;
    logic pc_in_valid;
    logic pc_out_ready;

    // ---------------------------------
    // PC update
    // ---------------------------------

    skid_buffer #(.WIDTH(`XLEN))
    u_pc_skid_buffer(
        .clk       (clk),
        .rst_n     (rst_n),
        .in_valid  (pc_in_valid),
        .in_data   (pc_in_data),
        .in_ready  (),
        .out_valid (),
        .out_data  (pc),
        .out_ready (pc_out_ready)
    );

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            pc_in_data <= `PC_RESET_VEC;
        end
        else begin
            // flush should unblock PC update from stall request
            if (run && (ifq_req_ready || IF_flush)) begin
                if (trap)      pc_in_data <= trap_pc;
                else if (jump) pc_in_data <= jump_pc;
                else           pc_in_data <= pc_in_data + 4;
            end
        end
    end

    assign pc_in_valid = run;

    assign pc_out_ready = ifq_req_ready;

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

    assign ifq_req_valid = run & ~IF_flush;

    // ---------------------------------
    // IFQ
    // --------------------------------

    ifq #(.DEPTH(2)) u_ifq (
    .clk        (clk),
    .rst_n      (rst_n),
    .flush      (IF_flush),
    .req_valid  (ifq_req_valid),
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
