#include <elf.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "memory.h"

#define FILE_READ_EXACT_OR_RETURN(dst, size, fp)      \
    do {                                              \
        FILE  *_fp   = (fp);                          \
        size_t _size = (size);                        \
        size_t count = fread((dst), 1, _size, (_fp)); \
        if (count != _size) {                         \
            LOG_ERROR("Failed to read file");         \
            fclose(_fp);                              \
            return -1;                                \
        }                                             \
    } while (0)

static int file_is_elf(const char *file) {
    FILE         *fp;
    unsigned char magic[SELFMAG];

    fp = fopen(file, "rb");
    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", file, strerror(errno));
        return -1;
    }

    // read the magic number
    size_t count = fread(magic, 1, sizeof(magic), fp);
    if (ferror(fp)) {
        LOG_ERROR("Failed to read %s", file);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    if (count != sizeof(magic)) {
        return -1;
    }
    if (memcmp(magic, ELFMAG, SELFMAG) != 0) {
        return -1;
    }
    return 0;
}

static int check_ehdr(Elf64_Ehdr *ehdr) {
    // some basic check
    if (ehdr->e_machine != EM_RISCV) {
        LOG_ERROR("The source file is not a RISCV executable.");
        return -1;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        LOG_ERROR("The source file is not a 64 bit RISCV executable.");
        return -1;
    }
    if (ehdr->e_version != EV_CURRENT) {
        LOG_ERROR("The source file is not a RISCV executable.");
        return -1;
    }

    // make sure we have program header and the e_phnum < PN_XNUM
    // Not supporting e_phnum > PN_XNUM. Will add support when we encountered such file later
    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        LOG_ERROR("No program header in the ELF file");
        return -1;
    }

    if (ehdr->e_phnum >= PN_XNUM) {
        LOG_ERROR("Not supporting e_phum > PN_XNUM");
        return -1;
    }
    return 0;
}

static int elf_seek(FILE *fp, Elf64_Off offset) {
    if (offset > (Elf64_Off)LONG_MAX) {
        LOG_ERROR("ELF offset is too large");
        return -1;
    }

    if (fseek(fp, (long)offset, SEEK_SET) != 0) {
        LOG_ERROR("Failed to seek ELF file");
        return -1;
    }
    return 0;
}

static int memory_fread(memory_t *memory, uint64_t start_addr, size_t size, FILE *stream) {
    uint64_t offset = start_addr - memory->base;
    // check address range
    if (start_addr < memory->base || start_addr > memory->end || size > memory->end - start_addr) { // out of range
        fclose(stream);
        LOG_ERROR("Address out of memory range");
        return -1;
    }
    FILE_READ_EXACT_OR_RETURN(&memory->data[offset], size, stream);
    return 0;
}

int memory_load_elf(memory_t *memory, const char *file, uint64_t *entry_point) {
    FILE         *fp;
    unsigned char magic[4];
    Elf64_Ehdr    ehdr;
    Elf64_Phdr    phdr;
    Elf64_Off     next_phoff;
    // Track loading and entry validation separately: data segments are not executable.
    bool          valid_entry = false;
    bool          loaded      = false;

    fp = fopen(file, "rb");
    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", file, strerror(errno));
        return -1;
    }

    // check the magic number
    FILE_READ_EXACT_OR_RETURN(magic, 4, fp);
    if (memcmp(magic, ELFMAG, SELFMAG) != 0) {
        fclose(fp);
        LOG_ERROR("The source file %s is not an ELF file.", file);
        return -1;
    }
    rewind(fp);

    // read ehdr
    FILE_READ_EXACT_OR_RETURN(&ehdr, sizeof(ehdr), fp);

    // check ehdr
    if (check_ehdr(&ehdr) != 0) {
        fclose(fp);
        return -1;
    }

    // Now load program header
    next_phoff = ehdr.e_phoff;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (elf_seek(fp, next_phoff) != 0) {
            fclose(fp);
            return -1;
        }
        FILE_READ_EXACT_OR_RETURN(&phdr, sizeof(phdr), fp);
        if (phdr.p_type == PT_LOAD) {
            // Load every PT_LOAD segment, including non-executable data segments.
            if (phdr.p_filesz > phdr.p_memsz) {
                fclose(fp);
                LOG_ERROR("ELF segment file size exceeds memory size");
                return -1;
            }
            if (elf_seek(fp, phdr.p_offset) != 0) {
                fclose(fp);
                return -1;
            }
            if (memory_fread(memory, phdr.p_vaddr, phdr.p_filesz, fp) != 0) {
                // No need to close file. File is already closed.
                return -1;
            }
            // "load" bss and clear it
            if (memory_set(memory, phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz) == NULL) {
                fclose(fp);
                return -1;
            }

            loaded = true;
            // p_flags is a bitmask, so executable segments may also have PF_R.
            // Compare the entry offset to avoid overflowing p_vaddr + p_memsz.
            if ((phdr.p_flags & PF_X) != 0 && ehdr.e_entry >= phdr.p_vaddr &&
                ehdr.e_entry - phdr.p_vaddr < phdr.p_memsz) {
                valid_entry = true;
            }
        }
        next_phoff += ehdr.e_phentsize;
    }
    fclose(fp);

    if (!loaded) {
        LOG_ERROR("No loadable program segment in the elf file.");
        return -1;
    }

    if (!valid_entry) {
        LOG_ERROR("e_entry not in any executable PT_LOAD segment");
        return -1;
    }

    *entry_point = ehdr.e_entry;
    return 0;
}

int memory_load_binary(memory_t *memory, const char *file) {
    FILE *fp = fopen(file, "rb");
    char  extra;

    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", file, strerror(errno));
        return -1;
    }
    size_t count = fread(memory->data, 1, (size_t)memory->size, fp);
    if (ferror(fp)) {
        fclose(fp);
        LOG_ERROR("Failed to read binary file");
        return -1;
    }

    // Check if the RAM size is too small
    if (fread(&extra, 1, 1, fp) == 1) {
        fclose(fp);
        LOG_ERROR("Binary file is larger than RAM");
        return -1;
    }
    if (ferror(fp)) {
        fclose(fp);
        LOG_ERROR("Failed to read binary file");
        return -1;
    }

    fclose(fp);
    LOG_INFO("Loaded %zu bytes", count);
    (void)count;
    return 0;
}

int memory_load_auto(memory_t *memory, const char *file, uint64_t *entry_point) {
    if (file_is_elf(file) == 0) {
        return memory_load_elf(memory, file, entry_point);
    } else {
        return memory_load_binary(memory, file);
    }
}
