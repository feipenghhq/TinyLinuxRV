/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module EX (
    input                clk,
    input                rst_n,
    // pipeline control
    input                in_ID_valid,
    input ID2EX_PIPE_t   in_ID_data,
    output               in_ID_ready,
    output               out_EX_valid,
    output EX2MEM_PIPE_t out_EX_data,
    input                out_EX_ready,
    input                EX_flush,
    // AHB request
    output AHBLiteReq_t  memory_req,
    input  AHBLiteRsp_t  memory_rsp,
    input [`XLEN-1:0]    MEM_hwdata, // write data from MEM stage
    // Jump
    output [`XLEN-1:0]   jump_pc,
    output               jump,
    // Data forwarding logic
    output [4:0]         ex_rd_addr,
    output               ex_rd_write,
    output [`XLEN-1:0]   ex_rd_wdata,
    // Load dependency check
    output               ex_mem_read,
    // block instruction fetch when there are memory request in EX
    output               block_fetch
);

    logic [`XLEN-1:0] alu_op1;
    logic [`XLEN-1:0] alu_op2;
    logic [`XLEN-1:0] alu_out;
    logic [`XLEN-1:0] mul_out;
    logic             mul_stall;
    logic [`XLEN-1:0] div_out;
    logic             div_stall;

    logic [2:0]       hsize;
    logic [`XLEN-1:0] haddr;
    logic [`XLEN-1:0] hwdata;
    logic             mem_read;
    logic             mem_write;
    logic             mem_request_raw;
    logic             mem_request;

    logic             EX_stall;

    // AMO LR/SC indicator
    logic [`XLEN-1:0] haddr_end;
    logic             sc_check_pass;
    logic             AMO_res_valid;
    logic [`XLEN-1:0] AMO_res_addr_start;
    logic [`XLEN-1:0] AMO_res_addr_end;

    // exception
    logic instr_addr_misaligned;
    logic mem_addr_misaligned;
    logic load_addr_misaligned;
    logic store_amo_addr_misaligned;

    // AMO
    typedef enum logic [1:0] {AMO_READ_REQ, AMO_READ_DATA, AMO_WRITE_REQ, AMO_WRITE_DATA} AMO_state_t;
    AMO_state_t       amo_state;
    logic             amo_read;
    logic             amo_write;
    logic [`XLEN-1:0] amo_rdata;
    logic [`XLEN-1:0] amo_alu_out;
    logic [`XLEN-1:0] amo_wdata;
    logic             amo_stall;

    // ---------------------------------
    // Main components
    // ---------------------------------

    alu u_alu (
        .alu_op1    (alu_op1),
        .alu_op2    (alu_op2),
        .alu_opcode (in_ID_data.cpu_ctrl.opcode[3:0]),
        .alu_32bit  (in_ID_data.cpu_ctrl.is_32bit),
        .alu_out    (alu_out)
    );

    amo_alu u_amo_alu(
        .a        (amo_rdata), // loaded value
        .b        (in_ID_data.rs2_value), // rs2
        .opcode   (in_ID_data.cpu_ctrl.opcode),
        .is_32bit (in_ID_data.cpu_ctrl.is_32bit),
        .o        (amo_alu_out)
    );

    multiplier u_multiplier (
        .clk      (clk),
        .rst_n    (rst_n),
        .req      (in_ID_data.cpu_ctrl.mul),
        .opcode   (in_ID_data.cpu_ctrl.opcode[1:0]),
        .is_32bit (in_ID_data.cpu_ctrl.is_32bit),
        .a        (alu_op1),      // rs0
        .b        (alu_op2),      // rs2
        .o        (mul_out),      // rd
        .stall    (mul_stall)
    );

    divider u_divider (
        .clk      (clk),
        .rst_n    (rst_n),
        .flush    (1'b0),         // FIXME: Need to add flush
        .req      (in_ID_data.cpu_ctrl.div),
        .opcode   (in_ID_data.cpu_ctrl.opcode[1:0]),
        .is_32bit (in_ID_data.cpu_ctrl.is_32bit),
        .a        (alu_op1),      // rs0
        .b        (alu_op2),      // rs2
        .o        (div_out),      // rd
        .stall    (div_stall)
    );

    bu u_bu (
        .EX_flush   (EX_flush),
        .jal        (in_ID_data.cpu_ctrl.jal),
        .jalr       (in_ID_data.cpu_ctrl.jalr),
        .branch     (in_ID_data.cpu_ctrl.branch),
        .branch_opcode(in_ID_data.cpu_ctrl.opcode[2:0]),
        .rs1_value  (in_ID_data.rs1_value),
        .rs2_value  (in_ID_data.rs2_value),
        .immediate  (in_ID_data.immediate),
        .pc         (in_ID_data.pc),
        .jump_pc    (jump_pc),
        .jump       (jump),
        .instr_addr_misaligned(instr_addr_misaligned)
    );

    // ---------------------------------
    // LR/SC
    // ---------------------------------
    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            AMO_res_valid <= 1'b0;
            AMO_res_addr_start <= 'b0;
            AMO_res_addr_end <= 'b0;
        end
        else begin
            if (in_ID_data.cpu_ctrl.lr) begin
                AMO_res_valid      <= 1'b1;
                AMO_res_addr_start <= haddr;
                AMO_res_addr_end   <= haddr_end;
            end
            else if (in_ID_data.cpu_ctrl.sc) begin
                AMO_res_valid      <= 1'b0;
                AMO_res_addr_start <= 'b0;
                AMO_res_addr_end   <= 'b0;
            end
        end
    end

    assign haddr_end = haddr + (in_ID_data.cpu_ctrl.is_32bit ? 4 : 8);

    assign sc_check_pass = AMO_res_valid &
                           haddr >= AMO_res_addr_start &
                           haddr_end <= AMO_res_addr_end;

    // ---------------------------------
    // ALU operand select
    // ---------------------------------

    assign alu_op1 = in_ID_data.cpu_ctrl.alu_op1_sel_zero ? 0 :
                     in_ID_data.cpu_ctrl.alu_op1_sel_pc   ? in_ID_data.pc :
                     in_ID_data.rs1_value;

    assign alu_op2 = in_ID_data.cpu_ctrl.alu_op2_sel_imm ? in_ID_data.immediate :
                     in_ID_data.rs2_value;

    // ---------------------------------
    // AMO control
    // ---------------------------------
    // FIXME: Need to consider address mis-align for AMO. Current it is NOT
    // AMO instruction need to read and write memory in one instruction which is not compatible with pipeline structure
    // We perform the memory read and write all in EX stage and use a state machine to control the progress
    // Once the operation complete, then we send the result to MEM stage.
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            amo_state <= AMO_READ_REQ;
        end
        else begin
            case(amo_state)
                AMO_READ_REQ: begin
                    if (in_ID_data.cpu_ctrl.amo && memory_rsp.hready) amo_state <= AMO_READ_DATA;
                end
                AMO_READ_DATA: begin
                    if (memory_rsp.hready) begin
                        if (memory_rsp.hresp == 0) begin
                            amo_state <= AMO_WRITE_REQ;
                        end
                        else begin
                            amo_state <= AMO_READ_REQ;
                        end
                    end
                end
                AMO_WRITE_REQ: begin
                    if (memory_rsp.hready) begin
                        amo_state <= AMO_WRITE_DATA;
                    end
                end
                AMO_WRITE_DATA: begin
                    if (memory_rsp.hready) begin
                        amo_state <= AMO_READ_REQ;
                    end
                end
            endcase
        end
    end

    assign amo_read  = (amo_state == AMO_READ_REQ)  & in_ID_data.cpu_ctrl.amo;
    assign amo_write = (amo_state == AMO_WRITE_REQ) & in_ID_data.cpu_ctrl.amo;

    // need to place the write data into correct position aligned with address
    assign amo_wdata = in_ID_data.cpu_ctrl.is_32bit ?
                        (haddr[2] ? {amo_alu_out[31:0], 32'b0} : amo_alu_out) :
                        amo_alu_out;

    always_comb begin
        amo_stall = in_ID_data.cpu_ctrl.amo;
        if ((amo_state == AMO_READ_DATA  & memory_rsp.hready & memory_rsp.hresp == 1) ||
            (amo_state == AMO_WRITE_DATA & memory_rsp.hready))
            amo_stall = 1'b0;
    end

    always_ff @(posedge clk) begin
        if (amo_state == AMO_READ_DATA & memory_rsp.hready)
            // need to place the write data into correct position aligned with address
            amo_rdata <= in_ID_data.cpu_ctrl.is_32bit ?
                        (haddr[2] ? {{32{memory_rsp.hrdata[63]}}, memory_rsp.hrdata[63:32]} :
                                    {{32{memory_rsp.hrdata[31]}}, memory_rsp.hrdata[31:0]}) :
                        memory_rsp.hrdata;
    end

    // ---------------------------------
    // Data memory control
    // ---------------------------------

    // Dedicated AGU for memory access
    assign haddr = (in_ID_data.cpu_ctrl.amo | in_ID_data.cpu_ctrl.lr | in_ID_data.cpu_ctrl.sc) ?
                   in_ID_data.rs1_value : in_ID_data.rs1_value + in_ID_data.immediate;

    // HSIZE and addr alignment check
    always_comb begin
        mem_addr_misaligned = 0;
        hsize = {1'b0, in_ID_data.cpu_ctrl.mem_opcode[1:0]};
        case (in_ID_data.cpu_ctrl.mem_opcode[1:0])
            MEM_HALF: begin
                mem_addr_misaligned = haddr[0] != 0;
            end
            MEM_WORD: begin
                mem_addr_misaligned = haddr[1:0] != 0;
            end
            MEM_DWORD: begin
                mem_addr_misaligned = haddr[2:0] != 0;
            end
            default;
        endcase
    end

    assign load_addr_misaligned      = ~EX_flush & in_ID_data.cpu_ctrl.mem_read  & mem_addr_misaligned;
    assign store_amo_addr_misaligned = ~EX_flush & in_ID_data.cpu_ctrl.mem_write & mem_addr_misaligned;

    assign mem_read  = in_ID_data.cpu_ctrl.mem_read  | in_ID_data.cpu_ctrl.lr | amo_read;
    assign mem_write = in_ID_data.cpu_ctrl.mem_write | in_ID_data.cpu_ctrl.sc & sc_check_pass | amo_write;

    assign mem_request_raw = ~EX_flush & ~mem_addr_misaligned & (mem_read | mem_write);
    assign mem_request     = mem_request_raw & memory_rsp.hready;
    assign block_fetch     = mem_request_raw;

    assign hwdata = (amo_state == AMO_WRITE_DATA) ? amo_wdata : MEM_hwdata;

    // AHBLite request
    assign memory_req.haddr  = haddr;
    assign memory_req.htrans = {mem_request, 1'b0};
    assign memory_req.hwrite = mem_write;
    assign memory_req.hsize  = hsize;
    assign memory_req.hwdata = hwdata; // data is from MEM stage
    assign memory_req.hsel   = 1'b1;

    // ---------------------------------
    // Other logic
    // ---------------------------------

    assign ex_mem_read = in_ID_data.cpu_ctrl.mem_read | in_ID_data.cpu_ctrl.lr | in_ID_data.cpu_ctrl.amo;

    assign ex_rd_addr  = in_ID_data.rd_addr;
    assign ex_rd_write = in_ID_data.rd_write;
    assign ex_rd_wdata = in_ID_data.cpu_ctrl.amo ? amo_rdata :
                         in_ID_data.cpu_ctrl.sc  ?  {{`XLEN-1{1'b0}}, ~sc_check_pass} :
                         in_ID_data.cpu_ctrl.mul ? mul_out :
                         in_ID_data.cpu_ctrl.div ? div_out :
                                                   alu_out;

    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    assign EX_stall = mem_request_raw & ~memory_rsp.hready | mul_stall | div_stall | amo_stall;

    assign in_ID_ready = out_EX_ready & ~EX_stall;

    assign out_EX_valid = in_ID_valid & ~EX_flush;

    assign out_EX_data.pc          = in_ID_data.pc;
    assign out_EX_data.instruction = in_ID_data.instruction;
    assign out_EX_data.mem_read    = in_ID_data.cpu_ctrl.mem_read  | in_ID_data.cpu_ctrl.lr; // AMO is processed purely in EX stage
    assign out_EX_data.mem_write   = in_ID_data.cpu_ctrl.mem_write | (in_ID_data.cpu_ctrl.sc & sc_check_pass); // AMO is processed purely in EX stage
    assign out_EX_data.mem_opcode  = in_ID_data.cpu_ctrl.mem_opcode;
    assign out_EX_data.haddr       = haddr[2:0];
    assign out_EX_data.rs2_value   = in_ID_data.rs2_value;
    assign out_EX_data.rd_write    = in_ID_data.rd_write;
    assign out_EX_data.rd_addr     = in_ID_data.rd_addr;
    // for jal/jalr, store pc + 4 to rd
    assign out_EX_data.alu_out     = jump ? in_ID_data.pc + 4 : ex_rd_wdata;

    // exception
    assign out_EX_data.exception_valid = in_ID_data.exception_valid |
                                         instr_addr_misaligned | load_addr_misaligned | store_amo_addr_misaligned;
    assign out_EX_data.exception_cause = in_ID_data.exception_valid ? in_ID_data.exception_cause :
                                         instr_addr_misaligned      ? INST_ADDR_MISALIGNED :
                                         load_addr_misaligned       ? LOAD_ADDR_MISALIGNED :
                                                                      STORE_AMO_ADDR_MISALIGNED;

endmodule
