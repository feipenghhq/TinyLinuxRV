/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module alu (
    input  [`XLEN-1:0]       alu_op1,
    input  [`XLEN-1:0]       alu_op2,
    input  [`ALU_OP_RANGE]   alu_opcode,
    input                    alu_32bit,
    output logic [`XLEN-1:0] alu_out
);

    logic [`XLEN-1:0] alu_out64;
    logic             slt_result;
    logic             sltu_result;
    logic [5:0]       shamt;

    assign slt_result  = ($signed(alu_op1)   < $signed(alu_op2));
    assign sltu_result = ($unsigned(alu_op1) < $unsigned(alu_op2));
    assign shamt = alu_32bit ? {1'b0, alu_op2[4:0]} : alu_op2[5:0];

    always @(*) begin
        case(alu_opcode)
            ALU_ADD:  alu_out64 = alu_op1 + alu_op2;
            ALU_SUB:  alu_out64 = alu_op1 - alu_op2;
            ALU_SLL:  alu_out64 = alu_op1 << shamt;
            ALU_SLT:  alu_out64 = {{(`XLEN-1){1'b0}},slt_result};
            ALU_SLTU: alu_out64 = {{(`XLEN-1){1'b0}},sltu_result};
            ALU_XOR:  alu_out64 = alu_op1 ^ alu_op2;
            ALU_SRL:  alu_out64 = alu_op1 >> shamt;
            ALU_SRA:  alu_out64 = $signed(alu_op1) >>> shamt;
            ALU_OR:   alu_out64 = alu_op1 | alu_op2;
            ALU_AND:  alu_out64 = alu_op1 & alu_op2;
            default:  alu_out64 = alu_op1 & alu_op2;
        endcase
    end

    assign alu_out = alu_32bit ? {{32{alu_out64[31]}}, alu_out64[31:0]} : alu_out64;

endmodule
