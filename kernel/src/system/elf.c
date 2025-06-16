//
// Created by dustyn on 6/15/25.
//

#include "../include/definitions/elf.h"
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

    elf64_hdr *elf_64 = elf;
    elf64_shdr *elf_shdr = elf + elf_64->e_shoff;
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

int64_t load_elf(struct process *process, int64_t handle, size_t base_address, elf_info *info) {
    if (handle < 0) {
        return KERN_BAD_HANDLE;
    }

    elf64_hdr header;

    if (read(handle, (char *) &header, sizeof(elf64_hdr) != KERN_SUCCESS)) {
        return KERN_BAD_DESCRIPTOR;
    }

    if (memcmp(header.e_ident, ELF_MAG, sizeof(ELF_MAG)) != 0) {
        return KERN_WRONG_TYPE;
    }

    if (header.e_ident[EI_CLASS] != EI_ARCH_CLASS || header.e_ident[EI_DATA] != EI_ARCH_DATA ||
        header.e_ident[EI_VERSION] != EV_CURRENT || header.e_ident[EI_OSABI] != ELFOSABI_SYSV ||
        header.e_machine != EI_ARCH_MACHINE || header.e_phentsize != sizeof(elf64_phdr)) {
        return KERN_WRONG_TYPE;
    }
}
