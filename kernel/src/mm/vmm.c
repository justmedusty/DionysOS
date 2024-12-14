//
// Created by dustyn on 6/21/24.
//
#pragma once
#include "include/mem/vmm.h"
#include "include/arch/arch_vmm.h"
#include "include/mem/kalloc.h"
struct virt_map* kernel_pg_map;

void arch_kvm_init(p4d_t *pgdir){
    map_kernel_address_space(pgdir);
}

void arch_vmm_init(){
    init_vmm();
}

void arch_map_pages(p4d_t* pgdir, uint64_t physaddr, uint64_t* va, const uint64_t perms, const uint64_t size) {
    map_pages(pgdir, physaddr, va, perms, size);
}

struct virtual_region* arch_create_region(const uint64_t start_address, const uint64_t size_pages,uint64_t type,uint64_t perms){
    struct virtual_region* virtual_region = (struct virtual_region*)kmalloc(sizeof(struct virtual_region));
    virtual_region->va = start_address;
    virtual_region->end_addr = start_address + (size_pages * PAGE_SIZE);
    virtual_region->ref_count = 1;
    virtual_region->num_pages = size_pages;
    virtual_region->flags = type;
}

void arch_attach_region(struct virt_map *map,struct virtual_region *region){

}

void arch_detach_region(struct virt_map *map,struct virtual_region *region){

}
