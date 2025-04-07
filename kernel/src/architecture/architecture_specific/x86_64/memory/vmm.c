//
// Created by dustyn on 6/25/24.
//

/*
 * Taking inspo from xv6 to keep things as readable and simple as possible here.
 */


#include <include/drivers/display/framebuffer.h>

#ifdef __x86_64__

#include <include/architecture/arch_cpu.h>
#include "include/memory/vmm.h"
#include "include/definitions/types.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_paging.h"
#include "include/memory/pmm.h"
#include "include/memory/kalloc.h"
#include "include/memory/mem_bounds.h"
#include "include/drivers/serial/uart.h"
#include "include/architecture//arch_vmm.h"
#include "include/architecture/x86_64/asm_functions.h"
#include "include/architecture/x86_64/msr.h"

#define FOUR_GB 0x100000000


p4d_t *global_pg_dir = 0;


void switch_page_table(p4d_t *page_dir) {
    lcr3((uint64_t) (page_dir));
}

void init_vmm() {
    kernel_pg_map = kmalloc(PAGE_SIZE);
    kernel_pg_map->top_level = V2P(kzmalloc(PAGE_SIZE));
    kernel_pg_map->vm_regions = NULL;

    /*
     * Map symbols in the linker script
     */
    map_kernel_address_space(kernel_pg_map->top_level);


    serial_printf("Kernel page table built in table located at %x.64\n", kernel_pg_map->top_level);
    switch_page_table(kernel_pg_map->top_level);
    serial_printf("VMM mapped and initialized\n");
}

void reload_vmm() {
    switch_page_table(kernel_pg_map->top_level);
}

void map_kernel_address_space(p4d_t *pgdir) {

    if (map_pages(pgdir, (uint64_t) (text_start - kernel_min) + kernel_phys_min, (uint64_t *) text_start, 0,
                  text_end - text_start) == -1) {
        panic("Mapping text!");
    }


    if (map_pages(pgdir, (uint64_t) (rodata_start - kernel_min) + kernel_phys_min, (uint64_t *) rodata_start, PTE_NX,
                  rodata_end - rodata_start) == -1) {
        panic("Mapping rodata!");
    }


    if (map_pages(pgdir, (uint64_t) (data_start - kernel_min) + kernel_phys_min, (uint64_t *) data_start,
                  PTE_NX | PTE_RW, data_end - data_start) == -1) {
        panic("Mapping data!");
    }
    /*
     * Map the first 4 GB since there is important stuff there
     */
    if (map_pages(pgdir, 0, (uint64_t *) ((uint64_t) 0 + (uint64_t) hhdm_offset), PTE_RW | PTE_NX,
                  FOUR_GB) == -1) {
        panic("Mapping First four gb!");
    }

    /*
     * Map only the end of user physical memory up to the highest address so the the kernel does not need to map all of user memory
     */
    if (map_pages(pgdir, highest_user_phys_addr,
                  (uint64_t *) ((uint64_t) highest_user_phys_addr + (uint64_t) hhdm_offset), PTE_RW | PTE_NX,
                  highest_address - highest_user_phys_addr) == -1) {
        panic("Mapping address space!");
    }
}

/*
 * Walk a page directory to find a physical address, or allocate all the way down if the alloc flag is set
 */

pte_t *walk_page_directory(p4d_t *pgdir, const void *va, const int flags) {
    if (pgdir == 0) {
        panic("page dir zero");
    }

    p4d_t p4d_idx = P4DX(va);
    pud_t pud_idx = PUDX(va);
    pmd_t pmd_idx = PMDX(va);
    pte_t pte_idx = PTX(va);

    pud_t *pud = P2V(pgdir);
    pud += p4d_idx;

    if (!(*pud & PTE_P)) {
        if (flags & ALLOC) {
            *pud = (pud_t) V2P(kzmalloc(PAGE_SIZE));
            *pud |= PTE_P | PTE_RW;
        } else {
            return 0;
        }
    }

    pmd_t *pmd = P2V(PTE_ADDR(*pud));
    pmd += pud_idx;
    if (!(*pmd & PTE_P)) {
        if (flags & ALLOC) {
            *pmd = (pmd_t) V2P(kzmalloc(PAGE_SIZE));
            *pmd |= PTE_P | PTE_RW;
        } else {
            return 0;
        }
    }

    pte_t *pte = P2V(PTE_ADDR(*pmd));
    pte += pmd_idx;

    if (!(*pte & PTE_P)) {
        if (flags & ALLOC) {
            *pte = (pte_t) V2P(kzmalloc(PAGE_SIZE));
            *pte |= PTE_P | PTE_RW;
        } else {
            return 0;
        }
    }

    pte_t *entry = P2V(PTE_ADDR(*pte));
    entry += pte_idx;
    return entry;
}

/*
 * Maps pages from VA/PA to size in page size increments.
 */
int map_pages(p4d_t *pgdir, uint64_t physaddr, const uint64_t *va, const uint64_t perms, const uint64_t size) {
    pte_t *pte;
    uint64_t address = PGROUNDDOWN((uint64_t) va);
    uint64_t last = PGROUNDUP(((uint64_t) va) + size - 1);

    for (;;) {
        if ((pte = walk_page_directory(pgdir, (void *) address, ALLOC)) == 0) {
            return -1;
        }

        if ((perms & PTE_U) && *pte & PTE_P) {
            panic("remap");
        }

        *pte = physaddr | perms | PTE_P;

        if (address == last) {
            break;
        }

        address += PAGE_SIZE;
        physaddr += PAGE_SIZE;
    }

    return 0;
}


uint64_t dealloc_va(p4d_t *pgdir, const uint64_t address) {
    uint64_t aligned_address = ALIGN_DOWN(address, PAGE_SIZE);
    pte_t *entry = walk_page_directory(pgdir, (void *) aligned_address, 0);

    if (entry == 0) {
        panic("dealloc_va");
        return 0;
    }
    if (*entry & PTE_P) {
        phys_dealloc((void *) PTE_ADDR(*entry));
        *entry = 0;
        native_flush_tlb_single(aligned_address);
        return 1;
    }

    return 0;
}

void dealloc_va_range(p4d_t *pgdir, const uint64_t address, const uint64_t size) {
    uint64_t aligned_size = ALIGN_UP(size, PAGE_SIZE);
    serial_printf("Aligned size %x.64\n", aligned_size);
    for (uint64_t i = 0; i <= aligned_size; i += PAGE_SIZE) {
        dealloc_va(pgdir, address + i);
    }
}

void arch_dealloc_page_table(p4d_t *pgdir) {
    dealloc_va_range(pgdir, 0, 0xFFFFFFFFFFFFFFFF & ~0xFFF);
}

void setup_pat() {
    uint64_t pat =
            (0ULL << 0) | (1ULL << 8) | (2ULL << 16) | (3ULL << 24) | (4ULL << 32) | (5ULL << 40) | (6ULL << 48) |
            (7ULL << 56);

    wrmsr(PAT_MSR, pat);
}

#endif


