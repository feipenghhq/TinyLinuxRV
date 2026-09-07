#ifndef ADDRESS_RANGE_H
#define ADDRESS_RANGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline bool check_addr_range(uint64_t addr, size_t nbyte, uint64_t base, uint64_t size) {
    // check address range
    if (addr >= base && nbyte <= size && addr - base <= size - nbyte) {
        return true;
    }
    return false;
}

#endif
