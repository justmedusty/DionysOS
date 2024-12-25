//
// Created by dustyn on 6/21/24.
//
#ifndef _VMM_H_
#define _VMM_H_
#pragma once
#include <stdbool.h>

#include "include/definitions/types.h"
extern struct virt_map* kernel_pg_map;

/*
 * x = value, y = align by
 */

#define KERNEL_MEM 0x100000000
#define DIV_ROUND_UP(x,y) (x + (y -1)) / y
#define ALIGN_UP(x,y) DIV_ROUND_UP(x,y) * y
#define ALIGN_DOWN(x,y) (x / y) * y

enum region_type {
    STACK,
    HEAP,
    TEXT,
    FILE
};
struct virtual_region {
    uint64_t va;
    uint64_t pa;
    uint64_t end_addr;
    uint64_t num_pages;
    uint64_t flags;
    uint64_t ref_count;
    uint64_t perms;
    bool contiguous; // if not contiguous we go page by page
};

struct virt_map {
    uint64_t *top_level;
    struct doubly_linked_list *vm_regions;
};



void arch_kvm_init(p4d_t *pgdir);
void arch_vmm_init();
struct virtual_region* arch_create_region(const uint64_t start_address, const uint64_t size_pages,uint64_t type,uint64_t perms,bool contiguous);
void arch_attach_region(struct virt_map *map,struct virtual_region *region);
void arch_detach_region(struct virt_map *map,struct virtual_region *region);
void arch_dealloc_page_table(p4d_t* pgdir);

#endif