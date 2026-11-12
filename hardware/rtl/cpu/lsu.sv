/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

/*
 * LSU is across EX and MEM stage.
 * When ever the LSU accept the request from EX stage, it is considered complete in EX stage and moving forward to MEM stage.
 */


// FIXME: hresp = 1 (error state) is not handled correctly !
// FIXME: lsu_flush?

module lsu #(
    parameter DEPTH = 1
) (
    input              clk,
    input              rst_n,
    // request
    input              lsu_req_read,
    input              lsu_req_write,
    input              lsu_req_lr,
    input              lsu_req_sc,
    input              lsu_req_amo,
    input  [`XLEN-1:0] lsu_req_addr,
    input  [`XLEN-1:0] lsu_req_wdata,
    input  [2:0]       lsu_req_mem_opcode,
    input  [4:0]       lsu_req_amo_opcode,
    input              lsu_req_is32bit,
    output             lsu_req_ready,
    // response
    output             lsu_rsp_valid,
    output [`XLEN-1:0] lsu_rsp_data,
    output             load_access_fault,
    output             store_amo_access_fault,
    // Exception and status
    output             sc_check_pass,
    output             load_addr_misaligned,
    output             store_amo_addr_misaligned,
    // AMO read data
    output logic [`XLEN-1:0] amo_rdata,
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

    // FIXME: AMO_READ_DATA and AMO_WRITE_ADDR can be combined
    typedef enum logic [2:0] {IDLE, AHB_DATA, AMO_READ_DATA, AMO_WRITE_ADDR, AMO_WRITE_DATA} LSU_state_t;

    LSU_state_t lsu_state;

    logic mem_addr_misaligned;
    logic lsu_non_amo_req;
    logic lsu_non_amo_req_p;

    // register the request from address phase
    logic             lsu_req_read_p;
    logic             lsu_req_write_p;
    logic             lsu_req_lr_p;
    logic             lsu_req_sc_p;
    logic             lsu_req_is32bit_p;
    logic [`XLEN-1:0] lsu_req_addr_p;
    logic [2:0]       lsu_req_mem_opcode_p;
    logic [`XLEN-1:0] lsu_req_wdata_p;

    // LR/SC reservation
    logic [`XLEN-1:0] haddr_end;
    logic             reserve_valid;
    logic [`XLEN-1:0] reserve_addr_start;
    logic [`XLEN-1:0] reserve_addr_end;

    // AMO
    logic             amo_complete;
    logic             amo_read;
    logic             amo_write;
    logic [`XLEN-1:0] amo_alu_out;
    logic [`XLEN-1:0] amo_wdata;

    // AHB request
    logic             mem_read;
    logic             mem_write;
    logic             mem_request;

    // AHB response
    logic             sign_bit;
    logic             sign_bit_final;
    logic             sign_ext;
    logic [`XLEN-1:0] rdata;
    logic [`XLEN-1:0] wdata;

    // ----------------------------------------------------
    // LSU request and AHB address phase control
    // ----------------------------------------------------

    assign lsu_non_amo_req   = lsu_req_lr   | lsu_req_sc   | lsu_req_read   | lsu_req_write;
    assign lsu_non_amo_req_p = lsu_req_lr_p | lsu_req_sc_p | lsu_req_read_p | lsu_req_write_p;

    // state machine control
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            lsu_state <= IDLE;
        end
        else begin
            // FIXME: NOT handling hresp = 1 cases.
            /* verilator lint_off CASEINCOMPLETE */
            case (lsu_state)
                IDLE: begin
                    if (lsu_non_amo_req  &&  hready) lsu_state <= AHB_DATA;
                    else if (lsu_req_amo &&  hready) lsu_state <= AMO_READ_DATA;
                end
                AHB_DATA: begin
                    if (hready) begin
                        if (lsu_req_amo) lsu_state <= AMO_READ_DATA;
                        else if (lsu_non_amo_req) lsu_state <= AHB_DATA;
                        else lsu_state <= IDLE;
                    end
                end
                AMO_READ_DATA: if (hready) lsu_state <= AMO_WRITE_ADDR;
                AMO_WRITE_ADDR: if (hready) lsu_state <= AMO_WRITE_DATA;
                AMO_WRITE_DATA: if (hready) lsu_state <= IDLE;
            endcase
            /* verilator lint_on CASEINCOMPLETE */
        end
    end

    // addr alignment check
    always_comb begin
        mem_addr_misaligned = 0;
        case (lsu_req_mem_opcode[1:0])
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

    assign load_addr_misaligned      = mem_addr_misaligned & (lsu_req_read  | lsu_req_lr);
    assign store_amo_addr_misaligned = mem_addr_misaligned & (lsu_req_write | lsu_req_sc | lsu_req_amo);

    // LSU ready state:
    // 1) request is NOT an AMO and the AHB bus is ready to take the request
    // 2) request is AMO and AMO instruction complete
    assign lsu_req_ready  = (lsu_non_amo_req && !mem_addr_misaligned && hready) ||      // 1
                            (lsu_req_amo     && !mem_addr_misaligned && amo_complete);  // 2


    // register the request information for data phase
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            lsu_req_read_p <= 1'b0;
            lsu_req_write_p <= 1'b0;
            lsu_req_lr_p <= 1'b0;
            lsu_req_sc_p <= 1'b0;
        end
        else begin
            if (lsu_req_read && lsu_req_ready && hready) lsu_req_read_p <= 1'b1;
            else if (lsu_req_read_p && hready) lsu_req_read_p <= 1'b0;

            if (lsu_req_write && lsu_req_ready && hready) lsu_req_write_p <= 1'b1;
            else if (lsu_req_write_p && hready) lsu_req_write_p <= 1'b0;

            if (lsu_req_lr && lsu_req_ready && hready) lsu_req_lr_p <= 1'b1;
            else if (lsu_req_lr_p && hready) lsu_req_lr_p <= 1'b0;

            if (lsu_req_sc && lsu_req_ready && hready) lsu_req_sc_p <= 1'b1;
            else if (lsu_req_sc_p && hready) lsu_req_sc_p <= 1'b0;

            if (hready) begin
                lsu_req_addr_p <= lsu_req_addr;
                lsu_req_mem_opcode_p <= lsu_req_mem_opcode;
                lsu_req_wdata_p <= lsu_req_wdata;
                lsu_req_is32bit_p <= lsu_req_is32bit;
            end
        end
    end

    // ----------------------------------------------------
    // LSU response and AHB data phase control
    // ----------------------------------------------------

    assign lsu_rsp_valid = (lsu_non_amo_req_p & hready) | amo_complete;
    assign lsu_rsp_data = rdata;

    // Process Read and Write Data
    // Opcode
    // 000  LB/SB
    // 001  LH/SH
    // 010  LW/SW
    // 011  LD/SD
    // 100  LBU
    // 101  LHU
    // opcode[2] is zero for signed extension
    assign sign_ext       = ~lsu_req_mem_opcode_p[2];
    assign sign_bit_final = sign_bit & sign_ext;

    always_comb begin
        sign_bit = 0;
        wdata = '0;
        case (lsu_req_mem_opcode_p[1:0])
            MEM_DWORD: begin
                rdata  = hrdata;
                wdata = lsu_req_wdata_p;
            end
            MEM_WORD: begin
                case (lsu_req_addr_p[2])
                    0: begin
                        rdata[31:0] = hrdata[31:0];
                        wdata[31:0] = lsu_req_wdata_p[31:0];
                        sign_bit = hrdata[31];
                    end
                    1: begin
                        rdata[31:0] = hrdata[63:32];
                        wdata[63:32] = lsu_req_wdata_p[31:0];
                        sign_bit = hrdata[63];
                    end
                endcase
                rdata[63:32] = {32{sign_bit_final}};
            end
            MEM_HALF: begin  // LH/LHU
                case (lsu_req_addr_p[2:1])
                    0: begin
                        rdata[15:0] = hrdata[15:0];
                        wdata[15:0] = lsu_req_wdata_p[15:0];
                        sign_bit = hrdata[15];
                    end
                    1: begin
                        rdata[15:0] = hrdata[31:16];
                        wdata[31:16] = lsu_req_wdata_p[15:0];
                        sign_bit = hrdata[31];
                    end
                    2: begin
                        rdata[15:0] = hrdata[47:32];
                        wdata[47:32] = lsu_req_wdata_p[15:0];
                        sign_bit = hrdata[47];
                    end
                    3: begin
                        rdata[15:0] = hrdata[63:48];
                        wdata[63:48] = lsu_req_wdata_p[15:0];
                        sign_bit = hrdata[63];
                    end
                endcase
                rdata[63:16] = {48{sign_bit_final}};
            end
            default: begin  // LB/LBU
                case (lsu_req_addr_p[2:0])
                    0: begin
                        rdata[7:0] = hrdata[7:0];
                        wdata[7:0] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[7];
                    end
                    1: begin
                        rdata[7:0] = hrdata[15:8];
                        wdata[15:8] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[15];
                    end
                    2: begin
                        rdata[7:0] = hrdata[23:16];
                        wdata[23:16] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[23];
                    end
                    3: begin
                        rdata[7:0] = hrdata[31:24];
                        wdata[31:24] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[31];
                    end
                    4: begin
                        rdata[7:0] = hrdata[39:32];
                        wdata[39:32] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[39];
                    end
                    5: begin
                        rdata[7:0] = hrdata[47:40];
                        wdata[47:40] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[47];
                    end
                    6: begin
                        rdata[7:0] = hrdata[55:48];
                        wdata[55:48] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[55];
                    end
                    7: begin
                        rdata[7:0] = hrdata[63:56];
                        wdata[63:56] = lsu_req_wdata_p[7:0];
                        sign_bit = hrdata[63];
                    end
                endcase
                rdata[63:8] = {56{sign_bit_final}};
            end
        endcase
    end

    // ---------------------------------
    // LR/SC
    // ---------------------------------

    assign haddr_end = haddr + (lsu_req_is32bit ? 4 : 8);

    // check SC result
    assign sc_check_pass = reserve_valid & (haddr >= reserve_addr_start) & (haddr_end <= reserve_addr_end);

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            reserve_valid <= 1'b0;
            reserve_addr_start <= 'b0;
            reserve_addr_end <= 'b0;
        end
        else begin
            // Update reservation when LR completes
            if (lsu_req_lr_p && hready) begin
                reserve_valid      <= 1'b1;
                reserve_addr_start <= lsu_req_addr_p;
                reserve_addr_end   <= lsu_req_addr_p + (lsu_req_is32bit_p ? 4 : 8);
            end
            // Clear reservation when seeing SC (after request being accepted or generate exception, or check failed)
            else if (lsu_req_sc && (hready || mem_addr_misaligned || !sc_check_pass)) begin
                reserve_valid      <= 1'b0;
                reserve_addr_start <= 'b0;
                reserve_addr_end   <= 'b0;
            end
        end
    end

    // ---------------------------------
    // AMO
    // ---------------------------------

    assign amo_read  = lsu_req_amo & (lsu_state == AHB_DATA || lsu_state == IDLE);
    assign amo_write = lsu_req_amo & (lsu_state == AMO_WRITE_ADDR);
    // need to place the write data into correct position aligned with address
    assign amo_wdata = lsu_req_is32bit ? (haddr[2] ? {amo_alu_out[31:0], 32'b0} : amo_alu_out) : amo_alu_out;
    // amo is completed when the WRITE has complete
    assign amo_complete = (lsu_state == AMO_WRITE_DATA) & hready;

    // store the read data for AMO
    always_ff @(posedge clk) begin
        // need to place the write data into correct position aligned with address
        if (lsu_state == AMO_READ_DATA & hready)
            amo_rdata <= lsu_req_is32bit ?
                        (haddr[2] ? {{32{hrdata[63]}}, hrdata[63:32]} :
                                    {{32{hrdata[31]}}, hrdata[31:0]}) :
                        hrdata;
    end


    // AMO ALU
    amo_alu u_amo_alu(
        .a        (amo_rdata), // loaded value
        .b        (lsu_req_wdata_p),
        .opcode   (lsu_req_amo_opcode),
        .is_32bit (lsu_req_is32bit),
        .o        (amo_alu_out)
    );

    // ------------------------------------------
    // AHB Control logic
    // ------------------------------------------

    assign mem_read    = lsu_req_read  | lsu_req_lr | amo_read;
    assign mem_write   = lsu_req_write | lsu_req_sc & sc_check_pass | amo_write;
    assign mem_request = ~mem_addr_misaligned && (mem_read || mem_write);

    assign haddr  = lsu_req_addr;
    assign htrans = {mem_request, 1'b0};
    assign hwrite = mem_write;
    assign hwdata = (lsu_state == AMO_WRITE_DATA) ? amo_wdata : wdata;
    assign hsize  = {1'b0, lsu_req_mem_opcode[1:0]};
    assign hlock  = amo_read | amo_write;

    // FIMXE: hresp
    assign load_access_fault = 1'b0;
    assign store_amo_access_fault = 1'b0;

endmodule
