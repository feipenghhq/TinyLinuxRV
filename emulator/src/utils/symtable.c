
#include "symtable.h"

#include <assert.h>
#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static symbol_table_t table;

static char *str_dup(const char *s) {
    size_t len = strlen(s) + 1;

    char *copy = malloc(len);
    if (!copy)
        return NULL;

    memcpy(copy, s, len);
    return copy;
}

static int symbol_table_double(void) {
    table.capacity *= 2;
    symbol_t *new_table = realloc(table.symbols, sizeof(symbol_t) * table.capacity);
    if (!new_table) {
        free(table.symbols);
        return 1;
    }
    table.symbols = new_table;
    return 0;
}

int symbol_table_init(size_t capacity) {
    assert(capacity > 0);
    table.count    = 0;
    table.capacity = capacity;
    table.symbols  = malloc(sizeof(symbol_t) * capacity);
    if (!table.symbols)
        return 1;
    return 0;
}

void symbol_table_free(void) {
    for (size_t i = 0; i < table.count; i++) {
        free(table.symbols[i].name);
    }
    free(table.symbols);
}

int symbol_table_add(int type, uint64_t start, uint64_t size, const char *name) {
    if (table.count == table.capacity) {
        if (symbol_table_double() != 0) {
            return 1;
        }
    }
    table.symbols[table.count].name  = str_dup(name);
    table.symbols[table.count].type  = type;
    table.symbols[table.count].start = start;
    table.symbols[table.count].size  = size;
    table.count++;
    return 0;
}

bool symbol_table_search(uint64_t addr, const char **name) {
    for (size_t i = 0; i < table.count; i++) {
        if (table.symbols[i].type == STT_FUNC) {
            if (addr >= table.symbols[i].start && addr < table.symbols[i].start + table.symbols[i].size) {
                *name = table.symbols[i].name;
                return true;
            }
        } else {
            if (addr == table.symbols[i].start) {
                *name = table.symbols[i].name;
                return true;
            }
        }
    }
    return false;
}
