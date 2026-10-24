/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`ifndef CPU_STRUCT_H
`define CPU_STRUCT_H

`include "cpu_arch.svh"

// ---------------------------------
// AHB-Lite Bus
// ---------------------------------

typedef struct packed {
    logic [`XLEN-1:0] haddr;
    logic [1:0]       htrans;
    logic             hwrite;
    logic [2:0]       hsize;
    logic             hwdata;
    logic             hsel;
} AHBLiteReq_t;

typedef struct packed {
    logic [`XLEN-1:0] hrdata;
    logic             hready;
    logic             hresp;
} AHBLiteRsp_t;

// ---------------------------------
// Pipeline Stages
// ---------------------------------

// IF/ID
typedef struct packed {
    logic [`XLEN-1:0] pc;
} IF2ID_PIPE_t;

`endif
