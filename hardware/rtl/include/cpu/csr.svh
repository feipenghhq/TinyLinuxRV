
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
    input [63:0] wdata
);
    mstatus_t data = 0;
    // CPU write field
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
    data[33:32] = 2; // Fixed: mstatus.uxl
    data[35:34] = 2; // Fixed: mstatus.sxl

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

function automatic logic [63:0] misa_read(
    input misa_t csr,
);
    logic [63:0] data = 0;
    data[0] = 1; // Fixed: misa.A
    data[8] = 1; // Fixed: misa.I
    data[12] = 1; // Fixed: misa.M
    data[18] = 1; // Fixed: misa.S
    data[20] = 1; // Fixed: misa.U
    data[63:62] = 2; // Fixed: misa.MXL

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
    input [63:0] wdata
);
    medeleg_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mideleg_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mie_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mtvec_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mcounteren_t data = 0;
    // CPU write field
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
// mcountinhibit
// --------------------------
typedef struct packed {
    logic cy;
    logic ir;
} mcountinhibit_t;

function automatic mcountinhibit_t mcountinhibit_write(
    input [63:0] wdata
);
    mcountinhibit_t data = 0;
    // CPU write field
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
// mscratch
// --------------------------
typedef struct packed {
    logic [63:0] mscratch;
} mscratch_t;

function automatic mscratch_t mscratch_write(
    input [63:0] wdata
);
    mscratch_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mepc_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mcause_t data = 0;
    // CPU write field
    data.exception_code = wdata[62:0];
    data.is_interrupt = wdata[63];

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
    input [63:0] wdata
);
    mtval_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    mip_t data = 0;
    // CPU write field
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
// mcycle
// --------------------------
typedef struct packed {
    logic [63:0] mcycle;
} mcycle_t;

function automatic mcycle_t mcycle_write(
    input [63:0] wdata
);
    mcycle_t data = 0;
    // CPU write field
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
    input [63:0] wdata
);
    minstret_t data = 0;
    // CPU write field
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
// mtime
// --------------------------
typedef struct packed {
    logic [63:0] mtime;
} mtime_t;

function automatic mtime_t mtime_write(
    input [63:0] wdata
);
    mtime_t data = 0;
    // CPU write field
    data.mtime = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] mtime_read(
    input mtime_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mtime;

    return data;
endfunction

// --------------------------
// mvendorid
// --------------------------
typedef struct packed {
    logic [63:0] mvendorid;
} mvendorid_t;

function automatic logic [63:0] mvendorid_read(
    input mvendorid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0; // Fixed: mvendorid.mvendorid

    return data;
endfunction

// --------------------------
// marchid
// --------------------------
typedef struct packed {
    logic [63:0] marchid;
} marchid_t;

function automatic logic [63:0] marchid_read(
    input marchid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0; // Fixed: marchid.marchid

    return data;
endfunction

// --------------------------
// mimpid
// --------------------------
typedef struct packed {
    logic [63:0] mimpid;
} mimpid_t;

function automatic logic [63:0] mimpid_read(
    input mimpid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0; // Fixed: mimpid.mimpid

    return data;
endfunction

// --------------------------
// mhartid
// --------------------------
typedef struct packed {
    logic [63:0] mhartid;
} mhartid_t;

function automatic logic [63:0] mhartid_read(
    input mhartid_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = 0; // Fixed: mhartid.mhartid

    return data;
endfunction

// --------------------------
// sstatus
// --------------------------
typedef struct packed {
    logic sie;
    logic spie;
    logic spp;
    logic sum;
    logic mxr;
    logic [1:0] uxl;
} sstatus_t;

function automatic mstatus_t sstatus_write(
    input [63:0] wdata
);
    mstatus_t data = 0;
    // CPU write field
    data.sie = wdata[1];
    data.spie = wdata[5];
    data.spp = wdata[8];
    data.sum = wdata[18];
    data.mxr = wdata[19];

    return data;
endfunction

function automatic logic [63:0] sstatus_read(
    input mstatus_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.sie;
    data[5] = csr.spie;
    data[8] = csr.spp;
    data[18] = csr.sum;
    data[19] = csr.mxr;
    data[33:32] = 2; // Fixed: sstatus.uxl

    return data;
endfunction

// --------------------------
// sie
// --------------------------
typedef struct packed {
    logic ssip;
    logic stip;
    logic seip;
} sie_t;

function automatic mie_t sie_write(
    input [63:0] wdata
);
    mie_t data = 0;
    // CPU write field
    data.ssip = wdata[1];
    data.stip = wdata[5];
    data.seip = wdata[9];

    return data;
endfunction

function automatic logic [63:0] sie_read(
    input mie_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.ssip;
    data[5] = csr.stip;
    data[9] = csr.seip;

    return data;
endfunction

// --------------------------
// stvec
// --------------------------
typedef struct packed {
    logic [1:0] mode;
    logic [61:0] base;
} stvec_t;

function automatic stvec_t stvec_write(
    input [63:0] wdata
);
    stvec_t data = 0;
    // CPU write field
    data.mode = wdata[1:0];
    data.base = wdata[63:2];

    return data;
endfunction

function automatic logic [63:0] stvec_read(
    input stvec_t csr,
);
    logic [63:0] data = 0;
    data[1:0] = csr.mode;
    data[63:2] = csr.base;

    return data;
endfunction

// --------------------------
// scounteren
// --------------------------
typedef struct packed {
    logic cy;
    logic tm;
    logic ir;
} scounteren_t;

function automatic scounteren_t scounteren_write(
    input [63:0] wdata
);
    scounteren_t data = 0;
    // CPU write field
    data.cy = wdata[0];
    data.tm = wdata[1];
    data.ir = wdata[2];

    return data;
endfunction

function automatic logic [63:0] scounteren_read(
    input scounteren_t csr,
);
    logic [63:0] data = 0;
    data[0] = csr.cy;
    data[1] = csr.tm;
    data[2] = csr.ir;

    return data;
endfunction

// --------------------------
// sscratch
// --------------------------
typedef struct packed {
    logic [63:0] sscratch;
} sscratch_t;

function automatic sscratch_t sscratch_write(
    input [63:0] wdata
);
    sscratch_t data = 0;
    // CPU write field
    data.sscratch = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] sscratch_read(
    input sscratch_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.sscratch;

    return data;
endfunction

// --------------------------
// sepc
// --------------------------
typedef struct packed {
    logic [63:0] sepc;
} sepc_t;

function automatic sepc_t sepc_write(
    input [63:0] wdata
);
    sepc_t data = 0;
    // CPU write field
    data.sepc = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] sepc_read(
    input sepc_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.sepc;

    return data;
endfunction

// --------------------------
// scause
// --------------------------
typedef struct packed {
    logic [62:0] exception_code;
    logic is_interrupt;
} scause_t;

function automatic scause_t scause_write(
    input [63:0] wdata
);
    scause_t data = 0;
    // CPU write field

    return data;
endfunction

function automatic logic [63:0] scause_read(
    input scause_t csr,
);
    logic [63:0] data = 0;
    data[62:0] = csr.exception_code;
    data[63] = csr.is_interrupt;

    return data;
endfunction

// --------------------------
// stval
// --------------------------
typedef struct packed {
    logic [63:0] stval;
} stval_t;

function automatic stval_t stval_write(
    input [63:0] wdata
);
    stval_t data = 0;
    // CPU write field
    data.stval = wdata[63:0];

    return data;
endfunction

function automatic logic [63:0] stval_read(
    input stval_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.stval;

    return data;
endfunction

// --------------------------
// sip
// --------------------------
typedef struct packed {
    logic ssip;
    logic stip;
    logic seip;
} sip_t;

function automatic mip_t sip_write(
    input [63:0] wdata
);
    mip_t data = 0;
    // CPU write field
    data.ssip = wdata[1];
    data.stip = wdata[5];
    data.seip = wdata[9];

    return data;
endfunction

function automatic logic [63:0] sip_read(
    input mip_t csr,
);
    logic [63:0] data = 0;
    data[1] = csr.ssip;
    data[5] = csr.stip;
    data[9] = csr.seip;

    return data;
endfunction

// --------------------------
// satp
// --------------------------
typedef struct packed {
    logic [3:0] ppn;
    logic [15:0] asid;
    logic [3:0] mode;
} satp_t;

function automatic satp_t satp_write(
    input [63:0] wdata
);
    satp_t data = 0;
    // CPU write field
    data.ppn = wdata[43:40];
    data.asid = wdata[59:44];
    data.mode = wdata[63:60];

    return data;
endfunction

function automatic logic [63:0] satp_read(
    input satp_t csr,
);
    logic [63:0] data = 0;
    data[43:40] = csr.ppn;
    data[59:44] = csr.asid;
    data[63:60] = csr.mode;

    return data;
endfunction

// --------------------------
// cycle
// --------------------------
typedef struct packed {
    logic [63:0] cycle;
} cycle_t;

function automatic logic [63:0] cycle_read(
    input mcycle_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mcycle;

    return data;
endfunction

// --------------------------
// time
// --------------------------
typedef struct packed {
    logic [63:0] _time;
} time_t;

function automatic logic [63:0] time_read(
    input mtime_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.mtime;

    return data;
endfunction

// --------------------------
// instret
// --------------------------
typedef struct packed {
    logic [63:0] instret;
} instret_t;

function automatic logic [63:0] instret_read(
    input minstret_t csr,
);
    logic [63:0] data = 0;
    data[63:0] = csr.minstret;

    return data;
endfunction

`endif
