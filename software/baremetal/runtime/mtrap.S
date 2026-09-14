
.macro SAVE_GPRS
    sd zero,   0(sp)
    sd ra,     8(sp)
    # sp: save at the end
    sd gp,    24(sp)
    sd tp,    32(sp)
    sd t0,    40(sp)
    sd t1,    48(sp)
    sd t2,    56(sp)
    sd s0,    64(sp)
    sd s1,    72(sp)
    sd a0,    80(sp)
    sd a1,    88(sp)
    sd a2,    96(sp)
    sd a3,   104(sp)
    sd a4,   112(sp)
    sd a5,   120(sp)
    sd a6,   128(sp)
    sd a7,   136(sp)
    sd s2,   144(sp)
    sd s3,   152(sp)
    sd s4,   160(sp)
    sd s5,   168(sp)
    sd s6,   176(sp)
    sd s7,   184(sp)
    sd s8,   192(sp)
    sd s9,   200(sp)
    sd s10,  208(sp)
    sd s11,  216(sp)
    sd t3,   224(sp)
    sd t4,   232(sp)
    sd t5,   240(sp)
    sd t6,   248(sp)
.endm

.macro RESTORE_GPRS
    ld ra,     8(sp)
    # sp: restore at end
    ld gp,    24(sp)
    ld tp,    32(sp)
    ld t0,    40(sp)
    ld t1,    48(sp)
    ld t2,    56(sp)
    ld s0,    64(sp)
    ld s1,    72(sp)
    ld a0,    80(sp)
    ld a1,    88(sp)
    ld a2,    96(sp)
    ld a3,   104(sp)
    ld a4,   112(sp)
    ld a5,   120(sp)
    ld a6,   128(sp)
    ld a7,   136(sp)
    ld s2,   144(sp)
    ld s3,   152(sp)
    ld s4,   160(sp)
    ld s5,   168(sp)
    ld s6,   176(sp)
    ld s7,   184(sp)
    ld s8,   192(sp)
    ld s9,   200(sp)
    ld s10,  208(sp)
    ld s11,  216(sp)
    ld t3,   224(sp)
    ld t4,   232(sp)
    ld t5,   240(sp)
    ld t6,   248(sp)
.endm

#define SP_OFFSET 16

#define MEPC_OFFSET    256
#define MSTATUS_OFFSET 264
#define MCAUSE_OFFSET  272
#define MTVAL_OFFSET   280

// make frame size 16 byte aligned
#define FRAME_SIZE     288

.section .text.trap

.globl mtrap_entry
mtrap_entry:

    // assign stack to store 32 register + mepc, mstatus, mcause, mtval
    addi sp, sp, -FRAME_SIZE

    // save context
    SAVE_GPRS
    addi t0, sp, FRAME_SIZE
    sd t0, SP_OFFSET(sp)

    csrr t0, mepc
    sd t0, MEPC_OFFSET(sp)
    csrr t0, mstatus
    sd t0, MSTATUS_OFFSET(sp)
    csrr t0, mcause
    sd t0, MCAUSE_OFFSET(sp)
    csrr t0, mtval
    sd t0, MTVAL_OFFSET(sp)

    // call handler
    mv a0, sp
    call trap_handler

    // update MEPC
    ld t0, MEPC_OFFSET(sp)
    csrw mepc, t0

    // restore context
    RESTORE_GPRS
    ld sp, SP_OFFSET(sp)

    // return from trap
    mret
