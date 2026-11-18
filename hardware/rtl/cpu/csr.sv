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
    input [4:0]         trap_cause,
    input               trap_interrupt,
    input [`XLEN-1:0]   trap_tval,
    input [`XLEN-1:0]   trap_pc,
    input [1:0]         trap_priv,
    // Returning from trap
    input               sret,
    input               mret,
    // CSR state for the trap controller
    output [`XLEN-1:0]  o_medeleg,
    output [`XLEN-1:0]  o_mideleg,
    output [`XLEN-1:0]  o_mtvec,
    output [`XLEN-1:0]  o_stvec,
    output [`XLEN-1:0]  o_mepc,
    output [`XLEN-1:0]  o_sepc,
    output              o_mstatus_tsr,
    output              o_mstatus_tw,
    output              o_mstatus_tvm,
    output              o_mstatus_mie,
    output              o_mstatus_sie,
    output mie_t        o_mie,
    output logic [1:0]  current_priv,
    // External Interrupt
    input               cpu_msip,
    input               cpu_mtip,
    input               cpu_meip,
    input               cpu_seip,
    // Supervisor mode interrupt output
    output              csr_ssip,
    output              csr_stip,
    output              csr_seip,
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
    logic [`XLEN-1:0] csr_rmw_data;

    logic cpu_seip_int;
    logic seip_pending;

    logic [1:0] addr_priv;

    logic       csr_wrong_priv;
    logic       csr_read_unimp;
    logic       csr_write_ro;

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
            else if (mret) current_priv <= mstatus.mpp;
            else if (sret) current_priv <= {1'b0, mstatus.spp};
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
        // External SEIP is visible to reads, but should not be use as read data for CSR Read-modified-write.
        csr_rmw_data = csr_rdata;
        if (csr_address == CSR_MIP)
            csr_rmw_data[9] = mip.seip;

        case(csr_opcode)
            CSR_RS:  csr_wdata_final = csr_rmw_data |  csr_wdata; // set
            CSR_RC:  csr_wdata_final = csr_rmw_data & ~csr_wdata; // clear
            default: csr_wdata_final = csr_wdata;
        endcase
    end

    // --------------------------------
    // CSR Decode and Read Logic
    // --------------------------------
    always_comb begin
        csr_read_unimp = 1'b0;
        csr_write_ro = 1'b0;
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
            CSR_MIP: begin
                csr_rdata = mip_read(mip);
                csr_rdata[9] = seip_pending;
            end
            CSR_MCYCLE:   csr_rdata = mcycle_read(mcycle);
            CSR_MINSTRET: csr_rdata = minstret_read(minstret);
            CSR_SSTATUS:  csr_rdata = sstatus_read(mstatus);
            CSR_SIE: begin
                // SIE only seens interrupt being delegated in mideleg
                csr_rdata = sie_read(mie) & mideleg_read(mideleg);
            end
            CSR_STVEC:    csr_rdata = stvec_read(stvec);
            CSR_SCOUNTEREN: csr_rdata = scounteren_read(scounteren);
            CSR_SSCRATCH: csr_rdata = sscratch_read(sscratch);
            CSR_SEPC:     csr_rdata = sepc_read(sepc);
            CSR_SCAUSE:   csr_rdata = scause_read(scause);
            CSR_STVAL:    csr_rdata = sepc_read(stval);
            CSR_SIP: begin
                csr_rdata = sip_read(mip);
                csr_rdata[9] = seip_pending;
                // SIP only sees interrupt being delegated in midelege
                csr_rdata = csr_rdata & mideleg_read(mideleg);
            end
            CSR_SATP: begin
                csr_rdata = satp_read(satp);
                if (mstatus.tvm && current_priv != PRIV_M) csr_read_unimp = 1'b1;
            end
            CSR_CYCLE: begin
                csr_rdata = cycle_read(mcycle);
                if (current_priv != PRIV_M && !mcounteren.cy) csr_read_unimp = 1'b1;
                if (csr_write) csr_write_ro = 1'b1;
            end
            CSR_TIME: begin
                csr_rdata = time_read(mtime);
                if (current_priv != PRIV_M && !mcounteren.tm) csr_read_unimp = 1'b1;
                if (csr_write) csr_write_ro = 1'b1;
            end
            CSR_INSTRET: begin
                csr_rdata = instret_read(minstret);
                if (current_priv != PRIV_M && !mcounteren.tm) csr_read_unimp = 1'b1;
                if (csr_write) csr_write_ro = 1'b1;
            end
            CSR_MVENDORID: csr_rdata = mvendorid_read(mvendorid);
            CSR_MARCHID:   csr_rdata = marchid_read(marchid);
            CSR_MIMPID:    csr_rdata = mimpid_read(mimpid);
            CSR_MHARTID:   csr_rdata = mhartid_read(mhartid);
            default: begin
                csr_rdata = 0;
                csr_read_unimp = 1;
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
            mtvec <= 0;
            mcounteren <= 0;
            mcountinhibit <= 0;
            mscratch <= 0;
            stvec <= 0;
            scounteren <= 0;
            sscratch <= 0;
            satp <= 0;
        end
        else begin
            case (csr_address)
                CSR_MEDELEG:    if (csr_write) medeleg <= medeleg_write(csr_wdata_final);
                CSR_MIDELEG:    if (csr_write) mideleg <= mideleg_write(csr_wdata_final);
                CSR_MTVEC:      if (csr_write) mtvec <= mtvec_write(csr_wdata_final);
                CSR_MCOUNTEREN: if (csr_write) mcounteren <= mcounteren_write(csr_wdata_final);
                CSR_MCOUNTINHIBIT: if (csr_write) mcountinhibit <= mcountinhibit_write(csr_wdata_final);
                CSR_MSCRATCH:      if (csr_write) mscratch <= mscratch_write(csr_wdata_final);
                CSR_STVEC:      if (csr_write) stvec <= stvec_write(csr_wdata_final);
                CSR_SCOUNTEREN: if (csr_write) scounteren <= scounteren_write(csr_wdata_final);
                CSR_SSCRATCH:   if (csr_write) sscratch <= sscratch_write(csr_wdata_final);
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
            mie <= 0;
            mip <= 0;
            mcycle <= 0;
            minstret <= 0;
            sepc <= 0;
            scause <= 0;
            stval <= 0;
        end
        else begin

            if (csr_write && csr_address == CSR_MSTATUS) begin
                mstatus <= mstatus_write(csr_wdata_final);
            end
            else if (csr_write && csr_address == CSR_SSTATUS) begin
                mstatus <= sstatus_write(mstatus, csr_wdata_final);
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
            else if (trap_enter && trap_priv == PRIV_M) mepc.mepc <= trap_pc;

            if (csr_write && csr_address == CSR_MCAUSE) mcause <= mcause_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_M) begin
                mcause.exception_code <= {58'b0, trap_cause};
                mcause.is_interrupt <= trap_interrupt;
            end

            if (csr_write && csr_address == CSR_MTVAL) mtval <= mtval_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_M) mtval.mtval <= trap_tval;

            if (csr_write && csr_address == CSR_MIE) mie <= mie_write(csr_wdata_final);

            if (csr_write && csr_address == CSR_MIP) mip <= mip_write(csr_wdata_final);
            else begin
                mip.msip <= cpu_msip;
                mip.mtip <= cpu_mtip;
                mip.meip <= cpu_meip;
            end

            if (csr_write && csr_address == CSR_MCYCLE) mcycle.mcycle <= mcycle_write(csr_wdata_final);
            else mcycle.mcycle <= mcycle.mcycle + 1'b1;

            if (csr_write && csr_address == CSR_MINSTRET) minstret <= minstret_write(csr_wdata_final);
            else if (instret) minstret.minstret <= minstret.minstret + 1'b1;

            if (csr_write && csr_address == CSR_SEPC) sepc <= sepc_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_S) sepc.sepc <= trap_pc;

            if (csr_write && csr_address == CSR_SCAUSE) scause <= scause_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_S) begin
                scause.exception_code <= {58'b0, trap_cause};
                scause.is_interrupt <= trap_interrupt;
            end

            if (csr_write && csr_address == CSR_STVAL) stval <= stval_write(csr_wdata_final);
            else if (trap_enter && trap_priv == PRIV_S) stval.stval <= trap_tval;

            if (csr_write && csr_address == CSR_SIE) begin
                if (csr_write && mideleg.ssip) mie.ssip <= csr_wdata_final[1];
                if (csr_write && mideleg.stip) mie.stip <= csr_wdata_final[5];
                if (csr_write && mideleg.seip) mie.seip <= csr_wdata_final[9];
            end

            // Superviosr mode can only write to mip.ssip
            if (csr_write && csr_address == CSR_SIP) begin
                if (csr_write && mideleg.ssip) mip.ssip <= csr_wdata_final[1];
            end
        end
    end

    // --------------------------------
    // Special handling for seip
    // --------------------------------

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) cpu_seip_int <= 1'b0;
        else cpu_seip_int <= cpu_seip;
    end

    assign seip_pending = mip.seip | cpu_seip_int; // mip.seip stores only the software-writable bit.


    // --------------------------------
    // Check illegal instruction
    // --------------------------------
    assign csr_illegal_instruction = (csr_read | csr_write) & (csr_wrong_priv | csr_read_unimp | csr_write_ro);

    // --------------------------------
    // CSR Information to other Logic
    // --------------------------------
    assign o_medeleg = medeleg_read(medeleg);
    assign o_mideleg = mideleg_read(mideleg);
    assign o_mtvec   = mtvec_read(mtvec);
    assign o_stvec   = stvec_read(stvec);
    assign o_mepc    = mepc_read(mepc);
    assign o_sepc    = sepc_read(sepc);
    assign o_mstatus_tsr = mstatus.tsr;
    assign o_mstatus_tw = mstatus.tw;
    assign o_mstatus_tvm = mstatus.tvm;
    assign o_mstatus_mie = mstatus.mie;
    assign o_mstatus_sie = mstatus.sie;
    assign o_mie = mie;


    assign csr_ssip = mip.ssip;
    assign csr_stip = mip.stip;
    assign csr_seip = seip_pending;

endmodule
