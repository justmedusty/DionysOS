//
// Created by dustyn on 6/25/24.
//

/*
 * Taking inspo from xv6 to keep things as readable and simple as possible here.
 */
#include <include/arch/arch_cpu.h>
#include "include/mem/vmm.h"
#include "include/types.h"
#include "include/mem/mem.h"
#include "include/arch//arch_paging.h"
#include "include/mem/pmm.h"
#include "include/mem/kalloc.h"
#include "include/mem/mem_bounds.h"
#include "include/drivers/uart.h"

#include "include/arch//arch_vmm.h"
#include "include/arch/x86_64/asm_functions.h"


p4d_t* global_pg_dir = 0;

void arch_switch_page_table(p4d_t* page_dir){
    lcr3((uint64)(page_dir));
}

void arch_init_vmm(){
    kernel_pg_map = kalloc(PAGE_SIZE);
    kernel_pg_map->top_level = (uint64*)phys_alloc(1);
    kernel_pg_map->vm_region_head = kalloc(PAGE_SIZE);
    kernel_pg_map->vm_region_head->next = kernel_pg_map->vm_region_head;
    kernel_pg_map->vm_region_head->prev = kernel_pg_map->vm_region_head;

    /*
     * Map symbols in the linker script
     */
    arch_map_kernel_address_space(kernel_pg_map->top_level);


    serial_printf("Kernel page table built in table located at %x.64\n", kernel_pg_map->top_level);
    serial_printf("VMM mapped and initialized\n");
}

void arch_reload_vmm() {
    arch_switch_page_table(kernel_pg_map->top_level);
}

void arch_map_kernel_address_space(p4d_t* pgdir){

    if (arch_map_pages(pgdir, (uint64)(text_start - kernel_min) + kernel_phys_min, (uint64*)text_start, 0,
                  text_end - text_start) == -1){
        panic("Mapping text!");
    }


    if (arch_map_pages(pgdir, (uint64)(rodata_start - kernel_min) + kernel_phys_min, (uint64*)rodata_start, PTE_NX,
                  rodata_end - rodata_start) == -1){
        panic("Mapping rodata!");
    }


    if (arch_map_pages(pgdir, (uint64)(data_start - kernel_min) + kernel_phys_min, (uint64*)data_start,
                  PTE_NX | PTE_RW, data_end - data_start) == -1){
        panic("Mapping data!");
    }
    /*
     * Map the first 4gb to the higher va space for the kernel, for the hhdm mapping
     */
    if (arch_map_pages(pgdir, 0, 0 + (uint64*)hhdm_offset, PTE_RW | PTE_NX, 0x100000000) == -1){
        panic("Mapping first 4gb!");
    }

    struct limine_memmap_response* memmap = memmap_request.response;
    struct limine_memmap_entry** entries = memmap->entries;

    for (uint64 i = 0; i < memmap->entry_count; i++){
        uint64 base = PGROUNDDOWN(entries[i]->base);
        uint64 top = PGROUNDUP(entries[i]->base + entries[i]->length);
        if (top < 0x100000000){
            continue;
        }
        for (uint64 j = base; j < top; j += PAGE_SIZE){
            if (j < (uint64)0x100000000){
                continue;
            }
            if (arch_map_pages(kernel_pg_map->top_level, j, (uint64*)j + hhdm_offset, PTE_NX | PTE_RW, PAGE_SIZE) == -1){
                panic("hhdm mapping");
            }
        }
    }
}

/*
 * Walk a page directory to find a physical address, or allocate all the way down if the alloc flag is set
 */

static pte_t* walkpgdir(p4d_t* pgdir, const void* va, int flags){
    if (pgdir == 0){
        panic("page dir zero");
    }

    p4d_t p4d_idx = P4DX(va);
    pud_t pud_idx = PUDX(va);
    pmd_t pmd_idx = PMDX(va);
    pte_t pte_idx = PTX(va);

    pud_t* pud = P2V(pgdir);
    pud += p4d_idx;

    if (!(*pud & PTE_P)){
        if (flags & ALLOC){
            *pud = (pud_t)phys_alloc(1);
            *pud |= PTE_P | PTE_RW | PTE_U;
        }
        else{
            return 0;
        }
    }

    pmd_t* pmd = P2V(PTE_ADDR(*pud));
    pmd += pud_idx;

    if (!(*pmd & PTE_P)){
        if (flags & ALLOC){
            *pmd = (pmd_t)phys_alloc(1);
            *pmd |= PTE_P | PTE_RW | PTE_U;
        }
        else{
            return 0;
        }
    }

    pte_t* pte = P2V(PTE_ADDR(*pmd));
    pte += pmd_idx;


    if (!(*pte & PTE_P)){
        if (flags & ALLOC){
            *pte = (pte_t)phys_alloc(1);
            *pte |= PTE_P | PTE_RW | PTE_U;
        }
        else{
            return 0;
        }
    }

    pte_t* entry = P2V(PTE_ADDR(*pte));
    entry += pte_idx;

    return entry;
}

/*
 * Maps pages from VA/PA to size in page size increments.
 */
int arch_map_pages(p4d_t* pgdir, uint64 physaddr, uint64* va, uint64 perms, uint64 size){
    pte_t* pte;
    uint64 address = PGROUNDDOWN((uint64) va);
    uint64 last = PGROUNDUP(((uint64) va) + size - 1);

    for (;;){
        if ((pte = walkpgdir(pgdir, (void*)address, 1)) == 0){
            return -1;
        }

        if ((perms & PTE_U) && *pte & PTE_P){
            panic("remap");
        }

        *pte = physaddr | perms | PTE_P;

        if (address == last){
            break;
        }

        address += PAGE_SIZE;
        physaddr += PAGE_SIZE;
    }

    return 0;
}


uint64 arch_dealloc_va(p4d_t* pgdir, uint64 address){
    uint64 aligned_address = ALIGN_DOWN(address, PAGE_SIZE);
    pte_t* entry = walkpgdir(pgdir, (void*)aligned_address, 0);

    if (entry == 0){
        panic("");
        return 0;
    }
    if (*entry & PTE_P){
        phys_dealloc((void*)PTE_ADDR(*entry), 1);
        *entry = 0;
        native_flush_tlb_single(aligned_address);

        return 1;
    }

    return 0;
}

void arch_dealloc_va_range(p4d_t* pgdir, uint64 address, uint64 size){
    uint64 aligned_size = ALIGN_UP(size, PAGE_SIZE);
    serial_printf("Aligned size %x.64\n", aligned_size);
    for (uint64 i = 0; i <= aligned_size; i += PAGE_SIZE){
        arch_dealloc_va(pgdir, address + i);
    }
}



