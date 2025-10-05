//
// Created by dustyn on 6/15/25.
//


#include "include/memory/mem.h"
#include "include/definitions/definitions.h"
#include <stdint.h>
#include "include/memory/pmm.h"
#include "include/architecture/arch_vmm.h"
#include "include/definitions/string.h"
#include "include/memory/kmalloc.h"
#include "include/memory/vmm.h"
#include "include/scheduling/process.h"
#include "include/definitions/elf.h"

#include "include/definitions/definitions.h"
#include "include/drivers/serial/uart.h"
#include "include/definitions/definitions.h"

#include "include/filesystem/vfs.h"

void *elf_section_get(void *elf, const char *name) {
    if (!elf || !name) {
        return NULL;
    }

    if (((uint8_t *) elf)[EI_CLASS] != ELFCLASS64) {
        return NULL;
    }

    void *result = NULL;

    elf64_hdr *elf_64 = elf;
    elf64_shdr *elf_shdr = elf + elf_64->e_shoff;
    char *shstrtab = elf + elf_shdr[elf_64->e_shstrndx].sh_offset;

    for (size_t i = 0; i < elf_64->e_shnum; i++) {
        const size_t name_len = strlen(name);
        const size_t sect_len = strlen(shstrtab + elf_shdr[i].sh_name);

        if (name_len != sect_len)
            continue;

        if (memcmp(name, shstrtab + elf_shdr[i].sh_name, min(name_len, sect_len)) == 0) {
            result = elf_shdr + i;
            break;
        }
    }
    return result;
}

int64_t load_elf(struct process *process, char *path, size_t base_address, elf_info *info) {
    if (!path) {
        return KERN_BAD_HANDLE;
    }
    bool init = false;
    elf64_hdr *header = kzmalloc(sizeof(elf64_hdr));
    //This must be init setup
    if (current_process() == NULL) {
        init = true;
        my_cpu()->running_process = process;
    }

    int64_t handle = vnode_open(path);
    if (handle < 0) {
        panic("INIT NOT FOUND!\n");
        if (init) {
            my_cpu()->running_process = NULL;
        }
        return KERN_BAD_HANDLE;
    }

    if (read(handle, (char *) header, sizeof(elf64_hdr) != KERN_SUCCESS)) {
        if (init) {
            my_cpu()->running_process = NULL;
        }
        return KERN_BAD_DESCRIPTOR;
    }

DEBUG_PRINT(" e_type=%x.64 e_machine=0x%x.64 e_version=0x%x.64 e_entry=0x%x.64 e_phoff=0x%x.64 e_shoff=0x%x.64 e_flags=0x%x.64 e_ehsize=%i e_phentsize=%i e_phnum=%i e_shentsize=%i e_shnum=%i e_shstrndx=%i\n",header->e_type,header->e_machine,
    header->e_version,
    (unsigned long)header->e_entry,
    (unsigned long)header->e_phoff,
    (unsigned long)header->e_shoff,
    header->e_flags,
    header->e_ehsize,
    header->e_phentsize,
    header->e_phnum,
    header->e_shentsize,
    header->e_shnum,
    header->e_shstrndx
);
    if (memcmp(header->e_ident, ELF_MAG, sizeof(ELF_MAG)) != 0) {
        if (init) {
            my_cpu()->running_process = NULL;
        }
        return KERN_WRONG_TYPE;
    }

    if (header->e_ident[EI_CLASS] != EI_ARCH_CLASS || header->e_ident[EI_DATA] != EI_ARCH_DATA ||
        header->e_ident[EI_VERSION] != EV_CURRENT || header->e_ident[EI_OSABI] != ELFOSABI_SYSV ||
        header->e_machine != EI_ARCH_MACHINE || header->e_phentsize != sizeof(elf64_phdr)) {
        if (init) {
            my_cpu()->running_process = NULL;
        }
        return KERN_WRONG_TYPE;
    }
    struct vnode *elf = handle_to_vnode(handle);
    char *elf_file = kzmalloc(elf->vnode_size);

    int64_t ret = vnode_read(elf,0,elf->vnode_size,elf_file);

    if (ret != KERN_SUCCESS) {
        kprintf("vnode_read failed!\n");
        panic("COULD NOT READ ELF FILE!\n");
    }

    elf64_phdr *program_header;
    for (size_t i = 0; i < header->e_phnum; i++) {
        size_t program_header_offset = header->e_phoff + (i * sizeof(elf64_phdr));
        DEBUG_PRINT("I %i HEAD OFFSET %i PROGRAM HEADER OFFSET %i\n",i,header->e_phoff,program_header_offset);
        program_header = (elf64_phdr *) ((size_t) elf_file + program_header_offset);
        DEBUG_PRINT("P TYPE IS %i\n",program_header->p_type);
        switch (program_header->p_type) {
            case PT_LOAD:
                uint64_t memory_protection = 0;
                memory_protection |= USER;

                if (program_header->p_flags & PF_R) {
                    memory_protection |= READ;
                } else {
                    if (init) {
                        my_cpu()->running_process = NULL;
                    }
                    return KERN_BAD_DESCRIPTOR;
                }

                if (program_header->p_flags & PF_W) {
                    memory_protection |= READWRITE;
                }

                if (program_header->p_flags & PF_X) {
                    //nothing
                } else {
                    memory_protection |= NO_EXECUTE;
                }

                uint64_t aligned_address = ALIGN_DOWN(program_header->p_vaddr + base_address, PAGE_SIZE);
                uint64_t aligned_diff = program_header->p_vaddr + base_address - aligned_address;

                uint64_t page_count = ALIGN_UP(program_header->p_memsz + aligned_diff, PAGE_SIZE) / PAGE_SIZE;

                DEBUG_PRINT("PAGE COUNT %i MEM SIZE %x.64 ALIGNED DIFF %x.64 ALIGNED ADDRES %x.64 PVIRT %x.64 BASE %x.64\n",page_count,program_header->p_memsz,aligned_diff,aligned_address,program_header->p_vaddr,base_address);
                for (size_t j = 0; j < page_count; j++) {
                    uint64_t *physical_page = umalloc(1);
                    serial_printf("MAPPING PAGE %x.64 TO VA %x.64\n",physical_page, (uint64_t *) (uint64_t)(aligned_address + (uint64_t) (j * PAGE_SIZE)));
                    arch_map_pages(process->page_map->top_level, (uint64_t) physical_page,
                                   (uint64_t *) (aligned_address +  (j * PAGE_SIZE)), memory_protection,
                                   PAGE_SIZE);
                    DEBUG_PRINT("VA %x.64\n",(uint64_t *) (aligned_address +  (j * PAGE_SIZE)));
                    DEBUG_PRINT("PHYSICAL %x.64 J %i\n",physical_page,j);

                }
                warn_printf("enter\n");
                DEBUG_PRINT("FOREIGN MAPPING NOW! ALIGNED ADDR %x.64\n",aligned_address);
                arch_map_foreign(process->page_map->top_level, (uint64_t *) aligned_address, page_count);
                DEBUG_PRINT("FOREIGN %x.64",KERNEL_FOREIGN_MAP_BASE);
                memcpy(((void *)((uint64_t) KERNEL_FOREIGN_MAP_BASE)),elf_file + program_header->p_offset,program_header->p_filesz);
                memset((void *) KERNEL_FOREIGN_MAP_BASE + aligned_diff + program_header->p_filesz, 0,
                       program_header->p_memsz - program_header->p_filesz);
                DEBUG_PRINT("unmap size: %i\n",PAGE_SIZE * page_count);
                arch_unmap_foreign(PAGE_SIZE * page_count);
                warn_printf("exit\n");
                break;
            case PT_PHDR:
                info->at_phdr = base_address + program_header->p_vaddr;
                break;
            case PT_INTERP:
                info->ld_path = kzmalloc(program_header->p_filesz);
                info->ld_path = (char *) ((uint64_t) elf_file + program_header->p_filesz);
                break;
            default:
                DEBUG_PRINT("Unknown elf section!\n");
                break;
        }
    }
    info->at_entry = base_address + header->e_entry;
    info->at_phnum = header->e_phnum;
    info->at_phent = header->e_phentsize;

#ifdef __x86_64__
    process->current_register_state->rip = header->e_entry;
    process->current_register_state->rsp = (uint64_t) (process->kernel_stack + DEFAULT_STACK_SIZE);
    process->current_register_state->rbp = USER_STACK_TOP;
#endif
    if (init) {
        my_cpu()->running_process = NULL;
    }
    DEBUG_PRINT("LOAD SUCCESSFUL! ENTRY IS %x.64",info->at_entry);
    kfree(header);
    return KERN_SUCCESS;
}

int64_t elf_relocate(elf64_rela *relocation, elf64_sym *symtab_data, char *strtab_data,elf64_shdr *section_headers,void *virtual_base) {
    elf64_sym* symbol = (elf64_sym *) ((uint64_t)symtab_data + (uint64_t) ELF64_R_SYM(relocation->r_info));
    const char* symbol_name = strtab_data + symbol->st_name;

    void* location = virtual_base + relocation->r_offset;

    switch (ELF_R_TYPE(relocation->r_info))
    {
#if defined(__x86_64__)
        case R_X86_64_64:
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
#endif
        {
            void* resolved;
            if (symbol->st_shndx == 0)
            {

                   //Need to write a symbol resolving function
                elf64_sym resolved_sym = {0};
                if (resolved_sym.st_value == 0)
                {

                    return KERN_NOT_FOUND;
                }
              resolved = (void*)resolved_sym.st_value;
            }
            else
                resolved = virtual_base + symbol->st_value;

            *(void**)location = resolved + relocation->r_addend;
            break;
        }
#if defined(__x86_64__)
        case R_X86_64_RELATIVE:
#endif
        {
            *(void**)location = virtual_base + relocation->r_addend;
            break;
        }
        default:
        {
            return KERN_NOT_FOUND;
        }
    }
    return KERN_SUCCESS;
}
