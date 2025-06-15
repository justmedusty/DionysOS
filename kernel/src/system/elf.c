//
// Created by dustyn on 6/15/25.
//

#include "include/definitions/elf.h"

#include "include/memory/mem.h"
#include <stddef.h>
#include <stdint.h>

#include "include/definitions/definitions.h"


void *elf_section_get(void *elf, const char *name) {
    if (!elf || !name) {
        return NULL;
    }

    if (((uint8_t *) elf)[EI_CLASS] != ELFCLASS64) {
        return NULL;
    }

    Elf64_Hdr *elf_64 = elf;
    Elf64_Shdr *elf_shdr = elf + elf_64->e_shoff;
    char *shstrtab = elf + elf_shdr[elf_64->e_shstrndx].sh_offset;

    for (size_t i = 0; i < elf_64->e_shnum; i++) {
        const size_t name_len = strlen(name);
        const size_t sect_len = strlen(shstrtab + elf_shdr[i].sh_name);

        if (name_len != sect_len)
            continue;

        if (memcmp(name, shstrtab + elf_shdr[i].sh_name, min(name_len, sect_len)) == 0) {
            return elf_shdr + i;
        }
    }
}
