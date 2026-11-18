/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"
`include "csr.svh"

module WB (
    input                clk,
    input                rst_n,
    // pipeline control
    input                in_MEM_valid,
    input MEM2WB_PIPE_t  in_MEM_data,
    output               in_MEM_ready,
    output               MEM_flush,
    // trap
    output               trap,
    output [`XLEN-1:0]   trap_tgt_pc,
    // csr to id
    output               wb_csr,
    output               mstatus_tw,
    output               mstatus_tvm,
    output [1:0]         current_priv,
    // Register write back
    output               wb_rd_write,
    output [4:0]         wb_rd_addr,
    output [`XLEN-1:0]   wb_rd_wdata,
    // interrupt input
    input                cpu_msip,
    input                cpu_mtip,
    input                cpu_meip,
    input                cpu_seip,
    // interrupt status in pipeline
    input                ex_int_valid,
    input                mem_int_valid,
    // interrupt req to ID stage
    output logic         wb_int_req,
    output logic [4:0]   wb_int_cause,
    output logic         wb_csr_drain
);

    logic             trap_enter;
    logic             trap_exit;
    logic [`XLEN-1:0] trap_enter_vec;
    logic [`XLEN-1:0] trap_exit_vec;
    logic [4:0]       trap_cause;
    logic             trap_interrupt;
    logic [`XLEN-1:0] trap_tval;
    logic [`XLEN-1:0] trap_pc;
    logic [1:0]       trap_priv;

    logic             sret_final;
    logic             mret_final;

    logic [`XLEN-1:0] csr_rdata;
    logic [`XLEN-1:0] medeleg;
    logic [`XLEN-1:0] mideleg;
    logic [`XLEN-1:0] mtvec;
    logic [`XLEN-1:0] stvec;
    logic [`XLEN-1:0] mepc;
    logic [`XLEN-1:0] sepc;
    logic             mstatus_tsr;
    logic             mstatus_mie;
    logic             mstatus_sie;
    mie_t             mie;
    logic             csr_ssip;
    logic             csr_stip;
    logic             csr_seip;

    logic             mret_wrong_priv;
    logic             sret_wrong_priv;
    logic             csr_illegal_instruction;
    logic             wb_exception_valid;
    logic [4:0]       wb_exception_cause;
    logic             final_exception_valid;
    logic [4:0]       final_exception_cause;

    logic             interrupt_valid;
    logic [4:0]       interrupt_cause;
    logic             allow_m_interrupt;
    logic             allow_s_interrupt;
    logic [11:0]      interrupt_pending;
    logic [11:0]      interrupt_enabled;
    logic [11:0]      interrupt_eligible;

    // ----------------------------------------------------
    // Pipeline control
    // ----------------------------------------------------

    assign in_MEM_ready = 1'b1; // WB never stall

    assign MEM_flush = trap;
    assign wb_csr = in_MEM_data.csr_read | in_MEM_data.csr_write;

    // ----------------------------------------------------
    // Trap control
    // ----------------------------------------------------

    assign trap = trap_enter | trap_exit;
    assign trap_tgt_pc = trap_enter ? trap_enter_vec : trap_exit_vec;

    // ----------------------------------------------------
    // Register Write back
    // ----------------------------------------------------

    assign wb_rd_write = in_MEM_data.rd_write & ~final_exception_valid; // cancel register write if there are exception
    assign wb_rd_addr  = in_MEM_data.rd_addr;
    assign wb_rd_wdata = in_MEM_data.csr_read ? csr_rdata : in_MEM_data.rd_wdata;

    // ----------------------------------------------------
    // Illegal instruction check from WB stage
    // ----------------------------------------------------

    // MRET: S-mode and U-mode can't execute mret, cpu should raise illegal instruction exception in such case
    assign mret_wrong_priv = in_MEM_data.mret & (current_priv == PRIV_S || current_priv == PRIV_U);
    assign mret_final = in_MEM_data.mret & ~mret_wrong_priv;

    // SRET: 1) U-mode can't execute mret, cpu should raise illegal instruction exception in such case
    //       2) When mstatus.TSR=1, attempts to execute SRET while executing in S-mode will raise an illegal instruction exception.
    //          When mstatus.TSR=0, this operation is permitted in S-mode.
    assign sret_wrong_priv = in_MEM_data.sret & (current_priv == PRIV_U || (current_priv == PRIV_S && mstatus_tsr));
    assign sret_final = in_MEM_data.sret & ~sret_wrong_priv;

    // WB generate exception
    assign wb_exception_valid = csr_illegal_instruction | mret_wrong_priv | sret_wrong_priv;
    assign wb_exception_cause = ILLEGAL_INSTRUCTION;

    // Final exception
    assign final_exception_valid = wb_exception_valid | in_MEM_data.exception_valid;
    assign final_exception_cause = wb_exception_valid ? wb_exception_cause : in_MEM_data.cause;

    // ----------------------------------------------------
    // Interrupt request
    // ----------------------------------------------------

    // We log all the interrupt to MIP CSR and check the interrupt eligibility here.
    // If the interrupt is eligible to be taken then it is send to ID stage to be accepted.
    // If there is no CSR instruction
    // After that, it stars to

    // If the current privilege < X privilege, xIE does not need to be checked.
    // If the target privilege  = X privilege, xIE need to be checked.
    assign allow_m_interrupt = (current_priv < PRIV_M) | ((current_priv == PRIV_M) & mstatus_mie);
    assign allow_s_interrupt = (current_priv < PRIV_S) | ((current_priv == PRIV_S) & mstatus_sie);

    always_comb begin
        interrupt_pending = '0;
        interrupt_pending[3]  = cpu_msip;
        interrupt_pending[7]  = cpu_mtip;
        interrupt_pending[11] = cpu_meip;
        interrupt_pending[1]  = csr_ssip;
        interrupt_pending[5]  = csr_stip;
        interrupt_pending[9]  = csr_seip;

        interrupt_enabled = '0;
        interrupt_enabled[3]  = mie.msip;
        interrupt_enabled[7]  = mie.mtip;
        interrupt_enabled[11] = mie.meip;
        interrupt_enabled[1]  = mie.ssip;
        interrupt_enabled[5]  = mie.stip;
        interrupt_enabled[9]  = mie.seip;
    end

    // Each bit selects its target-mode permission using mideleg.
    assign interrupt_eligible = interrupt_pending & interrupt_enabled &
                                ((~mideleg[11:0] & {12{allow_m_interrupt}}) |   // M-mode only handle interrupt not delegated to S mode
                                 ( mideleg[11:0] & {12{allow_s_interrupt}}));   // S-mode only handle interrupt delegated to S mode

    assign interrupt_valid = |interrupt_eligible;

    // Priority matches the emulator: MEIP > MSIP > MTIP > SEIP > SSIP > STIP.
    always_comb begin
        interrupt_cause = '0;
        if      (interrupt_eligible[11]) interrupt_cause = 5'd11;
        else if (interrupt_eligible[3])  interrupt_cause = 5'd3;
        else if (interrupt_eligible[7])  interrupt_cause = 5'd7;
        else if (interrupt_eligible[9])  interrupt_cause = 5'd9;
        else if (interrupt_eligible[1])  interrupt_cause = 5'd1;
        else if (interrupt_eligible[5])  interrupt_cause = 5'd5;
    end

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            wb_int_req <= 1'b0;
            wb_int_cause <= 0;
        end
        else begin
            wb_int_req    <= interrupt_valid & ~ex_int_valid & ~mem_int_valid & ~in_MEM_data.interrupt_valid;
            wb_int_cause  <= interrupt_cause;
        end
    end

    // Additional wait cycle for pipeline drain to accommodate the registered interrupt request
    // Because we register the interrupt request, if the last drained instruction is a CSR instruction that
    // changed the CSR eligibility, interrupt might be disabled. We then need to cancel this interrupt.
    // In this case, we need to wait for an additional cycle for the wb_int_req to be update.
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            wb_csr_drain <= 1'b0;
        end
        else begin
            wb_csr_drain <= in_MEM_data.csr_read | in_MEM_data.csr_write;
        end
    end

    // ----------------------------------------------------
    // Trap and CSR module
    // ----------------------------------------------------

    trap_ctrl u_trap_ctrl (
        .clk              (clk),
        .rst_n            (rst_n),
        .ecall            (in_MEM_data.ecall),
        .ebreak           (in_MEM_data.ebreak),
        .exception_valid  (final_exception_valid),
        .exception_cause  (final_exception_cause),
        .pc               (in_MEM_data.pc),
        .instruction      (in_MEM_data.instruction),
        .address          (in_MEM_data.address),
        .interrupt_valid  (in_MEM_data.interrupt_valid),
        .interrupt_cause  (in_MEM_data.cause),
        .sret             (sret_final),
        .mret             (mret_final),
        .medeleg          (medeleg),
        .mideleg          (mideleg),
        .mtvec            (mtvec),
        .mepc             (mepc),
        .sepc             (sepc),
        .stvec            (stvec),
        .current_priv     (current_priv),
        .trap_enter       (trap_enter),
        .trap_cause       (trap_cause),
        .trap_interrupt   (trap_interrupt),
        .trap_tval        (trap_tval),
        .trap_pc          (trap_pc),
        .trap_priv        (trap_priv),
        .trap_enter_vec   (trap_enter_vec),
        .trap_exit        (trap_exit),
        .trap_exit_vec    (trap_exit_vec)
    );

    csr u_csr (
        .clk              (clk),
        .rst_n            (rst_n),
        .trap_enter       (trap_enter),
        .trap_cause       (trap_cause),
        .trap_interrupt   (trap_interrupt),
        .trap_tval        (trap_tval),
        .trap_pc          (trap_pc),
        .trap_priv        (trap_priv),
        .sret             (sret_final),
        .mret             (mret_final),
        .o_medeleg        (medeleg),
        .o_mideleg        (mideleg),
        .o_mtvec          (mtvec),
        .o_stvec          (stvec),
        .o_mepc           (mepc),
        .o_sepc           (sepc),
        .o_mstatus_tsr    (mstatus_tsr),
        .o_mstatus_tw     (mstatus_tw),
        .o_mstatus_tvm    (mstatus_tvm),
        .o_mstatus_mie    (mstatus_mie),
        .o_mstatus_sie    (mstatus_sie),
        .o_mie            (mie),
        .current_priv     (current_priv),
        .cpu_msip         (cpu_msip),
        .cpu_mtip         (cpu_mtip),
        .cpu_meip         (cpu_meip),
        .cpu_seip         (cpu_seip),
        .csr_ssip         (csr_ssip),
        .csr_stip         (csr_stip),
        .csr_seip         (csr_seip),
        .instret          (in_MEM_valid),
        .mtime            (0),
        .csr_read         (in_MEM_data.csr_read),
        .csr_write        (in_MEM_data.csr_write),
        .csr_address      (in_MEM_data.csr_address),
        .csr_opcode       (in_MEM_data.csr_write_opcode),
        .csr_wdata        (in_MEM_data.csr_wdata),
        .csr_rdata        (csr_rdata),
        .csr_illegal_instruction(csr_illegal_instruction)
    );

endmodule
