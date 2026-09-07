#ifndef CLINT_H
#define CLINT_H

#include <stdint.h>

void clint_set_msip(uint64_t base);
void clint_clear_msip(uint64_t base);

void     clint_set_mtime(uint64_t base, uint64_t value);
uint64_t clint_read_mtime(uint64_t base);

void     clint_set_mtimecmp(uint64_t base, uint64_t value);
uint64_t clint_read_mtimecmp(uint64_t base);

#endif
