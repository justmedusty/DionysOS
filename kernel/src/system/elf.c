//
// Created by dustyn on 6/15/25.
//

#include "include/definitions/elf.h"
#include "include/memory/mem.h"
#include <stddef.h>
#include <stdint.h>
#include "include/memory/pmm.h"
#include "include/architecture/arch_vmm.h"
#include "include/definitions/string.h"
#include "include/memory/kmalloc.h"
#include "include/memory/vmm.h"
#include "include/scheduling/process.h"
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

    elf64_phdr program_header;
    for (size_t i = 0; i < header.e_phnum; i++) {
        size_t program_header_offset = header.e_phoff + (i * sizeof(elf64_phdr));

        if (read(handle, (char *) &program_header, sizeof(elf64_phdr)) < 0) {
            return KERN_BAD_DESCRIPTOR;
        }

        switch (program_header.p_type) {
            case PT_LOAD:
                uint64_t memory_protection = 0;
                memory_protection |= USER;

                if (program_header.p_flags & PF_R) {
                    memory_protection |= READ;
                } else {
                    return KERN_BAD_DESCRIPTOR;
                }

                if (program_header.p_flags & PF_W) {
                    memory_protection |= READWRITE;
                }

                if (program_header.p_flags & PF_X) {
                    //nothing
                } else {
                    memory_protection |= NO_EXECUTE;
                }

                uint64_t aligned_address = ALIGN_DOWN(program_header.p_vaddr, PAGE_SIZE);
                uint64_t aligned_diff = program_header.p_vaddr + base_address - aligned_address;

                uint64_t page_count = ALIGN_UP(program_header.p_memsz + aligned_diff, PAGE_SIZE) / PAGE_SIZE;

                for (size_t j = 0; j < page_count; j++) {
                    uint64_t *physical_page = umalloc(1);
                    arch_map_pages(process->page_map->top_level, (uint64_t) physical_page,
                                   (uint64_t *) aligned_address + (uint64_t) (j * PAGE_SIZE), memory_protection,
                                   PAGE_SIZE);
                }

                arch_map_foreign(process->page_map->top_level, (uint64_t *) aligned_address, PAGE_SIZE * page_count);

                read(handle, (char *) KERNEL_FOREIGN_MAP_BASE + aligned_diff, page_count * PAGE_SIZE);
                memset((void *) KERNEL_FOREIGN_MAP_BASE + aligned_diff, 0,
                       program_header.p_memsz - program_header.p_filesz);

                arch_unmap_foreign(PAGE_SIZE * page_count);


                break;
            case PT_PHDR:
                info->at_phdr = base_address + program_header.p_vaddr;
                break;
            case PT_INTERP:
                info->ld_path = kzmalloc(program_header.p_filesz);
                seek(handle, program_header.p_offset);
                read(handle, info->ld_path, program_header.p_filesz);
                break;
            default:
                break;
        }
    }
    info->at_entry = base_address + header.e_entry;
    info->at_phnum = header.e_phnum;
    info->at_phent = header.e_phentsize;

    return KERN_SUCCESS;
}
