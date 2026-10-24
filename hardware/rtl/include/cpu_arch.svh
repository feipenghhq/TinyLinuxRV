/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */


`ifndef CPU_ARCH_H
`define CPU_ARCH_H

// RISC-V
`define XLEN            64

// Register Number
`define REG_NUM         32
`define RF_RANGE        $clog2(`REG_NUM)-1:0

// PC reset number
`define PC_RESET_VEC    64'h80000000

`endif
