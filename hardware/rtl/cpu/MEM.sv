/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

module MEM (
    // verilog_format: off
    // pipeline control
    input                in_EX_valid,
    input EX2MEM_PIPE_t  in_EX_data,
    output               in_EX_ready,
    output               out_MEM_valid,
    output MEM2WB_PIPE_t out_MEM_data,
    input                out_MEM_ready,
    input                MEM_flush,
    // AHB response
    input AHBLiteRsp_t   memory_rsp,
    output logic [`XLEN-1:0] MEM_hwdata,
    // Data forwarding logic
    output [4:0]         mem_rd_addr,
    output               mem_rd_write,
    output [`XLEN-1:0]   mem_rd_wdata
    // verilog_format: on
);

    logic memory_request;
    logic memory_stall;
    logic load_access_fault;
    logic store_access_fault;

    logic sign_bit;
    logic sign_bit_final;
    logic sign_ext;

    logic [`XLEN-1:0] mem_rdata;

    logic [`XLEN-1:0] rd_wdata;

    // ---------------------------------
    // Main components
    // ---------------------------------

    // ---------------------------------
    // AHBLite Resp Control
    // ---------------------------------

    // write data is provided in DATA phase
    assign memory_request = in_EX_data.mem_ctrl.mem_read | in_EX_data.mem_ctrl.mem_write;
    assign memory_stall = memory_request & ~memory_rsp.hready;

    assign load_access_fault = in_EX_data.mem_ctrl.mem_read & memory_rsp.hready & memory_rsp.hresp;
    assign store_access_fault = in_EX_data.mem_ctrl.mem_write & memory_rsp.hready & memory_rsp.hresp;
    ;


    // ---------------------------------
    // Read/Write data generation
    // ---------------------------------

    // 000  LB *
    // 001  LH *
    // 010  LW *
    // 100  LBU
    // 101  LHU
    // opcode[2] is zero for signed extension
    assign sign_ext       = ~in_EX_data.mem_ctrl.mem_opcode[2];
    assign sign_bit_final = sign_bit & sign_ext;

    // We assume that the memory does not align the read data with the address.
    // so we need to re-align the data with address here.
    always_comb begin
        sign_bit = 0;
        MEM_hwdata = 'b0;
        case (in_EX_data.mem_ctrl.mem_opcode[1:0])
            MEM_DWORD: begin
                mem_rdata  = memory_rsp.hrdata;
                MEM_hwdata = in_EX_data.rs2_value;
            end
            MEM_WORD: begin
                case (in_EX_data.haddr[2])
                    0: begin
                        mem_rdata[31:0] = memory_rsp.hrdata[31:0];
                        MEM_hwdata[31:0] = in_EX_data.rs2_value[31:0];
                        sign_bit = memory_rsp.hrdata[31];
                    end
                    1: begin
                        mem_rdata[31:0] = memory_rsp.hrdata[63:32];
                        MEM_hwdata[63:32] = in_EX_data.rs2_value[31:0];
                        sign_bit = memory_rsp.hrdata[63];
                    end
                endcase
                mem_rdata[63:32] = {32{sign_bit_final}};
            end
            MEM_HALF: begin  // LH/LHU
                case (in_EX_data.haddr[2:1])
                    0: begin
                        mem_rdata[15:0] = memory_rsp.hrdata[15:0];
                        MEM_hwdata[15:0] = in_EX_data.rs2_value[15:0];
                        sign_bit = memory_rsp.hrdata[15];
                    end
                    1: begin
                        mem_rdata[15:0] = memory_rsp.hrdata[31:16];
                        MEM_hwdata[31:16] = in_EX_data.rs2_value[15:0];
                        sign_bit = memory_rsp.hrdata[31];
                    end
                    2: begin
                        mem_rdata[15:0] = memory_rsp.hrdata[47:32];
                        MEM_hwdata[47:32] = in_EX_data.rs2_value[15:0];
                        sign_bit = memory_rsp.hrdata[47];
                    end
                    3: begin
                        mem_rdata[15:0] = memory_rsp.hrdata[63:48];
                        MEM_hwdata[63:48] = in_EX_data.rs2_value[15:0];
                        sign_bit = memory_rsp.hrdata[63];
                    end
                endcase
                mem_rdata[63:16] = {48{sign_bit_final}};
            end
            default: begin  // LB/LBU
                case (in_EX_data.haddr)
                    0: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[7:0];
                        MEM_hwdata[7:0] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[7];
                    end
                    1: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[15:8];
                        MEM_hwdata[15:8] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[15];
                    end
                    2: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[23:16];
                        MEM_hwdata[23:16] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[23];
                    end
                    3: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[31:24];
                        MEM_hwdata[31:24] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[31];
                    end
                    4: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[39:32];
                        MEM_hwdata[39:32] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[39];
                    end
                    5: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[47:40];
                        MEM_hwdata[47:40] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[47];
                    end
                    6: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[55:48];
                        MEM_hwdata[55:48] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[55];
                    end
                    7: begin
                        mem_rdata[7:0] = memory_rsp.hrdata[63:56];
                        MEM_hwdata[63:56] = in_EX_data.rs2_value[7:0];
                        sign_bit = memory_rsp.hrdata[63];
                    end
                endcase
                mem_rdata[63:8] = {56{sign_bit_final}};
            end
        endcase
    end

    // ---------------------------------
    // Other logic
    // ---------------------------------

    assign rd_wdata = in_EX_data.mem_ctrl.mem_read ? mem_rdata : in_EX_data.alu_out;

    assign mem_rd_addr = in_EX_data.rd_addr;
    assign mem_rd_write = in_EX_data.rd_write;
    assign mem_rd_wdata = rd_wdata;

    // ---------------------------------
    // Pipeline Control
    // ---------------------------------

    assign in_EX_ready = out_MEM_ready & ~memory_stall;

    // ID is valid when 1）ID stage is valid, 2) not stalled due to load_dependency
    assign out_MEM_valid = in_EX_valid & ~MEM_flush & ~memory_stall;

    assign out_MEM_data.pc = in_EX_data.pc;
    assign out_MEM_data.instruction = in_EX_data.instruction;
    assign out_MEM_data.rd_write = in_EX_data.rd_write;
    assign out_MEM_data.rd_addr = in_EX_data.rd_addr;
    assign out_MEM_data.rd_wdata = rd_wdata;

    // exception
    assign out_MEM_data.exception_valid = in_EX_data.exception_valid |
                                          load_access_fault | store_access_fault;
    assign out_MEM_data.exception_cause = in_EX_data.exception_valid ? in_EX_data.exception_cause :
                                          load_access_fault          ? LOAD_ACCESS_FAULT :
                                                                       STORE_AMO_ACCESS_FAULT;


endmodule
