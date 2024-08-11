//
// Created by dustyn on 6/21/24.
//
#pragma once
#include "include/types.h"
extern struct virt_map* kernel_pg_map;

/*
 * x = value, y = align by
 */
#define DIV_ROUND_UP(x,y) (x + (y -1)) / y
#define ALIGN_UP(x,y) DIV_ROUND_UP(x,y) * y
#define ALIGN_DOWN(x,y) (x / y) * y

typedef struct virtual_region {
    uint64 va;
    uint64 pa;
    uint64 end_addr;
    uint64 num_pages;
    uint64 flags;

    uint64 ref_count;

    struct virtual_region* next;
    struct virtual_region* prev;

} virtual_region;

typedef struct virt_map {
    uint64 *top_level;
    struct virtual_region *vm_region_head;
};



void kvm_init(p4d_t *pgdir);
void vmm_init();
struct vm_region* create_region();
void attatch_region(struct virt_map *);
void detatch_region(struct virt_map *);
