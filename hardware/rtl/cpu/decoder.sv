/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module decoder (
    input [31:0] instruction,

    // register file
    output logic       rd_write,
    output logic [4:0] rd_addr,
    output logic       rs1_read,
    output logic [4:0] rs1_addr,
    output logic       rs2_read,
    output logic [4:0] rs2_addr,
    output ALU_CTRL_t  alu_ctrl,
    output JUMP_CTRL_t jump_ctrl,
    output MEM_CTRL_t  mem_ctrl,
    output logic [`XLEN-1:0] immediate,
    output logic illegal_instruction  // Illegal instruction
);


    // ---------------------------------
    // Signal Declaration
    // ---------------------------------

    logic [6:0] opcode;
    logic [6:0] func7;
    logic [2:0] func3;
    logic [5:0] inst31_26;

    logic func7_eq_0, func7_eq_0x20;
    logic func3_eq_0x1, func3_eq_0x5;
    logic inst31_26_eq_0, inst31_26_eq_0x10;


    // ---------------------------------
    //  main logic
    // ---------------------------------

    // Extract instruction field

    assign rd_addr  = instruction[`DEC_RD_FIELD];
    assign rs1_addr = instruction[`DEC_RS1_FIELD];
    assign rs2_addr = instruction[`DEC_RS2_FIELD];

    assign opcode = instruction[`DEC_OPCODE_FIELD];
    assign func7  = instruction[`DEC_FUNC7_FIELD];
    assign func3  = instruction[`DEC_FUNC3_FIELD];
    assign inst31_26 = instruction[31:26];

    assign func7_eq_0    = func7 == 0;
    assign func7_eq_0x20 = func7 == 'h20;
    assign func3_eq_0x1  = func3 == 1;
    assign func3_eq_0x5  = func3 == 5;

    assign inst31_26_eq_0    = inst31_26 == 0;
    assign inst31_26_eq_0x10 = inst31_26 == 'h10;

    // Decode logic
    always_comb begin

        rd_write = 1'b0;
        rs1_read = 1'b0;
        rs2_read = 1'b0;

        alu_ctrl.alu_op1_sel_zero = 1'b0;
        alu_ctrl.alu_op1_sel_pc   = 1'b0;
        alu_ctrl.alu_op2_sel_imm  = 1'b0;
        alu_ctrl.alu_opcode       = ALU_ADD;
        alu_ctrl.alu_32bit        = 1'b0;

        jump_ctrl.jal  = 1'b0;
        jump_ctrl.jalr = 1'b0;
        jump_ctrl.branch = 1'b0;
        jump_ctrl.branch_opcode = func3;

        mem_ctrl.mem_read   = 1'b0;
        mem_ctrl.mem_write  = 1'b0;
        mem_ctrl.mem_opcode = func3;

        illegal_instruction = 1'b0;

        // LEVEL 1 - opcode
        case (opcode)
            // Logic R Type instruction (64bit)
            OPCODE_LOGIC_R64, OPCODE_LOGIC_R32: begin
                rs1_read    = 1'b1;
                rs2_read    = 1'b1;
                rd_write    = 1'b1;
                alu_ctrl.alu_opcode[3:0] = ALU_OP_t'({func7[5], func3});
                alu_ctrl.alu_32bit       = opcode[3];  // opcode[3] is set for XXXW instruction
                // Instruction[31:25] (func7) is fixed encoding for R type instruction so we need to check them
                illegal_instruction = ~(func7_eq_0 | func7_eq_0x20);
            end

            // Logic I Type instruction
            OPCODE_LOGIC_I64, OPCODE_LOGIC_I32: begin
                rs1_read = 1'b1;
                rd_write = 1'b1;
                alu_ctrl.alu_op2_sel_imm = 1'b1;
                alu_ctrl.alu_opcode[3:0] = ALU_OP_t'({(func3 == 3'b101) & func7[5], func3});
                alu_ctrl.alu_32bit = opcode[3];  // opcode[3] is set for XXXIW instruction
                // For 32 bit operations SLLIW, SRLIW, SRAIW, instruction[31:25] (func7) is fixed encoding
                // For 64 bit operations SLLI, SRLI, SRAI, instruction[31:2365] is fixed encoding
                // So we need to check them
                illegal_instruction = alu_ctrl.alu_32bit ? (func3_eq_0x1 & ~func7_eq_0 | func3_eq_0x5 & ~(func7_eq_0 | func7_eq_0x20))      // SLLIW, SRLIW, SRAIW
                                    : (func3_eq_0x1 & ~inst31_26_eq_0 | func3_eq_0x5 & ~(inst31_26_eq_0 | inst31_26_eq_0x10));  // SLLI, SRLI, SRAI
            end

            // LUI
            OPCODE_LUI: begin
                alu_ctrl.alu_op1_sel_zero = 1'b1;
                alu_ctrl.alu_op2_sel_imm = 1'b1;
                alu_ctrl.alu_opcode = ALU_ADD;
                rd_write = 1'b1;
            end

            // AUIPC
            OPCODE_AUIPC: begin
                alu_ctrl.alu_op1_sel_pc = 1'b1;
                alu_ctrl.alu_op2_sel_imm = 1'b1;
                alu_ctrl.alu_opcode = ALU_ADD;
                rd_write = 1'b1;
            end

            // JAL
            OPCODE_JAL: begin
                jump_ctrl.jal = 1'b1;
                rd_write = 1'b1;
            end

            // JALR
            OPCODE_JALR: begin
                jump_ctrl.jalr = 1'b1;
                rs1_read = 1'b1;
                rd_write = 1'b1;
                illegal_instruction = func3 != 0;
            end

            // Branch instruction
            OPCODE_BRANCH: begin
                jump_ctrl.branch = 1'b1;
                rs1_read = 1'b1;
                rs2_read = 1'b1;
                illegal_instruction = (func3 == 3'b011 | func3 == 3'b010);
            end

            // Load instruction
            OPCODE_LOAD: begin
                rs1_read = 1'b1;
                rd_write = 1'b1;
                mem_ctrl.mem_read = 1'b1;
                illegal_instruction = func3 == 3'b111;
            end

            // Store instruction
            OPCODE_STORE: begin
                rs1_read = 1'b1;
                rs2_read = 1'b1;
                mem_ctrl.mem_write = 1'b1;
                illegal_instruction = func3[2];
            end

            // TBD: system

            default: begin
                illegal_instruction = 1'b1;
            end

        endcase

    end


    // immediate values
    always @(*) begin
        case (opcode)
            // In the default section
            OPCODE_LOGIC_I64, OPCODE_LOGIC_I32, OPCODE_LOAD: begin
                immediate = {{52{instruction[31]}}, instruction[31:20]};
            end
            OPCODE_STORE: begin
                immediate = {{52{instruction[31]}}, instruction[31:25], instruction[11:7]};
            end
            OPCODE_BRANCH: begin
                immediate = {
                    {52{instruction[31]}}, instruction[7], instruction[30:25], instruction[11:7]
                };
            end
            OPCODE_JAL: begin
                immediate = {
                    {44{instruction[31]}},
                    instruction[19:12],
                    instruction[20],
                    instruction[30:21],
                    1'b0
                };
            end
            OPCODE_LUI, OPCODE_AUIPC: begin
                immediate = {{32{instruction[31]}}, instruction[31:12], 12'b0};
            end
            default: immediate = {{52{instruction[31]}}, instruction[31:20]};
        endcase
    end

endmodule
