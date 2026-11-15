/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"
`include "csr.svh"
module csr
(
    input               clk,
    input               rst_n,

    // CSR access interface
    input             csr_read,
    input             csr_write,
    input [11:0]      csr_address,
    input [1:0]       csr_opcode,
    input [`XLEN-1:0] csr_wdata,
    output logic [`XLEN-1:0] csr_rdata
);

    logic [63:0] csr_wdata_final;

    mstatus_t       mstatus;
    misa_t          misa;
    mie_t           mie;
    mtvec_t         mtvec;
    mcounteren_t    mcounteren;
    mcountinhibit_t mcountinhibit;
    mscratch_t      mscratch;
    mepc_t          mepc;
    mcause_t        mcause;
    mtval_t         mtval;
    mip_t           mip;
    mcycle_t        mcycle;
    minstret_t      minstret;
    mvendorid_t     mvendorid;
    marchid_t       marchid;
    mimpid_t        mimpid;
    mhartid_t       mhartid;

    // --------------------------------
    // Write Data generation
    // --------------------------------

    always @(*) begin
        case(csr_opcode)
            CSR_RS:  csr_wdata_final = csr_rdata |  csr_wdata;  // set
            CSR_RC:  csr_wdata_final = csr_rdata & ~csr_wdata; // clear
            default: csr_wdata_final = csr_wdata;
        endcase
    end


    // --------------------------------
    // Read Logic
    // --------------------------------
    always @(*) begin
        case(csr_address)
            CSR_MSTATUS:        csr_rdata = mstatus_read(mstatus);
            CSR_MISA:           csr_rdata = misa_read(misa);
            CSR_MIE:            csr_rdata = mie_read(mie);
            CSR_MTVEC:          csr_rdata = mtvec_read(mtvec);
            CSR_MCOUNTEREN:     csr_rdata = mcounteren_read(mcounteren);
            CSR_MCOUNTINHIBIT:  csr_rdata = mcountinhibit_read(mcountinhibit);
            CSR_MSCRATCH:       csr_rdata = mscratch_read(mscratch);
            CSR_MEPC:           csr_rdata = mepc_read(mepc);
            CSR_MCAUSE:         csr_rdata = mcause_read(mcause);
            CSR_MTVAL:          csr_rdata = mtval_read(mtval);
            CSR_MIP:            csr_rdata = mip_read(mip);
            CSR_MCYCLE:         csr_rdata = mcycle_read(mcycle);
            CSR_MINSTRET:       csr_rdata = minstret_read(minstret);
            CSR_MVENDORID:      csr_rdata = mvendorid_read(mvendorid);
            CSR_MARCHID:        csr_rdata = marchid_read(marchid);
            CSR_MIMPID:         csr_rdata = mimpid_read(mimpid);
            CSR_MHARTID:        csr_rdata = mhartid_read(mhartid);
            default:            csr_rdata = 0;
        endcase
    end

    // --------------------------------
    // Write Logic
    // --------------------------------

    // mstatus
    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            mstatus <= 0;
        end
        else begin
            if (csr_write && csr_address == CSR_MSTATUS) begin
                mstatus <= mstatus_write(mstatus, csr_wdata_final);
            end
        end
    end


endmodule
