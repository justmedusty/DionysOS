//
// Created by dustyn on 6/25/24.
//

#ifndef _ELF_H
#define _ELF_H
// Format of an ELF executable file
#pragma once
#include <stdint.h>
#include <stddef.h>
// ELF Header Identification
#define ELF_MAG (const char[4]) {0x7F, 'E', 'L', 'F'}

// e_ident[] indexes
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

// File class
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFCLASSNUM 3

// Data encoding
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#define ELFDATANUM 3

// ELF version
#define EV_NONE 0
#define EV_CURRENT 1
#define EV_NUM 2

// OS ABI
#define ELFOSABI_SYSV 0
#define ELFOSABI_HPUX 1
#define ELFOSABI_STANDALONE 255

// ELF file types
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOOS 0xFE00
#define ET_HIOS 0xFEFF
#define ET_LOPROC 0xFF00
#define ET_HIPROC 0xFFFF

// Program header types
#define PT_NULL 0x00000000
#define PT_LOAD 0x00000001
#define PT_DYNAMIC 0x00000002
#define PT_INTERP 0x00000003
#define PT_NOTE 0x00000004
#define PT_SHLIB 0x00000005
#define PT_PHDR 0x00000006
#define PT_TLS 0x00000007

// Program header flags
#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x04

// Section header flags
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKOS 0x0F000000
#define SHF_MASKPROC 0xF0000000

// Section header types
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11

// Symbol table macros
#define ELF_ST_BIND(i) ((i) >> 4)
#define ELF_ST_TYPE(i) ((i) & 0xf)
#define ELF_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

// Relocation macros
#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)
#define ELF64_R_INFO(s, t) (((s) << 32) + ((t) & 0xffffffffL))
#define ELF_R_SYM(i) ELF64_R_SYM(i)
#define ELF_R_TYPE(i) ELF64_R_TYPE(i)
#define ELF_R_INFO(s, t) ELF64_R_INFO(s, t)

// Dynamic table tags
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_SYMBOLIC 16
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_PLTREL 20
#define DT_DEBUG 21
#define DT_TEXTREL 22
#define DT_JMPREL 23
#define DT_BIND_NOW 24
#define DT_INIT_ARRAY 25
#define DT_FINI_ARRAY 26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_LOOS 0x60000000
#define DT_HIOS 0x6FFFFFFF
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7FFFFFFF

// Symbol bindings
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOOS 10
#define STB_HIOS 12
#define STB_LOPROC 13
#define STB_HIPROC 15

// Symbol types
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOOS 10
#define STT_HIOS 12
#define STT_LOPROC 13
#define STT_HIPROC 15

// Auxiliary vector types
#define AT_NULL 0
#define AT_IGNORE 1
#define AT_EXECFD 2
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define AT_BASE 7
#define AT_FLAGS 8
#define AT_ENTRY 9
#define AT_NOTELF 10
#define AT_UID 11
#define AT_EUID 12
#define AT_GID 13
#define AT_EGID 14
#define AT_L4_AUX 0xf0
#define AT_L4_ENV 0xf1

#ifdef __x86_64__

// Architecture
#define EM_X86_64 62

// Relocation types for x86-64
#define R_X86_64_NONE 0
#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_GOT32 3
#define R_X86_64_PLT32 4
#define R_X86_64_COPY 5
#define R_X86_64_GLOB_DAT 6
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE 8
#define R_X86_64_GOTPCREL 9
#define R_X86_64_32 10
#define R_X86_64_32S 11
#define R_X86_64_16 12
#define R_X86_64_PC16 13
#define R_X86_64_8 14
#define R_X86_64_PC8 15
#define R_X86_64_PC64 24
#define R_X86_64_GOTOFF64 25
#define R_X86_64_GOTPC32 26
#define R_X86_64_SIZE32 32
#define R_X86_64_SIZE64 33

// Architecture class definitions
#define EI_ARCH_CLASS ELFCLASS64
#define EI_ARCH_DATA ELFDATA2LSB
#define EI_ARCH_MACHINE EM_X86_64
#endif

typedef uint64_t elf64_addr;
typedef uint64_t elf64_off;

// ELF header
typedef struct [[gnu::packed]] {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    elf64_addr e_entry;
    elf64_off e_phoff;
    elf64_off e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_hdr;

// Program header
typedef struct [[gnu::packed]] {
    uint32_t p_type;
    uint32_t p_flags;
    elf64_off p_offset;
    elf64_addr p_vaddr;
    elf64_addr p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr;

// Section header
typedef struct [[gnu::packed]] {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    elf64_addr sh_addr;
    elf64_off sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} elf64_shdr;

// Symbol table entry
typedef struct [[gnu::packed]] {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    elf64_addr st_value;
    uint64_t st_size;
} elf64_sym;

// Dynamic table entry
typedef struct [[gnu::packed]] {
    int64_t d_tag;
    union {
        uint64_t d_val;
        elf64_addr d_ptr;
    } d_un;
} elf64_dyn;

// Relocation entry (without addend)
typedef struct [[gnu::packed]] {
    elf64_addr r_offset;
    uint64_t r_info;
} elf64_rel;

// Relocation entry (with addend)
typedef struct [[gnu::packed]] {
    elf64_addr r_offset;
    uint64_t r_info;
    int64_t r_addend;
} elf64_rela;

// Note header
typedef struct {
    uint64_t n_namesz;
    uint64_t n_descsz;
    uint64_t n_type;
} elf64_nhdr;

// Auxiliary vector
typedef struct {
    uint32_t atype;
    uint32_t avalue;
} elf64_auxv;

// ELF runtime info
typedef struct {
    elf64_addr at_entry;
    elf64_addr at_phdr;
    uint64_t at_phent;
    uint64_t at_phnum;
    char *ld_path;
} elf_info;



void *elf_section_get(void *elf, const char *name);
int64_t load_elf(struct process *process, int64_t handle, size_t base_address, elf_info *info);
#endif
