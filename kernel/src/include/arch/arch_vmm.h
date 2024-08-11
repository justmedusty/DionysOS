//
// Created by dustyn on 6/25/24.
//

#pragma once
#include "include/types.h"
#include "include/arch//arch_paging.h"

//walkpgdir flags , going to add a flag for debugging
#define ALLOC 0x1
#define DEBUG 0x2

static void native_flush_tlb_single(unsigned long vaddr) {
    asm volatile("invlpg (%0)" ::"r" (vaddr) : "memory");
}

static void native_flush_tlb_range(unsigned long vaddr,uint64 pages) {
    for(int i = 0; i < pages; i++){
        asm volatile("invlpg (%0)" ::"r" (vaddr + (i * PAGE_SIZE)) : "memory");
    }
}


extern char text_start[];
extern char text_end[];
extern char rodata_start[];
extern char rodata_end[];
extern char data_start[];
extern char data_end[];
extern char kernel_start[];
extern char kernel_end[];

void arch_init_vmm();
void arch_switch_page_table(p4d_t *page_dir);
int arch_map_pages(p4d_t *pgdir, uint64 physaddr, uint64 *va, uint64 perms,uint64 size);
uint64 arch_dealloc_va(p4d_t* pgdir, uint64 address);
void arch_dealloc_va_range(p4d_t* pgdir, uint64 address, uint64 size);
void arch_map_kernel_address_space(p4d_t* pgdir);