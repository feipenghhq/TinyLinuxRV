
/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`ifndef _CSR_H_
`define _CSR_H_

// --------------------------
// CSR address
// --------------------------

enum logic [11:0] {
    CSR_SSTATUS       = 12'h100,
    CSR_SIE           = 12'h104,
    CSR_STVEC         = 12'h105,
    CSR_SCOUNTEREN    = 12'h106,
    CSR_SSCRATCH      = 12'h140,
    CSR_SEPC          = 12'h141,
    CSR_SCAUSE        = 12'h142,
    CSR_STVAL         = 12'h143,
    CSR_SIP           = 12'h144,
    CSR_SATP          = 12'h180,

    CSR_MSTATUS       = 12'h300,
    CSR_MISA          = 12'h301,
    CSR_MEDELEG       = 12'h302,
    CSR_MIDELEG       = 12'h303,
    CSR_MIE           = 12'h304,
    CSR_MTVEC         = 12'h305,
    CSR_MCOUNTEREN    = 12'h306,
    CSR_MSCRATCH      = 12'h340,
    CSR_MEPC          = 12'h341,
    CSR_MCAUSE        = 12'h342,
    CSR_MTVAL         = 12'h343,
    CSR_MIP           = 12'h344,
    CSR_MCOUNTINHIBIT = 12'h320,
    CSR_MCYCLE        = 12'hB00,
    CSR_MINSTRET      = 12'hB02,
    CSR_MVENDORID     = 12'hF11,
    CSR_MARCHID       = 12'hF12,
    CSR_MIMPID        = 12'hF13,
    CSR_MHARTID       = 12'hF14,

    CSR_CYCLE         = 12'hC00,
    CSR_TIME          = 12'hC01,
    CSR_INSTRET       = 12'hC02
} csr_addr_enum;


// --------------------------
// mstatus
// --------------------------
typedef struct packed {
    logic sie;
    logic mie;
    logic spie;
    logic mpie;
    logic spp;
    logic [1:0] mpp;
    logic mprv;
    logic sum;
    logic mxr;
    logic tvm;
    logic tw;
    logic tsr;
    logic [1:0] uxl;
    logic [1:0] sxl;
} mstatus_t;

function automatic mstatus_t mstatus_write(
    input mstatus_t csr,
    input [63:0] wdata
);
    mstatus_t data = csr;
    data.sie = wdata[1];
    data.mie = wdata[3];
    data.spie = wdata[5];
    data.mpie = wdata[7];
    data.spp = wdata[8];
    data.mpp = wdata[12:11];
    data.mprv = wdata[17];
    data.sum = wdata[18];
    data.mxr = wdata[19];
    data.tvm = wdata[20];
    data.tw = wdata[21];
    data.tsr = wdata[22];

    return data;
endfunction

function automatic logic [63:0] mstatus_read(
    input mstatus_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.sie;
    data[3] = csr.mie;
    data[5] = csr.spie;
    data[7] = csr.mpie;
    data[8] = csr.spp;
    data[12:11] = csr.mpp;
    data[17] = csr.mprv;
    data[18] = csr.sum;
    data[19] = csr.mxr;
    data[20] = csr.tvm;
    data[21] = csr.tw;
    data[22] = csr.tsr;
    data[33:32] = 2;
    data[35:34] = 2;

    return data;
endfunction

// --------------------------
// misa
// --------------------------
typedef struct packed {
    logic A;
    logic I;
    logic M;
    logic S;
    logic U;
    logic [1:0] MXL;
} misa_t;

function automatic misa_t misa_write(
    input misa_t csr,
    input [63:0] wdata
);
    misa_t data = csr;

    return data;
endfunction

function automatic logic [63:0] misa_read(
    input misa_t csr,
);
    logic [63:0] data = 0;
    data[0] = 1;
    data[8] = 1;
    data[12] = 1;
    data[18] = 1;
    data[20] = 1;
    data[63:62] = 2;

    return data;
endfunction

// --------------------------
// medeleg
// --------------------------
typedef struct packed {
    logic instruction_address_misaligned;
    logic instruction_access_fault;
    logic illegal_instruction;
    logic breakpoint;
    logic load_address_misaligned;
    logic load_access_fault;
    logic store_amo_address_misaligned;
    logic store_amo_access_fault;
    logic ecall_u;
    logic ecall_s;
    logic instruction_page_fault;
    logic load_page_fault;
    logic store_amo_page_fault;
} medeleg_t;

function automatic medeleg_t medeleg_write(
    input medeleg_t csr,
    input [63:0] wdata
);
    medeleg_t data = csr;
    data.instruction_address_misaligned = wdata[0];
    data.instruction_access_fault = wdata[1];
    data.illegal_instruction = wdata[2];
    data.breakpoint = wdata[3];
    data.load_address_misaligned = wdata[4];
    data.load_access_fault = wdata[5];
    data.store_amo_address_misaligned = wdata[6];
    data.store_amo_access_fault = wdata[7];
    data.ecall_u = wdata[8];
    data.ecall_s = wdata[9];
    data.instruction_page_fault = wdata[12];
    data.load_page_fault = wdata[13];
    data.store_amo_page_fault = wdata[15];

    return data;
endfunction

function automatic logic [63:0] medeleg_read(
    input medeleg_t csr,
);
    logic [63:0] data = 0;
    data[0] = csr.instruction_address_misaligned;
    data[1] = csr.instruction_access_fault;
    data[2] = csr.illegal_instruction;
    data[3] = csr.breakpoint;
    data[4] = csr.load_address_misaligned;
    data[5] = csr.load_access_fault;
    data[6] = csr.store_amo_address_misaligned;
    data[7] = csr.store_amo_access_fault;
    data[8] = csr.ecall_u;
    data[9] = csr.ecall_s;
    data[12] = csr.instruction_page_fault;
    data[13] = csr.load_page_fault;
    data[15] = csr.store_amo_page_fault;

    return data;
endfunction

// --------------------------
// mideleg
// --------------------------
typedef struct packed {
    logic ssip;
    logic msip;
    logic stip;
    logic mtip;
    logic seip;
    logic meip;
} mideleg_t;

function automatic mideleg_t mideleg_write(
    input mideleg_t csr,
    input [63:0] wdata
);
    mideleg_t data = csr;
    data.ssip = wdata[1];
    data.msip = wdata[3];
    data.stip = wdata[5];
    data.mtip = wdata[7];
    data.seip = wdata[9];
    data.meip = wdata[11];

    return data;
endfunction

function automatic logic [63:0] mideleg_read(
    input mideleg_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.ssip;
    data[3] = csr.msip;
    data[5] = csr.stip;
    data[7] = csr.mtip;
    data[9] = csr.seip;
    data[11] = csr.meip;

    return data;
endfunction

// --------------------------
// mie
// --------------------------
typedef struct packed {
    logic ssip;
    logic msip;
    logic stip;
    logic mtip;
    logic seip;
    logic meip;
} mie_t;

function automatic mie_t mie_write(
    input mie_t csr,
    input [63:0] wdata
);
    mie_t data = csr;
    data.ssip = wdata[1];
    data.msip = wdata[3];
    data.stip = wdata[5];
    data.mtip = wdata[7];
    data.seip = wdata[9];
    data.meip = wdata[11];

    return data;
endfunction

function automatic logic [63:0] mie_read(
    input mie_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.ssip;
    data[3] = csr.msip;
    data[5] = csr.stip;
    data[7] = csr.mtip;
    data[9] = csr.seip;
    data[11] = csr.meip;

    return data;
endfunction

// --------------------------
// mtvec
// --------------------------
typedef struct packed {
    logic [1:0] mode;
    logic [61:0] base;
} mtvec_t;

function automatic mtvec_t mtvec_write(
    input mtvec_t csr,
    input [63:0] wdata
);
    mtvec_t data = csr;
    data.mode = wdata[1:0];
    data.base = wdata[63:2];

    return data;
endfunction

function automatic logic [63:0] mtvec_read(
    input mtvec_t csr,
);
    logic [63:0] data = 0;
    data[1:0] = csr.mode;
    data[63:2] = csr.base;

    return data;
endfunction

// --------------------------
// mcounteren
// --------------------------
typedef struct packed {
    logic cy;
    logic tm;
    logic ir;
} mcounteren_t;

function automatic mcounteren_t mcounteren_write(
    input mcounteren_t csr,
    input [63:0] wdata
);
    mcounteren_t data = csr;
    data.cy = wdata[0];
    data.tm = wdata[1];
    data.ir = wdata[2];

    return data;
endfunction

function automatic logic [63:0] mcounteren_read(
    input mcounteren_t csr,
);
    logic [63:0] data = 0;
    data[0] = csr.cy;
    data[1] = csr.tm;
    data[2] = csr.ir;

    return data;
endfunction

// --------------------------
// mscratch
// --------------------------
typedef struct packed {
    logic [63:0] mscratch;
} mscratch_t;

function automatic mscratch_t mscratch_write(
    input mscratch_t csr,
    input [63:0] wdata
);
    mscratch_t data = csr;
    data.mscratch = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] mscratch_read(
    input mscratch_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mscratch;

    return data;
endfunction

// --------------------------
// mepc
// --------------------------
typedef struct packed {
    logic [63:0] mepc;
} mepc_t;

function automatic mepc_t mepc_write(
    input mepc_t csr,
    input [63:0] wdata
);
    mepc_t data = csr;
    data.mepc = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] mepc_read(
    input mepc_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mepc;

    return data;
endfunction

// --------------------------
// mcause
// --------------------------
typedef struct packed {
    logic [62:0] exception_code;
    logic is_interrupt;
} mcause_t;

function automatic mcause_t mcause_write(
    input mcause_t csr,
    input [63:0] wdata
);
    mcause_t data = csr;

    return data;
endfunction

function automatic logic [63:0] mcause_read(
    input mcause_t csr,
);
    logic [63:0] data = 0;
    data[62:0] = csr.exception_code;
    data[63] = csr.is_interrupt;

    return data;
endfunction

// --------------------------
// mtval
// --------------------------
typedef struct packed {
    logic [63:0] mtval;
} mtval_t;

function automatic mtval_t mtval_write(
    input mtval_t csr,
    input [63:0] wdata
);
    mtval_t data = csr;
    data.mtval = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] mtval_read(
    input mtval_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mtval;

    return data;
endfunction

// --------------------------
// mip
// --------------------------
typedef struct packed {
    logic ssip;
    logic msip;
    logic stip;
    logic mtip;
    logic seip;
    logic meip;
} mip_t;

function automatic mip_t mip_write(
    input mip_t csr,
    input [63:0] wdata
);
    mip_t data = csr;
    data.ssip = wdata[1];
    data.msip = wdata[3];
    data.stip = wdata[5];
    data.mtip = wdata[7];
    data.seip = wdata[9];
    data.meip = wdata[11];

    return data;
endfunction

function automatic logic [63:0] mip_read(
    input mip_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.ssip;
    data[3] = csr.msip;
    data[5] = csr.stip;
    data[7] = csr.mtip;
    data[9] = csr.seip;
    data[11] = csr.meip;

    return data;
endfunction

// --------------------------
// mcountinhibit
// --------------------------
typedef struct packed {
    logic cy;
    logic ir;
} mcountinhibit_t;

function automatic mcountinhibit_t mcountinhibit_write(
    input mcountinhibit_t csr,
    input [63:0] wdata
);
    mcountinhibit_t data = csr;
    data.cy = wdata[0];
    data.ir = wdata[2];

    return data;
endfunction

function automatic logic [63:0] mcountinhibit_read(
    input mcountinhibit_t csr,
);
    logic [63:0] data = 0;
    data[0] = csr.cy;
    data[2] = csr.ir;

    return data;
endfunction

// --------------------------
// mcycle
// --------------------------
typedef struct packed {
    logic [63:0] mcycle;
} mcycle_t;

function automatic mcycle_t mcycle_write(
    input mcycle_t csr,
    input [63:0] wdata
);
    mcycle_t data = csr;
    data.mcycle = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] mcycle_read(
    input mcycle_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mcycle;

    return data;
endfunction

// --------------------------
// minstret
// --------------------------
typedef struct packed {
    logic [63:0] minstret;
} minstret_t;

function automatic minstret_t minstret_write(
    input minstret_t csr,
    input [63:0] wdata
);
    minstret_t data = csr;
    data.minstret = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] minstret_read(
    input minstret_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.minstret;

    return data;
endfunction

// --------------------------
// mvendorid
// --------------------------
typedef struct packed {
    logic [63:0] mvendorid;
} mvendorid_t;

function automatic mvendorid_t mvendorid_write(
    input mvendorid_t csr,
    input [63:0] wdata
);
    mvendorid_t data = csr;

    return data;
endfunction

function automatic logic [63:0] mvendorid_read(
    input mvendorid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0;

    return data;
endfunction

// --------------------------
// marchid
// --------------------------
typedef struct packed {
    logic [63:0] marchid;
} marchid_t;

function automatic marchid_t marchid_write(
    input marchid_t csr,
    input [63:0] wdata
);
    marchid_t data = csr;

    return data;
endfunction

function automatic logic [63:0] marchid_read(
    input marchid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0;

    return data;
endfunction

// --------------------------
// mimpid
// --------------------------
typedef struct packed {
    logic [63:0] mimpid;
} mimpid_t;

function automatic mimpid_t mimpid_write(
    input mimpid_t csr,
    input [63:0] wdata
);
    mimpid_t data = csr;

    return data;
endfunction

function automatic logic [63:0] mimpid_read(
    input mimpid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0;

    return data;
endfunction

// --------------------------
// mhartid
// --------------------------
typedef struct packed {
    logic [63:0] mhartid;
} mhartid_t;

function automatic mhartid_t mhartid_write(
    input mhartid_t csr,
    input [63:0] wdata
);
    mhartid_t data = csr;

    return data;
endfunction

function automatic logic [63:0] mhartid_read(
    input mhartid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0;

    return data;
endfunction

`endif
