/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

/**

- Since our CPU is mainly targeted FPGA, so for multiplier we use * operator directly.
  We have input register and output register so we expect the logic to be mapped into the FPGA DSP logic.

- Mul will raise stall to stall the pipeline,
  If a req is pending and stall is de-asserted, then the data is valid and send to the next stage.

opcode used in opcode signal:
    MUL       2'b00
    MULH      2'b01
    MULHSU    2'b10
    MULHU     2'b11

*/

`include "cpu.svh"


module multiplier (
    input              clk,
    input              rst_n,
    input              req,
    input  [1:0]       opcode,
    input              is_32bit,
    input  [`XLEN-1:0] a,      // rs0
    input  [`XLEN-1:0] b,      // rs2
    output [`XLEN-1:0] o,      // rd
    output             stall
);

    // ---------------------------------
    // Signal Declaration
    // ---------------------------------
    // status
    logic [1:0] stage_valid;
    logic       running;
    logic       new_req;
    // processing sign
    logic [`XLEN:0] a_sext;
    logic [`XLEN:0] b_sext;
    logic [`XLEN:0] a_uext;
    logic [`XLEN:0] b_uext;
    // stage 0
    logic signed [`XLEN:0] a_s0;
    logic signed [`XLEN:0] b_s0;
    // stage 1
    logic signed [(`XLEN+1)*2-1:0] o_s1;
    logic [`XLEN-1:0] output_raw;

    // ---------------------------------
    // Main logic
    // ---------------------------------

    // stage 0: process the input data and store it in the register

    assign a_sext = {a[`XLEN-1], a};
    assign b_sext = {b[`XLEN-1], b};
    assign a_uext = {1'b0, a};
    assign b_uext = {1'b0, b};

    // a unsigned ext: MULHU => opcode[1:0] == 2'b11
    always @(posedge clk) a_s0 <= (opcode[1:0] == 2'b11) ? a_uext : a_sext;

    // b unsigned ext: MULHSU or MULHU => opcode[1] == 1
    always @(posedge clk) b_s0 <= opcode[1] ? b_uext : b_sext;

    // stage 1: calculate the result
    always @(posedge clk) o_s1 <= a_s0 * b_s0;

    // stage 2: process the output data (pipeline register is EX/MEM stage)
    assign output_raw = (opcode[1:0] == 2'b00) ? o_s1[`XLEN-1:0] : o_s1[2*`XLEN-1:`XLEN];
    assign o = is_32bit ? {{32{output_raw[31]}}, output_raw[31:0]} : output_raw;

    // flow control
    assign new_req = ~running & req;
    always @(posedge clk) begin
        if (!rst_n) stage_valid <= 'b0;
        else stage_valid <= {stage_valid[0], new_req};
    end

    always @(posedge clk) begin
        if (!rst_n) running <= 1'b0;
        else if (new_req) running <= 1'b1;
        else if (stage_valid[1]) running <= 1'b0;
    end

    assign stall = new_req | running & ~stage_valid[1];

endmodule
