/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

module fwft_fifo #(
    parameter WIDTH = 32,
    parameter DEPTH = 1
) (
    input              clk,
    input              rst_n,
    input              flush,
    input              push_valid,
    output             push_ready,
    input  [WIDTH-1:0] push_data,
    output             pop_valid,
    input              pop_ready,
    output [WIDTH-1:0] pop_data
);

    parameter AW = (DEPTH == 1) ? 1 : $clog2(DEPTH);
    parameter CW = $clog2(DEPTH+1);

    logic [WIDTH-1:0] mem[DEPTH-1:0];
    logic [AW-1:0] rd_ptr, wr_ptr;
    logic [CW-1:0] count;
    logic write, read;
    logic full, empty;

    // ------------------------------------------
    // read, write pointer and count update
    // ------------------------------------------
    always_ff @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            rd_ptr <= 'b0;
            wr_ptr <= 'b0;
            count  <= 'b0;
        end
        else if (flush) begin
            rd_ptr <= 'b0;
            wr_ptr <= 'b0;
            count  <= 'b0;
        end
        else begin

            /* verilator lint_off WIDTHEXPAND */
            if (write) begin
                wr_ptr <= (wr_ptr == (DEPTH - 1)) ? 0 : wr_ptr + 1'b1;
            end
            if (read) begin
                rd_ptr <= (rd_ptr == (DEPTH - 1)) ? 0 : rd_ptr + 1'b1;
            end
            /* verilator lint_on WIDTHEXPAND */

            case ({
                write, read
            })
                2'b00: count <= count;
                2'b01: count <= count - 1'b1;
                2'b10: count <= count + 1'b1;
                2'b11: count <= count;
            endcase
        end
    end

    // ------------------------------------------
    // full, empty condition
    // ------------------------------------------

    /* verilator lint_off WIDTHEXPAND */
    assign full  = count == DEPTH;
    assign empty = count == 0;
    /* verilator lint_on WIDTHEXPAND */

    // ------------------------------------------
    // read and write logic
    // ------------------------------------------

    // when FIFO is NOT empty, write to the FIFO memory
    // when FIFO is empty, if the downstream does not take the data, write to the FIFO memory
    assign write = push_valid & (~empty | ~pop_ready);

    always_ff @(posedge clk, negedge rst_n) begin
        if (write) mem[wr_ptr] <= push_data;
    end

    // when FIFO is NOT empty, read the FIFO when pop handshakes
    assign read  = ~empty & pop_valid & pop_ready;

    // ------------------------------------------
    // push/pop interface
    // ------------------------------------------

    // upstream can write to FIFO when it is not FULL
    assign push_ready = ~full;

    // indicate downstream FIFO has content
    assign pop_valid  = ~empty | push_valid;

    // forward the data from input directly to output when FIFO is empty (FWFT)
    // else the data come from FIFO
    assign pop_data = empty ? push_data : mem[rd_ptr];

endmodule
