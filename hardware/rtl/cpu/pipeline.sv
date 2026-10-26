/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

// Common pipeline stage

module pipeline #(
    parameter WIDTH = 1
) (
    input                    clk,
    input                    rst_n,
    // pipeline input
    input                    in_valid,
    output                   in_ready,
    input        [WIDTH-1:0] in_data,
    // pipeline output
    output logic             out_valid,
    input                    out_ready,
    output logic [WIDTH-1:0] out_data
);

    assign in_ready = !out_valid | out_ready;

    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            out_valid <= 1'b0;
            out_data  <= 'b0;
        end
        // if downstream is ready to receive data
        else if (in_ready) begin
            // if input is valid then load the input into pipeline
            if (in_valid) begin
                out_valid <= in_valid;
                out_data  <= in_data;
            end
            // if input is not valid then clear the pipeline
            else begin
                out_valid <= 1'b0;
                out_data  <= 'b0;
            end
        end
    end

endmodule
