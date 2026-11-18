/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`include "cpu.svh"

module trap_ctrl
(
    input               clk,
    input               rst_n,
    // instruction
    input               ecall,
    input               ebreak,
    // exception
    input               exception_valid,
    input [4:0]         exception_cause,
    input [`XLEN-1:0]   pc,
    input [31:0]        instruction,
    input [`XLEN-1:0]   address,
    // interrupt
    input               interrupt_valid,
    input [4:0]         interrupt_cause,
    // Returning from trap
    input               sret,
    input               mret,

    // CSR
    input [`XLEN-1:0]   medeleg,
    input [`XLEN-1:0]   mideleg,
    input [`XLEN-1:0]   mtvec,
    input [`XLEN-1:0]   mepc,
    input [`XLEN-1:0]   stvec,
    input [1:0]         current_priv,

    // Entering trap
    output logic              trap_enter,
    output logic [`XLEN-1:0]  trap_enter_vec,
    output logic [4:0]        trap_cause,
    output logic [`XLEN-1:0]  trap_tval,
    output logic [`XLEN-1:0]  trap_pc,
    output logic [1:0]        trap_priv,

    // Exit trap
    output logic              trap_exit,
    output logic [`XLEN-1:0]  trap_exit_vec
);

    logic [1:0] target_priv;
    logic       target_priv_is_M;
    logic       handle_interrupt;

    logic [`XLEN-3:0] trap_vec_base;
    logic [1:0]       trap_vec_mode;

    logic [4:0] ecall_cause;

    // ----------------------------------------------------
    // Handling Interrupt/Exception Entering
    // ----------------------------------------------------

    // 1. Check privilege mode for interrupt and exceptions.
    always_comb begin

        // Additional check on whether the interrupt will be taken based on privilege mode and mideleg
        // From SPEC: Delegated interrupts result in the interrupt being masked at the delegator privilege level.
        // This means: For interrupts raised in M mode, if it is delegated to S mode, it will NOT be taken.
        handle_interrupt = interrupt_valid;
        if (current_priv == PRIV_M) begin
            if (mideleg[{1'b0, interrupt_cause}]) begin
                handle_interrupt = 1'b0;
            end
        end

        // Check if the exception and interrupts are delegated.
        target_priv = PRIV_M; // default is Machine mode
        target_priv_is_M = 1'b1;
        // For exception raised in S/U mode, if the corresponding bit in medeleg is set, then it is delegated to S-mode
        // For interrupts raised in S/U mode, if the corresponding bit in mideleg is set, then it is delegated to S-mode
        if (current_priv < PRIV_M) begin
            if (exception_valid  && medeleg[{1'b0, exception_cause}] ||
                handle_interrupt && mideleg[{1'b0, interrupt_cause}]) begin
                target_priv = PRIV_S;
                target_priv_is_M = 1'b0;
            end
        end

    end

    // 2. Handling Exception and Trap
    assign trap_enter = exception_valid | handle_interrupt | ecall | ebreak;

    // Update: trap cause (mcause/scause)
    /* verilator lint_off WIDTHEXPAND */
    assign ecall_cause = current_priv + 8;
    /* verilator lint_on WIDTHEXPAND */
    assign trap_cause = ecall  ? ecall_cause :
                        ebreak ? BREAKPOINT:
                        exception_valid ? exception_cause : interrupt_cause;

    // Update: mepc/sepc
    // since we only support only IALIGN=32, the two low bits (mepc[1:0]) are always zero.
    assign trap_pc = {pc[`XLEN-1:2], 2'b0};

    // Update: mtval/stval
    assign trap_tval = exception_cause == ILLEGAL_INSTRUCTION ? {{(`XLEN-32){1'b0}}, instruction} : address;

    // Calculate trap vector (jump pc)
    assign trap_vec_base = target_priv_is_M ? mtvec[`XLEN-1:2] : stvec[`XLEN-1:2];
    assign trap_vec_mode = target_priv_is_M ? mtvec[1:0]       : stvec[1:0];
    always_comb begin
        trap_enter_vec = {trap_vec_base, 2'b0};
        if (handle_interrupt && trap_vec_mode == 1)
            /* verilator lint_off WIDTHEXPAND */
            trap_enter_vec = {trap_vec_base, 2'b0} + (interrupt_cause * 4);
            /* verilator lint_on WIDTHEXPAND */
    end

    // Target Privilege Mode
    assign trap_priv = target_priv;

    // ----------------------------------------------------
    // Handling Interrupt/Exception Return
    // ----------------------------------------------------

    assign trap_exit     = mret | sret;
    assign trap_exit_vec = mepc;

endmodule
