//
// Created by dustyn on 6/21/24.
//
#pragma once
#include "include/memory/vmm.h"

#include <include/data_structures/doubly_linked_list.h>

#include "include/architecture/arch_vmm.h"
#include "include/memory/kalloc.h"
struct virt_map* kernel_pg_map;

/*
 * Architecture agnostic map kernel address space function
 */
void arch_kvm_init(p4d_t *pgdir){
    map_kernel_address_space(pgdir);
}
/*
 * Architecture agnostic vmm init function
 */
void arch_vmm_init(){
    init_vmm();
}
/*
 * Architecture agnostic map pages function
 */
void arch_map_pages(p4d_t* pgdir, uint64_t physaddr, uint64_t* va, const uint64_t perms, const uint64_t size) {
    map_pages(pgdir, physaddr, va, perms, size);
}
/*
 * Creates a virtual memory region , takes size, start, type (stack heap memmap etc) , perms, contiguous (do one big contiguous range or a lot of single page allocs)
 *
 */
struct virtual_region* arch_create_region(const uint64_t start_address, const uint64_t size_pages, const uint64_t type, const uint64_t perms,const bool contiguous){
    struct virtual_region* virtual_region = (struct virtual_region*)kmalloc(sizeof(struct virtual_region));
    virtual_region->va = start_address;
    virtual_region->end_addr = start_address + (size_pages * PAGE_SIZE);
    virtual_region->ref_count = 1;
    virtual_region->num_pages = size_pages;
    virtual_region->flags = type;
    virtual_region->perms = perms;
    virtual_region->contiguous = contiguous;
    return virtual_region;
}
/*
 * Attach a passed region to a virt map structure. Will map it into memory
 */
void arch_attach_region(struct virt_map *map,struct virtual_region *region){
    doubly_linked_list_insert_tail(map->vm_regions,region);
    if (region->contiguous) {
        /*
         * Using kmalloc and removing the offset because kmalloc is locked phys_alloc is not
         * This is ugly and I will likely change it in the future
         */
        arch_map_pages(map->top_level,(uint64_t)kmalloc((uint64_t)V2P(region->num_pages)), (uint64_t *) region->va,region->perms,region->num_pages);
        return;
    }

    for (size_t i = 0; i < region->num_pages; i++) {
        map_pages(map->top_level,(uint64_t) kmalloc((uint64_t)V2P(1)),(uint64_t *) region->va + (i * PAGE_SIZE),region->perms,1);
    }

}
/*
 * detach a passed region to a virt map structure. Will free the region
 */
void arch_detach_region(struct virt_map *map,struct virtual_region *region){
    doubly_linked_list_remove_node_by_data_address(map->vm_regions,region);
    if (region->contiguous) {
        dealloc_va_range(map->top_level,region->va,region->num_pages);
        return;
    }

    for (size_t i = 0; i < region->num_pages; i++) {
        dealloc_va(map->top_level,region->va + (i * PAGE_SIZE));
    }
}
