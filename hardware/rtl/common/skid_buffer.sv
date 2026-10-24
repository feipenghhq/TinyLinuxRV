/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

module skid_buffer #(
    parameter WIDTH = 64
) (
    input                    clk,
    input                    rst_n,
    // input side
    input                    in_valid,
    input        [WIDTH-1:0] in_data,
    output                   in_ready,
    // output side
    output logic             out_valid,
    output logic [WIDTH-1:0] out_data,
    input                    out_ready
);

    logic [WIDTH-1:0] buffer;
    logic             buffer_valid;

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            buffer_valid <= 1'b0;
        end
        else begin
            if (in_valid && !out_valid && !out_ready) buffer_valid <= 1'b1;
            else if (out_valid && out_ready) buffer_valid <= 1'b0;
        end
    end

    always_ff @(posedge clk) begin
        if (in_valid && !out_valid && !out_ready) buffer <= in_data;
    end

    assign in_ready = ~buffer_valid;

    assign out_valid = buffer_valid | in_valid;
    assign out_data  = buffer_valid ? buffer : in_data;

endmodule
