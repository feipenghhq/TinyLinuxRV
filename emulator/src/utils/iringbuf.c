#include "iringbuf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define IRINGBUF_LEN 16

typedef struct {
    uint64_t addr[IRINGBUF_LEN];
    uint32_t inst[IRINGBUF_LEN];
    int      count;
    int      ptr;
} iringbuf_t;

static iringbuf_t iringbuf = {.count = 0, .ptr = 0};

static inline void ptr_advance(void) {
    if (++iringbuf.ptr == IRINGBUF_LEN) {
        iringbuf.ptr = 0;
    }
}

void iringbuf_write(uint64_t addr, uint32_t inst) {
    iringbuf.addr[iringbuf.ptr] = addr;
    iringbuf.inst[iringbuf.ptr] = inst;
    if (iringbuf.count < IRINGBUF_LEN) {
        iringbuf.count++;
    }
    ptr_advance();
}

void iringbuf_print(void) {
    int first_pos    = 0;
    int physical_pos = 0;

    fprintf(stderr, "Instruction sequence to error instruction (Dump from iringbuf):\n");
    if (iringbuf.count == 16) {
        first_pos = iringbuf.ptr;
    }

    for (int i = 0; i < iringbuf.count - 1; i++) {
        physical_pos = (first_pos + i) % IRINGBUF_LEN;
        fprintf(stderr, "     @%lx: %08x\n", iringbuf.addr[physical_pos], iringbuf.inst[physical_pos]);
    }

    // last one
    physical_pos = (first_pos + iringbuf.count - 1) % IRINGBUF_LEN;
    fprintf(stderr, "---> @%lx: %08x\n", iringbuf.addr[physical_pos], iringbuf.inst[physical_pos]);
}
