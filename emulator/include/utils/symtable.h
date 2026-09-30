#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    int      type;
    uint64_t start;
    uint64_t size;
    char    *name;
} symbol_t;

typedef struct {
    symbol_t *symbols;
    size_t    count;
    size_t    capacity;
} symbol_table_t;

int  symbol_table_init(size_t capacity);
void symbol_table_free(void);
int  symbol_table_add(int type, uint64_t start, uint64_t size, const char *name);
bool symbol_table_search(uint64_t addr, const char **name);

#endif
