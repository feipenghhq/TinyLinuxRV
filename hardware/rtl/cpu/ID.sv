/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module ID (
    input clk,
    input rst_n,

    // pipeline control
    input               in_IF_valid,
    input IF2ID_PIPE_t  in_IF_data,
    output              in_IF_ready,
    output              out_ID_valid,
    output ID2EX_PIPE_t out_ID_data,
    input               out_ID_ready,
    input               ID_flush,
    // Data forwarding logic
    input [4:0]         ex_rd_addr,
    input               ex_rd_write,
    input [`XLEN-1:0]   ex_rd_wdata,
    input [4:0]         mem_rd_addr,
    input               mem_rd_write,
    input [`XLEN-1:0]   mem_rd_wdata,
    // load dependency check
    input               ex_mem_read,
    input               ex_lr,
    // Register write back
    input               wb_rd_write,
    input [4:0]         wb_rd_addr,
    input [`XLEN-1:0]   wb_rd_wdata
);

    CPU_CTRL_t        cpu_ctrl;

    logic             rd_write;
    logic [4:0]       rd_addr;
    logic             rs1_read;
    logic [4:0]       rs1_addr;
    logic [`XLEN-1:0] rs1_rdata;
    logic             rs2_read;
    logic [4:0]       rs2_addr;
    logic [`XLEN-1:0] rs2_rdata;

    logic             instruction_access_fault;
    logic             instruction_valid;
    logic [31:0]      instruction_buffer;
    logic [31:0]      instruction;
    logic             illegal_instruction;
    logic [`XLEN-1:0] immediate;

    logic             rs1_nonzero;
    logic             rs1_match_ex;
    logic             rs1_match_mem;
    logic             rs2_nonzero;
    logic             rs2_match_ex;
    logic             rs2_match_mem;
    logic [`XLEN-1:0] final_rs1;
    logic [`XLEN-1:0] final_rs2;

    logic             load_dependency;
    logic             ID_stage_valid;
    logic             ID_stage_stall;

    typedef enum logic {RUN=0, STALLED=1} state_t;
    state_t           ID_state;

    // ---------------------------------
    // Main components
    // ---------------------------------

    // decoder
    decoder u_decoder(
        .instruction (instruction),
        .rd_write    (rd_write),
        .rd_addr     (rd_addr),
        .rs1_read    (rs1_read),
        .rs1_addr    (rs1_addr),
        .rs2_read    (rs2_read),
        .rs2_addr    (rs2_addr),
        .cpu_ctrl    (cpu_ctrl),
        .immediate   (immediate),
        .illegal_instruction(illegal_instruction)
    );

    // register file
    regfile u_regfile(
        .clk        (clk),
        .reg_write  (wb_rd_write),
        .reg_addr   (wb_rd_addr),
        .reg_wdata  (wb_rd_wdata),
        .rs1_addr   (rs1_addr),
        .rs1_value  (rs1_rdata),
        .rs2_addr   (rs2_addr),
        .rs2_value  (rs2_rdata)
    );

    // ---------------------------------
    // Fetch Instruction from IF
    // ---------------------------------

    assign instruction_valid = (in_IF_valid | ID_state == STALLED) & ~ID_flush;
    assign instruction_access_fault = in_IF_valid & ~ID_flush & in_IF_data.inst_access_fault;

    always_ff @(posedge clk, negedge rst_n) begin
        if (~in_IF_ready && ID_state == RUN) instruction_buffer <= in_IF_data.instruction;
    end

    assign instruction = (ID_state == STALLED) ? instruction_buffer : in_IF_data.instruction;

    // ---------------------------------
    // Data forwarding MUX
    // ---------------------------------

    assign rs1_nonzero   = (rs1_addr != 0);
    assign rs2_nonzero   = (rs2_addr != 0);
    assign rs1_match_ex  = rs1_nonzero & rs1_read & (rs1_addr == ex_rd_addr)  & ex_rd_write;
    assign rs2_match_ex  = rs2_nonzero & rs2_read & (rs2_addr == ex_rd_addr)  & ex_rd_write;
    assign rs1_match_mem = rs1_nonzero & rs1_read & (rs1_addr == mem_rd_addr) & mem_rd_write;
    assign rs2_match_mem = rs2_nonzero & rs2_read & (rs2_addr == mem_rd_addr) & mem_rd_write;

    assign final_rs1 = rs1_match_ex  ? ex_rd_wdata  :
                       rs1_match_mem ? mem_rd_wdata :
                       rs1_rdata;
    assign final_rs2 = rs2_match_ex  ? ex_rd_wdata  :
                       rs2_match_mem ? mem_rd_wdata :
                       rs2_rdata;

    // ---------------------------------
    // dependency check
    // ---------------------------------

    // There are several dependency requires ID stage to be stalled

    // FIXME: SC will also write to the register and SC is not completed at EX stage!!!

    // Case 1: When EX stage is a Load instruction and ID stage instruction depends on the result of the load:
    // - We cannot forward a result from Load instruction from EX stage to ID stage as the read data is not ready yet in EX
    // - We need to Stall ID stage for 1 cycle to let the memory read instruction goto the MEM stage to get the data ready
    // Case 2: When LR is followed by an SC. LR completes at MEM stage so SC need to wait till LR reach
    assign load_dependency = ID_stage_valid &
                            ((rs1_match_ex | rs2_match_ex) & ex_mem_read |  // case 1
                             (cpu_ctrl.sc & ex_lr));                        // case 2


    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    // ID state machine
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            ID_state <= RUN;
        end
        else begin
            case (ID_state)
                RUN: if (ID_stage_stall && ID_stage_valid) ID_state <= STALLED;
                STALLED: if (ID_flush || ~ID_stage_stall) ID_state <= RUN;
            endcase
        end
    end

    // ID stage is valid when: 1) Get valid instruction, 2) Instruction is decoded successfully, 3) Not flushed
    assign ID_stage_valid = instruction_valid & ~illegal_instruction & ~ID_flush;

    // ID need to be stalled when: 1) EX is stalled, or 2) load dependency
    assign ID_stage_stall = ~out_ID_ready | load_dependency;

    // ID/IF is ready ID is not stalled
    assign in_IF_ready = ~ID_stage_stall;

    // ID/EX is valid when 1）ID stage is valid, 2) not stalled due to load_dependency
    assign out_ID_valid = ID_stage_valid & ~load_dependency;

    // ctrl and data
    assign out_ID_data.pc          = in_IF_data.pc;
    assign out_ID_data.instruction = instruction;
    assign out_ID_data.immediate   = immediate;
    assign out_ID_data.cpu_ctrl    = cpu_ctrl;
    assign out_ID_data.rs1_value   = final_rs1;
    assign out_ID_data.rs2_value   = final_rs2;
    assign out_ID_data.rd_write    = rd_write;
    assign out_ID_data.rd_addr     = rd_addr;

    // exception
    assign out_ID_data.exception_valid = instruction_access_fault | illegal_instruction;
    assign out_ID_data.exception_cause = instruction_access_fault ? INST_ACCESS_FAULT : ILLEGAL_INSTRUCTION;

endmodule
