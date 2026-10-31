/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

module amo_alu (
    input  [`XLEN-1:0] a, // loaded value
    input  [`XLEN-1:0] b, // rs2
    input  [4:0]       opcode,
    input              is_32bit,
    output logic [`XLEN-1:0] o
);

    logic             is_signed;
    logic [`XLEN-1:0] a_ext;
    logic [`XLEN-1:0] b_ext;
    logic [`XLEN-1:0] a_post;
    logic [`XLEN-1:0] b_post;
    logic             less_then;
    logic [`XLEN-1:0] result;

    assign is_signed = opcode[4:3] == 2'b10;

    assign a_ext = is_32bit ? {{32{a[31]}}, a[31:0]} : a;
    assign b_ext = is_32bit ? {{32{b[31]}}, b[31:0]} : b;

    assign a_post = {a_ext[`XLEN-1] ^ is_signed, a_ext[`XLEN-2:0]};
    assign b_post = {b_ext[`XLEN-1] ^ is_signed, b_ext[`XLEN-2:0]};

    assign less_then = a_post < b_post;

    always_comb begin
        result = 0;
        case(opcode)
        5'b00001: result = b;                 // AMO SWAP
        5'b00000: result = a + b;             // AMO ADD
        5'b00100: result = a ^ b;             // AMO XOR
        5'b01100: result = a & b;             // AMO AND
        5'b01000: result = a | b;             // AMO OR
        5'b10000: result = less_then ? a : b; // AMO MIN
        5'b10100: result = less_then ? b : a; // AMO MAX
        5'b11000: result = less_then ? a : b; // AMO MINU
        5'b11100: result = less_then ? b : a; // AMO MAXU
        default: result = 0;
        endcase
    end

    assign o = is_32bit ? {{32{result[31]}}, result[31:0]} : result;

endmodule
