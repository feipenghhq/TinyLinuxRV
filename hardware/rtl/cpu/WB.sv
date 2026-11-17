/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */



`include "cpu.svh"

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
    // Register write back
    output               wb_rd_write,
    output [4:0]         wb_rd_addr,
    output [`XLEN-1:0]   wb_rd_wdata
);

    logic             trap_enter;
    logic             trap_exit;
    logic [`XLEN-1:0] trap_enter_vec;
    logic [`XLEN-1:0] trap_exit_vec;
    logic [`XLEN-1:0] trap_cause;
    logic [`XLEN-1:0] trap_tval;
    logic [`XLEN-1:0] trap_pc;
    logic [1:0]       trap_priv;
    logic [1:0]       current_priv;
    logic [`XLEN-1:0] medeleg;
    logic [`XLEN-1:0] mideleg;
    logic [`XLEN-1:0] mtvec;
    logic [`XLEN-1:0] mepc;
    logic             mstatus_tsr;
    logic [`XLEN-1:0] csr_rdata;

    logic             sret_final;
    logic             mret_final;

    logic             mret_wrong_priv;
    logic             sret_wrong_priv;
    logic             csr_illegal_instruction;
    logic             wb_exception_valid;
    logic [4:0]       wb_exception_cause;

    logic             final_exception_valid;
    logic [4:0]       final_exception_cause;

    // ----------------------------------------------------
    // Pipeline control
    // ----------------------------------------------------

    assign in_MEM_ready = 1'b1; // WB never stall

    assign MEM_flush = trap;

    // ----------------------------------------------------
    // Trap control
    // ----------------------------------------------------

    assign trap = trap_enter | trap_exit;
    assign trap_tgt_pc = trap_enter ? trap_enter_vec : trap_exit_vec;

    // ----------------------------------------------------
    // Register Write back
    // ----------------------------------------------------

    assign wb_rd_write = in_MEM_data.rd_write;
    assign wb_rd_addr  = in_MEM_data.rd_addr;
    assign wb_rd_wdata = in_MEM_data.csr_read ? csr_rdata : in_MEM_data.rd_wdata;

    // ----------------------------------------------------
    // Illegal instruction check
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
    assign final_exception_cause = wb_exception_valid ? wb_exception_cause : in_MEM_data.exception_cause;

    // ----------------------------------------------------
    // Trap and CSR module
    // ----------------------------------------------------

    trap_ctrl u_trap_ctrl (
        .clk              (clk),
        .rst_n            (rst_n),
        .exception_valid  (final_exception_valid),
        .exception_cause  (final_exception_cause),
        .pc               (in_MEM_data.pc),
        .instruction      (in_MEM_data.instruction),
        .address          ('0),
        .interrupt_valid  (1'b0),
        .interrupt_cause  (5'b0),
        .sret             (sret_final),
        .mret             (mret_final),
        .medeleg          (medeleg),
        .mideleg          (mideleg),
        .mtvec            (mtvec),
        .mepc             (mepc),
        .stvec            ('0),
        .current_priv     (current_priv),
        .trap_enter       (trap_enter),
        .trap_cause       (trap_cause),
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
        .trap_tval        (trap_tval),
        .trap_pc          (trap_pc),
        .trap_priv        (trap_priv),
        .o_medeleg        (medeleg),
        .o_mideleg        (mideleg),
        .o_mtvec          (mtvec),
        .o_mepc           (mepc),
        .o_mstatus_tsr    (mstatus_tsr),
        .current_priv     (current_priv),
        .sret             (sret_final),
        .mret             (mret_final),
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
