#ifndef FUNC_TRACE_H
#define FUNC_TRACE_H

#include <stdbool.h>
#include <stdint.h>

#include "riscv.h"

int  func_trace_init(const char *file);
void func_trace_end(void);
void trace_call(uint64_t pc, uint64_t target, priv_mode_t priv);
void trace_jump(uint64_t pc, uint64_t target, priv_mode_t priv);
void trace_return(uint64_t pc, uint64_t target, priv_mode_t priv);
void trace_trap(uint64_t pc, uint64_t target, priv_mode_t priv);

#endif
