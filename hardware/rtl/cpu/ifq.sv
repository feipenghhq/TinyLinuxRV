/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

module ifq #(
    parameter DEPTH = 1
) (
    input              clk,
    input              rst_n,
    input              flush,
    input              req_valid,
    output             req_ready,
    input  [`XLEN-1:0] req_pc,
    output             rsp_valid,
    input              rsp_ready,
    output [`XLEN-1:0] rsp_pc,
    output [31:0]      rsp_instruction,
    output             rsp_error,
    // AHB
    output [`XLEN-1:0] haddr,
    output [1:0]       htrans,
    output             hwrite,
    output [2:0]       hsize,
    output [`XLEN-1:0] hwdata,
    output             hlock,
    input              hready,
    input [`XLEN-1:0]  hrdata,
    input              hresp
);

    typedef enum logic {INIT=0, WAIT=1} state_t;

    state_t ifq_state;

    logic pc_push_valid;
    logic pc_push_ready;
    logic pc_pop_ready;

    logic inst_push_valid;
    logic inst_pop_valid;
    logic inst_pop_ready;

    logic pending_resp;
    logic pending_flush;
    logic [`XLEN-1:0] pending_pc;

    // ------------------------------------------
    // IFQ Control logic
    // ------------------------------------------

    // IFQ State
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            ifq_state <= INIT;
            pending_resp <= 1'b0;
            pending_flush <= 1'b0;
            pending_pc <= 'b0;
        end
        else begin
            case (ifq_state)
                INIT: if (req_valid && req_ready && !hready) ifq_state <= WAIT;
                WAIT: if (hready) ifq_state <= INIT;
            endcase

            if (pending_flush && hready) pending_flush <= 1'b0; // clear pending flush when the next response comes
            else if (flush && ifq_state == WAIT && !hready) pending_flush <= 1'b1; // store the flush request when in wait state and is still waiting

            if (flush) pending_resp <= 1'b0;    // flush clear pending response
            else if (req_valid && req_ready && hready) pending_resp <= 1'b1; // new request is accepted
            else if (ifq_state == WAIT && hready && !pending_flush) pending_resp <= 1'b1; // In wait state and request accepted
            else if (pending_resp & hready) pending_resp <= 1'b0;

            if (req_valid && req_ready && !hready) pending_pc <= req_pc;
        end
    end

    // Upstream side: IFQ can accept new request when PC FIFO still has space and in INIT ifq_state
    assign req_ready = pc_push_ready & (ifq_state == INIT);

    // Downstream side: response is valid when instruction FIFO is valid
    assign rsp_valid = inst_pop_valid;

    // FIFO push
    assign pc_push_valid   = req_valid & req_ready;
    assign inst_push_valid = pending_resp & hready;

    // FIFO pop
    assign pc_pop_ready   = rsp_valid & rsp_ready;
    assign inst_pop_ready = rsp_valid & rsp_ready;

    // ------------------------------------------
    // AHB Control logic
    // ------------------------------------------

    assign haddr  = ifq_state == WAIT ? pending_pc : req_pc;
    assign htrans = ifq_state == WAIT ? 2'b10 : {req_valid && req_ready, 1'b0};
    assign hwrite = 1'b0;
    assign hsize  = 3'd2;
    assign hwdata = 0;
    assign hlock  = 0;

    // ------------------------------------------
    // FWFT FIFO for PC and AHB response
    // ------------------------------------------

    fwft_fifo #(
        .WIDTH(`XLEN),
        .DEPTH(DEPTH)
    ) pc_fifo (
        .clk         (clk),
        .rst_n       (rst_n),
        .flush       (flush),
        .push_valid  (pc_push_valid),
        .push_ready  (pc_push_ready),
        .push_data   (req_pc),
        /* verilator lint_off PINCONNECTEMPTY */
        .pop_valid   (), // don't need to check this
        /* verilator lint_on PINCONNECTEMPTY */
        .pop_ready   (pc_pop_ready),
        .pop_data    (rsp_pc)
    );

    fwft_fifo #(
        .WIDTH(33),
        .DEPTH(DEPTH)
    ) inst_fifo (
        .clk         (clk),
        .rst_n       (rst_n),
        .flush       (flush),
        .push_valid  (inst_push_valid),
        /* verilator lint_off PINCONNECTEMPTY */
        .push_ready  (), // don't need to check this
        /* verilator lint_on PINCONNECTEMPTY */
        .push_data   ({hresp, hrdata[31:0]}),
        .pop_valid   (inst_pop_valid),
        .pop_ready   (inst_pop_ready),  // Pop the FIFO when an instruction id taken by downstream
        .pop_data    ({rsp_error, rsp_instruction})
    );


endmodule
