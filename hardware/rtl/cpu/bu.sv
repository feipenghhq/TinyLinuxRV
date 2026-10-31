/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module bu (
    input              EX_flush,
    input              jal,
    input              jalr,
    input              branch,
    input [2:0]        branch_opcode,
    input [`XLEN-1:0]  rs1_value,
    input [`XLEN-1:0]  rs2_value,
    input [`XLEN-1:0]  immediate,
    input [`XLEN-1:0]  pc,
    output [`XLEN-1:0] jump_pc,
    output             jump,
    output             instr_addr_misaligned
);

    logic eq_result;
    logic lt_result;
    logic ltu_result;
    logic branch_result_raw;
    logic branch_result;

    logic             jump_raw;
    logic [`XLEN-1:0] jump_addr_op1;
    logic [`XLEN-1:0] jump_addr_op2;
    /* verilator lint_off UNUSEDSIGNAL */
    logic [`XLEN-1:0] jump_addr_result;
    /* verilator lint_on UNUSEDSIGNAL */

    // ---------------------------------
    // Check branch result
    // ---------------------------------
    assign eq_result  = rs1_value == rs2_value;
    assign lt_result  = $signed(rs1_value)   < $signed(rs2_value);
    assign ltu_result = $unsigned(rs1_value) < $unsigned(rs2_value);

    // Branch opcode encoding is the same as func3 field.
    // FUNC3    INST
    // 000      BEQ *
    // 001      BNE
    // 100      BLT *
    // 101      BGE
    // 110      BLTU *
    // 111      BGEU
    // - opcode[2:1] is used to distinguish BEQ/BLT/BLTU
    // - opcode[0]   is used to distinguish BEQ/BNE, BLT/BGE, BLTU/BGEU
    always @(*) begin
        case (branch_opcode[2:1])
            2'b10:   branch_result_raw = lt_result;
            2'b11:   branch_result_raw = ltu_result;
            default: branch_result_raw = eq_result;
        endcase
    end
    // if opcode[0] is 1 we need to invert the result.
    assign branch_result = branch_result_raw ^ branch_opcode[0];
    assign jump_raw = ~EX_flush & branch_result & branch | jal | jalr;

    // ---------------------------------
    // calculate the target pc address
    // ---------------------------------
    // for branch instruction: PC = current pc + immediate
    // for jal instruction:    PC = current pc + immediate
    // for jalr instruction:   PC = rs1_value + immediate
    assign jump_addr_op1 = jalr ? rs1_value : pc;
    assign jump_addr_op2 = immediate;
    assign jump_addr_result = jump_addr_op1 + jump_addr_op2;
    // Based on the RISCV Spec, bit zero of the target pc is always zero.
    assign jump_pc = {jump_addr_result[`XLEN-1:1], 1'b0};

    // address should be 4 byte aligned, jump_pc[0] is already 0, so jump_pc[1] must be 0
    assign instr_addr_misaligned = jump_raw & jump_pc[1];

    // Final jump decision
    assign jump = jump_raw & ~instr_addr_misaligned;

endmodule
