/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

/**

The simplest algorithm for divide operation for hardware is to mimic the Long division.
This divider design used the long division algorithm.

The divider use UNSIGNED division. We divide the number using their absolute value then post process the result
to get correct value.
- For unsigned division, it is simple, just use the original input
- For signed division, we firstly convert the negative value to positive value and then adjust the result.

RISC-V signed division cut towards 0, so
- quotient: if either one of the operand is negative, then output is negative
- reminder: if dividend is negative, then output is negative

Let sa and sb be the signs (+1 or -1) of dividend a and divisor b.
Unsigned division of their magnitudes gives:
  |a| = |b| * Q + R
Multiplying both sides by sa:
  a = sa * |b| * Q + sa * R
    = b * (sa * sb * Q) + sa * R
Comparing with a = b * q + r:
  q = sa * sb * Q  -> negate Q when the operand signs differ.
  r = sa * R       -> negate R when the dividend is negative.

*/

`include "cpu.svh"

module divider (
    input              clk,
    input              rst_n,
    input              req,
    input              flush,
    input  [1:0]       opcode,
    input              is_32bit,
    input  [`XLEN-1:0] a,      // rs1
    input  [`XLEN-1:0] b,      // rs2
    output [`XLEN-1:0] o,      // rd
    output             stall
);

    // ---------------------------------
    // Signal Declaration
    // ---------------------------------

    logic [1:0]         stage_valid;
    logic               running;
    logic               new_req;
    logic               done;
    logic  [`XLEN-1:0]  a_post;
    logic  [`XLEN-1:0]  b_post;

    logic [`XLEN-1:0]   dividend;
    logic [`XLEN-1:0]   divisor;
    logic [`XLEN-1:0]   quotient;
    logic               is_rem; // 1 - REM/REMU, 0 - DIV/DIVU
    logic               quotient_negate;
    logic               remainder_negate;
    logic [$clog2(`XLEN+1):0] iterations;

    logic               signed_div;
    logic               dividend_sign_bit;
    logic               divisor_sign_bit;
    logic [`XLEN-1:0]   dividend_unsign;
    logic [`XLEN-1:0]   divisor_unsign;
    logic               not_divide_by_zero;

    logic [2*`XLEN-1:0] extended_dividend;
    logic [2*`XLEN-1:0] extended_divisor;
    logic [2*`XLEN-1:0] sub_result;
    logic               sub_result_sign_bit;
    logic [`XLEN-1:0]   remainder;
    logic [`XLEN-1:0]   quotient_post;
    logic [`XLEN-1:0]   remainder_post;

    logic [`XLEN-1:0]   output_raw;

    // ---------------------------------
    // Main logic
    // ---------------------------------

    // Process the input data and store it in the register

    assign new_req = req & ~running;

    // ---------------------------------
    // Pre processing and store input data
    // ---------------------------------

    // processing for 32 bit vs 64 bit
    assign a_post = is_32bit ? (signed_div ? {{32{a[31]}}, a[31:0]}: {32'b0, a[31:0]} ): a;
    assign b_post = is_32bit ? (signed_div ? {{32{b[31]}}, b[31:0]}: {32'b0, b[31:0]} ): b;

    // we use unsigned divider so for DIV/REM we need to convert NEGATIVE number to POSITIVE number
    assign dividend_sign_bit = a_post[`XLEN-1];
    assign divisor_sign_bit  = b_post[`XLEN-1];

    assign dividend_unsign = ~a_post + 1'b1;
    assign divisor_unsign  = ~b_post + 1'b1;

    assign signed_div = opcode[0] == 0; // DIV/REM => opcode[0] == 0
    assign not_divide_by_zero = b_post != 0;

    always @(posedge clk) begin
        if (new_req) begin
            // dividend is handled later because it's need other operations
            divisor <= (signed_div && divisor_sign_bit) ? divisor_unsign : b_post;
            quotient_negate  <= signed_div & (dividend_sign_bit ^ divisor_sign_bit) & not_divide_by_zero;
            remainder_negate <= signed_div & dividend_sign_bit;
            is_rem <= opcode[1];
        end
    end

    // ---------------------------------
    // Calculation process
    // ---------------------------------

    assign done = iterations == `XLEN;

    // flow control
    always @(posedge clk) begin
        if (!rst_n)       running <= 1'b0;
        else if (flush)   running <= 1'b0;
        else if (new_req) running <= 1'b1;
        else if (done)    running <= 1'b0;
    end

    // Zero-extend the dividend and shift the divisor left by XLEN-1 bits
    // to test the highest quotient bit first. Each iteration shifts the
    // divisor right by one bit to test the next quotient bit.
    // The extra leading zero allows the subtraction's MSB to indicate a negative result.

    assign extended_dividend = {{`XLEN{1'b0}}, dividend};
    assign extended_divisor  = {1'b0, divisor, {(`XLEN-1){1'b0}}};

    // Try subtracting divisor << (XLEN-1-iterations) from the current remainder.
    // If the result is nonnegative, keep it and set the current quotient bit to 1.
    // Otherwise, keep the remainder unchanged and set the quotient bit to 0.
    assign sub_result = extended_dividend - (extended_divisor >> iterations);

    assign sub_result_sign_bit = sub_result[2*`XLEN-1];

    always @(posedge clk) begin
        if (!rst_n) begin
            quotient   <= 0;
            iterations <= 0;
        end
        else if (flush || new_req) begin
            quotient   <= 0;
            iterations <= 0;
        end
        else if (running) begin
            // left shift by 1 position to push the previous bit to the left (This is mimic we move right in long division)
            quotient[`XLEN-1:1] <= quotient[`XLEN-2:0];
            // if subtracting yield a positive number, then this position is a one otherwise it is zero (similar to long division)
            quotient[0] <= (sub_result_sign_bit == 0) ? 1'b1 : 1'b0;
            iterations  <= iterations + 1'b1;
        end
    end

    // handle dividend seperately
    always @(posedge clk) begin
        if (new_req) dividend <= (signed_div && dividend_sign_bit) ? dividend_unsign : a_post;
        // if subtracting yield a positive number, then we update the dividend with the remaining data
        else dividend <= (sub_result_sign_bit == 0) ? sub_result[`XLEN-1:0] : dividend;
    end

    assign remainder = dividend;

    // ---------------------------------
    // Post process the result
    // ---------------------------------

    // adjust the sign of the result
    assign quotient_post  = quotient_negate  ? (~quotient + 1'b1)  : quotient;
    assign remainder_post = remainder_negate ? (~remainder + 1'b1) : remainder;

    // assign the correct value to output
    assign output_raw = is_rem ? remainder_post : quotient_post;
    assign o = is_32bit ? {{32{output_raw[31]}}, output_raw[31:0]} : output_raw;

    assign stall = new_req | (running & ~done);

endmodule
