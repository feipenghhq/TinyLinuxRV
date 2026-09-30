#include "utils/func_trace.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "riscv.h"
#include "symtable.h"
#include "utils/log.h"

#define RISCV_TEST_MODE 1

static int   call_depth  = 0;
static FILE *fp          = NULL;
static char  priv_char[] = {'U', 'S', 'R', 'M'};

// Convert the high virtual address used by riscv-tests back to the
// original ELF link address for symbol lookup.
static uint64_t resolve_symbol_addr(uint64_t addr) {
    if (addr >= 0xffffffffffe00000ULL)
        return addr - 0xffffffffffe00000ULL + 0x80000000ULL;

    return addr;
}

static void trace_jump_or_call(uint64_t pc, uint64_t target, priv_mode_t priv, char *type) {
    char        target_buf[32];
    const char *name;

    if (RISCV_TEST_MODE) {
        pc     = resolve_symbol_addr(pc);
        target = resolve_symbol_addr(target);
    }

    if (symbol_table_search(target, &name)) {
        fprintf(fp, "%*s[%c] %-4s %-18s    <- 0x%016lx\n", call_depth * 2, "", priv_char[priv], type, name, pc);
    } else {
        snprintf(target_buf, sizeof(target_buf), "0x%016lx", target);
        fprintf(fp, "%*s[%c] %-4s %-18s    <- 0x%016lx\n", call_depth * 2, "", priv_char[priv], type, target_buf, pc);
    }
    call_depth++;
}

void trace_call(uint64_t pc, uint64_t target, priv_mode_t priv) {
    trace_jump_or_call(pc, target, priv, "CALL");
}

void trace_jump(uint64_t pc, uint64_t target, priv_mode_t priv) {
    trace_jump_or_call(pc, target, priv, "JUMP");
}

void trace_trap(uint64_t pc, uint64_t target, priv_mode_t priv) {
    trace_jump_or_call(pc, target, priv, "TRAP");
}

void trace_return(uint64_t pc, uint64_t target, priv_mode_t priv) {
    char        pc_buf[32];
    const char *name;

    if (RISCV_TEST_MODE) {
        pc     = resolve_symbol_addr(pc);
        target = resolve_symbol_addr(target);
    }

    if (call_depth > 0)
        call_depth--;
    if (symbol_table_search(pc, &name)) {
        fprintf(fp, "%*s[%c] %-4s %-18s    -> 0x%016lx\n", call_depth * 2, "", priv_char[priv], "RET", name, target);
    } else {
        snprintf(pc_buf, sizeof(pc_buf), "0x%016lx", pc);
        fprintf(fp, "%*s[%c] %-4s %-18s    -> 0x%016lx\n", call_depth * 2, "", priv_char[priv], "RET", pc_buf, target);
    }
}

int func_trace_init(const char *file) {
    call_depth = 0;

    fp = fopen(file, "w");
    if (fp == NULL) {
        LOG_ERROR("Unable to open the file: %s", file);
        return 1;
    }
    return 0;
}

void func_trace_end(void) {
    fclose(fp);
}
