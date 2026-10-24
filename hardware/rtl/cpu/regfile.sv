/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

module regfile (
    input              clk,
    // rd write back
    input              reg_write,
    input  [4:0]       reg_addr,
    input  [`XLEN-1:0] reg_wdata,
    // rs1
    input  [4:0]       rs1_addr,
    output [`XLEN-1:0] rs1_value,
    // rs2
    input  [4:0]       rs2_addr,
    output [`XLEN-1:0] rs2_value
);

    logic [`XLEN-1:0] register_file[`REG_NUM-1:0];

    logic rs1_addr_eq_zero;
    logic rs2_addr_eq_zero;

    logic rs1_forward;
    logic rs2_forward;

    // Write back port
    always @(posedge clk) begin
        if (reg_write) register_file[reg_addr] <= reg_wdata;
    end

    // Register_file x0 is always read zero. Here we use a mux to select 0 when read reg_addr is 0
    // We forward the data from WB stage if the rd addr match with rs1/rs2 addr.

    // Read port A
    assign rs1_addr_eq_zero = (rs1_addr == 0);
    assign rs1_forward = (rs1_addr == reg_addr) & reg_write;
    assign rs1_value = rs1_addr_eq_zero ? 0 : (rs1_forward ? reg_wdata : register_file[rs1_addr]);

    // Read port B
    assign rs2_addr_eq_zero = (rs2_addr == 0);
    assign rs2_forward = (rs2_addr == reg_addr) & reg_write;
    assign rs2_value = rs2_addr_eq_zero ? 0 : (rs2_forward ? reg_wdata : register_file[rs2_addr]);

endmodule
