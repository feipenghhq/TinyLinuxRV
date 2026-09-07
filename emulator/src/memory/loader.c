#include <elf.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory/memory.h"
#include "utils/address_range.h"
#include "utils/log.h"

typedef enum { NOT_ELF, BAD_ELF, ELF_HEADER_OK, IO_ERROR, BAD_FILE } elf_probe_result_t;

// check if the FILE is ELF or not
static elf_probe_result_t probe_elf(FILE *fp, Elf64_Ehdr *ehdr) {
    size_t count;

    // read the ELF header
    count = fread(ehdr, 1, sizeof(*ehdr), fp);
    if (ferror(fp)) {
        LOG_ERROR("Failed to read the file");
        return IO_ERROR;
    }

    // check that we read at least 4 bytes
    if (count < SELFMAG) {
        LOG_ERROR("The file is corrupted");
        return BAD_FILE;
    }

    // Check the magic number
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) { // not ELF
        return NOT_ELF;
    } else {
        // read less then elf header size, must be a corrupted ELF
        if (count < sizeof(*ehdr)) {
            LOG_ERROR("The source file is a corrupted ELF.");
            return BAD_ELF;
        }
    }

    // Check if the ELF is a RV64 ELF
    if (ehdr->e_machine != EM_RISCV) {
        LOG_ERROR("The source file is not a RISCV executable.");
        return BAD_ELF;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        LOG_ERROR("The source file is not a 64 bit RISCV executable.");
        return BAD_ELF;
    }
    if (ehdr->e_version != EV_CURRENT) {
        LOG_ERROR("The source file is not a RISCV executable.");
        return BAD_ELF;
    }

    // Make sure we have program header and the e_phnum < PN_XNUM
    // !NOTE: Not supporting e_phnum > PN_XNUM. Will add support when we encountered such file later
    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        LOG_ERROR("No program header in the ELF file");
        return BAD_ELF;
    }
    if (ehdr->e_phnum >= PN_XNUM) {
        LOG_ERROR("Not supporting e_phum > PN_XNUM");
        return BAD_ELF;
    }
    return ELF_HEADER_OK;
}

static int load_binary_segment(memory_t *memory, FILE *fp) {
    size_t count;
    char   extra;

    count = fread(memory->data, 1, (size_t)memory->size, fp);
    if (ferror(fp)) {
        LOG_ERROR("Failed to read binary file");
        return -1;
    }

    // Check if the RAM size is too small. (If there are extra content in the file, then memory is too small)
    if (fread(&extra, 1, 1, fp) == 1) {
        LOG_ERROR("Binary file is larger than RAM");
        return -1;
    }
    if (ferror(fp)) {
        LOG_ERROR("Failed to read binary file");
        return -1;
    }

    // read successfully
    LOG_INFO("Loaded %zu bytes", count);
    (void)count;
    return 0;
}

static int elf_seek(FILE *fp, Elf64_Off offset) {
    if (offset > (Elf64_Off)LONG_MAX) {
        LOG_ERROR("ELF offset is too large. Might be a corrupted ELF");
        return -1;
    }

    if (fseek(fp, (long)offset, SEEK_SET) != 0) {
        LOG_ERROR("Failed to seek ELF file");
        return -1;
    }
    return 0;
}

static int memory_fread(memory_t *memory, uint64_t start_addr, size_t size, FILE *fp) {
    size_t   count;
    uint64_t offset = start_addr - memory->base;
    // check address range
    if (!check_addr_range(start_addr, size, memory->base, memory->size)) { // out of range
        LOG_ERROR("Address out of memory range");
        return -1;
    }
    count = fread(&memory->data[offset], 1, size, fp);
    if (count != size) {
        LOG_ERROR("Failed to read file");
        return -1;
    }
    return 0;
}

static int load_elf_segments(memory_t *memory, FILE *fp, const Elf64_Ehdr *ehdr, uint64_t *entry_point) {
    Elf64_Phdr phdr;
    Elf64_Off  next_phoff;
    size_t     count;
    // Track loading and entry validation separately: data segments are not executable.
    bool valid_entry = false;
    bool loaded      = false;

    // We have already read and check ehdr, Now load program header one by one.
    next_phoff = ehdr->e_phoff;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (elf_seek(fp, next_phoff) != 0) {
            return -1;
        }
        // read the phdr from file
        count = fread(&phdr, 1, sizeof(phdr), fp);
        if (count != sizeof(phdr)) {
            LOG_ERROR("Failed to read file");
            return -1;
        }

        // Load every PT_LOAD segment, including non-executable data segments.
        if (phdr.p_type == PT_LOAD) {

            // ELF segment file size exceeds memory size
            if (phdr.p_filesz > phdr.p_memsz) {
                LOG_ERROR("ELF segment file size exceeds memory size");
                return -1;
            }
            // Failed to seek the file the
            if (elf_seek(fp, phdr.p_offset) != 0) {
                return -1;
            }
            // Read the segment into the memory
            if (memory_fread(memory, phdr.p_vaddr, phdr.p_filesz, fp) != 0) {
                return -1;
            }
            // "load" bss and clear it
            if (memory_set(memory, phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz) == NULL) {
                return -1;
            }

            loaded = true;
            // p_flags is a bitmask, so executable segments may also have PF_R.
            // Compare the entry offset to avoid overflowing p_vaddr + p_memsz.
            if ((phdr.p_flags & PF_X) != 0 && ehdr->e_entry >= phdr.p_vaddr &&
                ehdr->e_entry - phdr.p_vaddr < phdr.p_memsz) {
                valid_entry = true;
            }
        }
        next_phoff += ehdr->e_phentsize;
    }

    // load completed. check if we every loaded anything.
    if (!loaded) {
        LOG_ERROR("No loadable program segment in the elf file.");
        return -1;
    }
    if (!valid_entry) {
        LOG_ERROR("e_entry not in any executable PT_LOAD segment");
        return -1;
    }

    *entry_point = ehdr->e_entry;
    return 0;
}

int memory_load_binary(memory_t *memory, const char *file) {
    FILE *fp = NULL;
    int   result;

    fp = fopen(file, "rb");
    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", file, strerror(errno));
        return -1;
    }
    result = load_binary_segment(memory, fp);
    fclose(fp);
    return result;
}

int memory_load_elf(memory_t *memory, const char *file, uint64_t *entry_point) {
    FILE      *fp = NULL;
    Elf64_Ehdr ehdr;
    int        result = 0;

    fp = fopen(file, "rb");
    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", file, strerror(errno));
        return -1;
    }

    switch (probe_elf(fp, &ehdr)) {
    case BAD_FILE: // fall-through
    case IO_ERROR: {
        fclose(fp);
        return -1;
    }
    case NOT_ELF: {
        LOG_ERROR("The source file is not an ELF file");
        fclose(fp);
        return -1;
    }
    case BAD_ELF: {
        fclose(fp);
        return -1;
    }
    case ELF_HEADER_OK: {
        result = load_elf_segments(memory, fp, &ehdr, entry_point);
    }
    }

    fclose(fp);
    return result;
}

int memory_load_auto(memory_t *memory, const char *file, uint64_t *entry_point) {
    FILE      *fp = NULL;
    Elf64_Ehdr ehdr;
    int        result = 0;

    fp = fopen(file, "rb");
    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", file, strerror(errno));
        return -1;
    }

    switch (probe_elf(fp, &ehdr)) {
    case BAD_FILE: // fall-through
    case IO_ERROR: // fall-through
    case BAD_ELF: {
        fclose(fp);
        return -1;
    }
    case ELF_HEADER_OK: {
        result = load_elf_segments(memory, fp, &ehdr, entry_point);
        break;
    }
    case NOT_ELF: {
        rewind(fp);
        result = load_binary_segment(memory, fp);
        break;
    }
    }

    fclose(fp);
    return result;
}
