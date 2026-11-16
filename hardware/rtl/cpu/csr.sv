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

    // Entering trap
    input               trap_enter,
    input [`XLEN-1:0]   trap_cause,
    input [`XLEN-1:0]   trap_tval,
    input [`XLEN-1:0]   trap_pc,
    input [1:0]         trap_priv,
    // CSR state for the trap controller
    output [`XLEN-1:0]  o_medeleg,
    output [`XLEN-1:0]  o_mideleg,
    output [`XLEN-1:0]  o_mtvec,
    output [`XLEN-1:0]  o_mepc,
    output              o_mstatus_tsr,
    output logic [1:0]  current_priv,
    // Returning from trap
    input               sret,
    input               mret,
    // MISC
    input               instret, // instruction retire
    input [`XLEN-1:0]   mtime,   // from CLINT
    // CSR access interface
    input               csr_read,
    input               csr_write,
    input [11:0]        csr_address,
    input [1:0]         csr_opcode,
    input [`XLEN-1:0]   csr_wdata,
    output logic [`XLEN-1:0] csr_rdata,
    output              csr_illegal_instruction
);

    logic [63:0] csr_wdata_final;

    logic [1:0] addr_priv;

    logic       csr_wrong_priv;
    logic       csr_access_unimp;


    // Machine Trap Setup
    mstatus_t       mstatus;
    misa_t          misa;
    medeleg_t       medeleg;
    mideleg_t       mideleg;
    mie_t           mie;
    mtvec_t         mtvec;
    mcounteren_t    mcounteren;
    // Machine Counter Setup
    mcountinhibit_t mcountinhibit;
    // Machine Trap Handling
    mscratch_t      mscratch;
    mepc_t          mepc;
    mcause_t        mcause;
    mtval_t         mtval;
    mip_t           mip;
    // Machine Counters
    mcycle_t        mcycle;
    minstret_t      minstret;
    // Machine Information
    mvendorid_t     mvendorid;
    marchid_t       marchid;
    mimpid_t        mimpid;
    mhartid_t       mhartid;
    // Supervisor Trap Setup
    stvec_t         stvec;
    scounteren_t    scounteren;
    sscratch_t      sscratch;
    sepc_t          sepc;
    scause_t        scause;
    stval_t         stval;
    // Supervisor Address Translation and Protection
    satp_t          satp;


    // --------------------------------
    // Privilege Check and update
    // --------------------------------

    // Update privilege
    always @(posedge clk, negedge rst_n) begin
        if (!rst_n)
            current_priv <= PRIV_M; // start with M mode
        else begin
            // entering trap change the privilege to trap privilege
            if (trap_enter) current_priv <= trap_priv;
            // exit trap restore privilege from mstatus.MPP field
            else if (sret || mret) current_priv <= mstatus.mpp;
        end
    end

    // Check the privilege for CSR access. Lower privilege can't access CSR that are only accessible by higher privilege
    assign addr_priv = csr_address[9:8];
    always_comb begin
        csr_wrong_priv = 1'b0;
        if ((current_priv == PRIV_U && addr_priv > PRIV_U) || // U-mode can only access csr in User-Level CSR
            (current_priv == PRIV_S && addr_priv > PRIV_S))   // S-mode can only access Supervisor/User-level CSR
            csr_wrong_priv = 1'b1;
    end

    // --------------------------------
    // Write Data generation
    // --------------------------------

    always_comb begin
        case(csr_opcode)
            CSR_RS:  csr_wdata_final = csr_rdata |  csr_wdata; // set
            CSR_RC:  csr_wdata_final = csr_rdata & ~csr_wdata; // clear
            default: csr_wdata_final = csr_wdata;
        endcase
    end

    // --------------------------------
    // CSR Read Logic
    // --------------------------------
    always_comb begin
        csr_access_unimp = 1'b0;
        case(csr_address)
            CSR_MSTATUS: csr_rdata = mstatus_read(mstatus);
            CSR_MISA:    csr_rdata = misa_read(misa);
            CSR_MEDELEG: csr_rdata = medeleg_read(medeleg);
            CSR_MIDELEG: csr_rdata = mideleg_read(mideleg);
            CSR_MIE:     csr_rdata = mie_read(mie);
            CSR_MTVEC:   csr_rdata = mtvec_read(mtvec);
            CSR_MCOUNTEREN: csr_rdata = mcounteren_read(mcounteren);
            CSR_MCOUNTINHIBIT: csr_rdata = mcountinhibit_read(mcountinhibit);
            CSR_MSCRATCH: csr_rdata = mscratch_read(mscratch);
            CSR_MEPC:     csr_rdata = mepc_read(mepc);
            CSR_MCAUSE:   csr_rdata = mcause_read(mcause);
            CSR_MTVAL:    csr_rdata = mtval_read(mtval);
            CSR_MIP:      csr_rdata = mip_read(mip);
            CSR_MCYCLE:   csr_rdata = mcycle_read(mcycle);
            CSR_MINSTRET: csr_rdata = minstret_read(minstret);
            CSR_SSTATUS:  csr_rdata = sstatus_read(mstatus);
            CSR_SIE:      csr_rdata = sie_read(mie);
            CSR_STVEC:    csr_rdata = stvec_read(stvec);
            CSR_SCOUNTEREN: csr_rdata = scounteren_read(scounteren);
            CSR_SSCRATCH: csr_rdata = sscratch_read(sscratch);
            CSR_SEPC:     csr_rdata = sepc_read(sepc);
            CSR_SCAUSE:   csr_rdata = scause_read(scause);
            CSR_STVAL:    csr_rdata = sepc_read(stval);
            CSR_SIP:      csr_rdata = sip_read(mip);
            CSR_SATP:     csr_rdata = satp_read(satp);
            CSR_CYCLE: begin
                csr_rdata = cycle_read(mcycle);
                if (current_priv != PRIV_M && !mcounteren.cy) csr_access_unimp = 1'b1;
            end
            CSR_TIME: begin
                csr_rdata = time_read(mtime);
                if (current_priv != PRIV_M && !mcounteren.tm) csr_access_unimp = 1'b1;
            end
            CSR_INSTRET: begin
                csr_rdata = instret_read(minstret);
                if (current_priv != PRIV_M && !mcounteren.tm) csr_access_unimp = 1'b1;
            end
            CSR_MVENDORID: csr_rdata = mvendorid_read(mvendorid);
            CSR_MARCHID:   csr_rdata = marchid_read(marchid);
            CSR_MIMPID:    csr_rdata = mimpid_read(mimpid);
            CSR_MHARTID:   csr_rdata = mhartid_read(mhartid);
            default: begin
                csr_rdata = 0;
                csr_access_unimp = 1;
            end
        endcase
    end

    // --------------------------------
    // CSR Write Logic
    // --------------------------------

    // common CSR without hardware update
    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            medeleg <= 0;
            mideleg <= 0;
            mie <= 0;
            mtvec <= 0;
            mcounteren <= 0;
            mcountinhibit <= 0;
            mscratch <= 0;
            stvec <= 0;
            scounteren <= 0;
            sscratch <= 0;
            sepc <= 0;
            scause <= 0;
            stval <= 0;
            satp <= 0;
        end
        else begin
            case (csr_address)
                CSR_MEDELEG:    if (csr_write) medeleg <= medeleg_write(csr_wdata_final);
                CSR_MIDELEG:    if (csr_write) mideleg <= mideleg_write(csr_wdata_final);
                CSR_MIE:        if (csr_write) mie <= mie_write(csr_wdata_final);
                CSR_MTVEC:      if (csr_write) mtvec <= mtvec_write(csr_wdata_final);
                CSR_MCOUNTEREN: if (csr_write) mcounteren <= mcounteren_write(csr_wdata_final);
                CSR_MCOUNTINHIBIT: if (csr_write) mcountinhibit <= mcountinhibit_write(csr_wdata_final);
                CSR_MSCRATCH:      if (csr_write) mscratch <= mscratch_write(csr_wdata_final);
                CSR_SSTATUS:    if (csr_write) mstatus <= sstatus_write(csr_wdata_final);
                CSR_SIE:        if (csr_write) mie <= sie_write(csr_wdata_final);
                CSR_STVEC:      if (csr_write) stvec <= stvec_write(csr_wdata_final);
                CSR_SCOUNTEREN: if (csr_write) scounteren <= scounteren_write(csr_wdata_final);
                CSR_SSCRATCH:   if (csr_write) sscratch <= sscratch_write(csr_wdata_final);
                CSR_SEPC:       if (csr_write) sepc <= sepc_write(csr_wdata_final);
                CSR_SCAUSE:     if (csr_write) scause <= scause_write(csr_wdata_final);
                CSR_STVAL:      if (csr_write) stval <= sepc_write(csr_wdata_final);
                CSR_SIP:        if (csr_write) mip <= sip_write(csr_wdata_final);
                CSR_SATP:       if (csr_write) satp <= satp_write(csr_wdata_final);
                default: begin end
            endcase
        end
    end

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            mstatus <= 0;
            mepc <= 0;
            mcause <= 0;
            mtval <= 0;
            mcycle <= 0;
            minstret <= 0;
        end
        else begin

            if (csr_write && csr_address == CSR_MSTATUS) begin
                mstatus <= mstatus_write(csr_wdata_final);
            end
            else begin
                if (trap_enter) begin
                    if (trap_priv == PRIV_M) begin
                        mstatus.mpie <= mstatus.mie; // MPIE = MIE
                        mstatus.mie <= 1'b0;         // MIE = 0
                        mstatus.mpp <= current_priv; // MPP = previous privilege
                    end
                    else begin
                        mstatus.spie <= mstatus.sie; // SPIE = SIE
                        mstatus.sie <= 1'b0;         // SIE = 0
                        mstatus.spp <= current_priv[0]; // SPP = previous privilege
                    end
                end
                // From SPEC:
                // An MRET or SRET instruction is used to return from a trap in M-mode or S-mode respectively.
                // When executing an xRET instruction, supposing xPP holds the value y, xIE is set to xPIE;
                // the privilege mode is changed to y; xPIE is set to 1; and xPP is set to the least-privileged
                // supported mode (U if U-mode is implemented, else M). If y≠M, xRET also sets MPRV=0.
                else if (mret) begin
                    // Restore MIE and set MPIE to 1
                    mstatus.mie <= mstatus.mpie;
                    mstatus.mpie <= 1'b1;
                    mstatus.mpp <= PRIV_U;
                    if (mstatus.mpp != PRIV_M) mstatus.mprv <= 0;
                end
                else if (sret) begin
                    // Restore SIE and set SPIE to 1
                    mstatus.sie <= mstatus.spie;
                    mstatus.spie <= 1'b1;
                    mstatus.spp <= PRIV_U[0];
                    mstatus.mprv <= 0;
                end
            end

            if (csr_write && csr_address == CSR_MEPC) mepc <= mepc_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_M) mepc <= trap_pc;

            if (csr_write && csr_address == CSR_MCAUSE) mcause <= mcause_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_M) mcause <= trap_cause;

            if (csr_write && csr_address == CSR_MTVAL) mtval <= mtval_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_M) mtval <= trap_cause;

            if (csr_write && csr_address == CSR_MCYCLE) mcycle <= mcycle_write(csr_wdata_final);
            else mcycle <= mcycle + 1'b1;

            if (csr_write && csr_address == CSR_MINSTRET) minstret <= minstret_write(csr_wdata_final);
            else if (instret) minstret <= minstret + 1'b1;
        end
    end

    // --------------------------------
    // Check illegal instruction
    // --------------------------------
    assign csr_illegal_instruction = (csr_read | csr_write) & (csr_wrong_priv | csr_access_unimp);

    // --------------------------------
    // CSR Information to other Logic
    // --------------------------------
    assign o_medeleg = medeleg_read(medeleg);
    assign o_mideleg = mideleg_read(mideleg);
    assign o_mtvec   = mtvec_read(mtvec);
    assign o_mepc    = mepc_read(mepc);
    assign o_mstatus_tsr = mstatus.tsr;


endmodule
