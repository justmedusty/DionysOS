//
// Created by dustyn on 6/25/24.
//

#pragma once

#include "include/definitions/types.h"
#include "include/architecture/arch_paging.h"

#define USER_SPAN_SIZE (((highest_address / 2)) + ((highest_address) / 4)) // how much of memory is going to be assigned to the user pool

enum {
    ALLOC = 1,
    DEBUG = 2
};

#ifdef __x86_64__

static void native_flush_tlb_single(unsigned long vaddr) {
    asm volatile("invlpg (%0)"::"r" (vaddr) : "memory");
}

static void native_flush_tlb_range(unsigned long vaddr, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++) {
        asm volatile("invlpg (%0)"::"r" (vaddr + (i * PAGE_SIZE)) : "memory");
    }
}

#endif

extern char text_start[];
extern char text_end[];
extern char rodata_start[];
extern char rodata_end[];
extern char data_start[];
extern char data_end[];
extern char kernel_start[];
extern char kernel_end[];

void init_vmm();

void switch_page_table(p4d_t *page_dir);

int map_pages(p4d_t *pgdir, uint64_t physaddr, const uint64_t *va, uint64_t perms, uint64_t size);

uint64_t dealloc_va(p4d_t *pgdir, uint64_t address);

void dealloc_va_range(p4d_t *pgdir, uint64_t address, uint64_t size);

void map_kernel_address_space(p4d_t *pgdir);

void reload_vmm();