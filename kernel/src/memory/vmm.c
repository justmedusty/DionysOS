//
// Created by dustyn on 6/21/24.
//
#pragma once
#include "include/memory/vmm.h"

#include <include/data_structures/doubly_linked_list.h>
#include <include/drivers/display/framebuffer.h>

#include "include/architecture/arch_vmm.h"
#include "include/memory/kmalloc.h"
struct virt_map* kernel_pg_map;

/*
 * Architecture agnostic map kernel address space function
 */
void arch_kvm_init(p4d_t *pgdir){
    map_kernel_address_space(pgdir);
}

uint64_t *alloc_virtual_map() {
    return Virt2Phys(kzmalloc(PAGE_SIZE));
}

void free_virtual_map(uint64_t *virtual_map) {
    kfree(Phys2Virt(virtual_map));
}

uint64_t get_current_page_map() {
    return get_page_table();
}
/*
 * Architecture agnostic vmm init function
 */
void arch_vmm_init(){
    kprintf("Initializing Virtual Memory And Mapping Address Space...\n");
    init_vmm();
    kprintf("Virtual Memory Manager Initialized And Address Space Mapped\n");
}


void arch_dealloc_page_table(p4d_t *pgdir) {
    free_page_tables(pgdir);
}

/*
 * Architecture agnostic map pages function
 */
void arch_map_pages(p4d_t* pgdir, uint64_t physaddr, uint64_t* va, const uint64_t perms, const uint64_t size) {
    map_pages(pgdir, physaddr, va, perms, size);
}

void *arch_get_physical_address(void *virtual_address,uint64_t *page_map) {
    return (void *)(PTE_ADDR(*walk_page_directory(page_map,virtual_address,0)));
}

/*
 *  This will only support one foreign mapping for now
 */

void arch_map_foreign(p4d_t *user_page_table,uint64_t *va, uint64_t size) {
    uint64_t pages_mapped = 0;
    uint64_t page_map = get_current_page_map();
    uint64_t current_address = (uint64_t) KERNEL_FOREIGN_MAP_BASE;
    uint64_t virtual_address = (uint64_t) va;
    while (pages_mapped != size) {
        DEBUG_PRINT("FOREIGN MAP: PHYSICAL ADDR %x.64 VA %x.64 USER PAGE TABLE %x.64\n",arch_get_physical_address((void *)virtual_address,user_page_table),virtual_address,user_page_table);
        arch_map_pages((p4d_t *)page_map,(uint64_t) arch_get_physical_address((void *)virtual_address,user_page_table),(uint64_t *) current_address,READWRITE,PAGE_SIZE);
        current_address += PAGE_SIZE;
        pages_mapped++;
        virtual_address += PAGE_SIZE;
    }
}

void arch_unmap_foreign(uint64_t size) {
    dealloc_va_range((p4d_t *) get_current_page_map(),KERNEL_FOREIGN_MAP_BASE,size);
}
/*
 * Creates a virtual memory region , takes size, start, type (stack heap memmap etc) , perms, contiguous (do one big contiguous range or a lot of single page allocs)
 *
 *  For now I will not bother with including the physical backings, if I want to introduce paging or copying I will need to but for now I am fine not including all of the physical mappings
 */
struct virtual_region* create_region(const uint64_t start_address, const uint64_t size_pages, const uint64_t type, const uint64_t perms,const bool contiguous){

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
void attach_region(struct virt_map *map,struct virtual_region *region){
    if(map->vm_regions == NULL){
        map->vm_regions = kmalloc(sizeof(struct doubly_linked_list));
        doubly_linked_list_init(map->vm_regions);
    }
    doubly_linked_list_insert_tail(map->vm_regions,region);

    if(map->top_level == kernel_pg_map->top_level){
        /*
         * Kernel thread page maps are shared with the main kernel map aside from having independent stack regions, so we do not want to map anything here
         * just return.
         */
        return;
    }

    if (region->contiguous) {
        /*
         * Using kmalloc and removing the offset because kmalloc is locked phys_alloc is not
         * This is ugly and I will likely change it in the future
         */
        arch_map_pages(map->top_level,(uint64_t)kmalloc((uint64_t)Virt2Phys(region->num_pages)), (uint64_t *) region->va,region->perms,region->num_pages);
        return;
    }

    for (size_t i = 0; i < region->num_pages; i++) {
        map_pages(map->top_level,(uint64_t) kmalloc((uint64_t)Virt2Phys(1)),(uint64_t *) region->va + (i * PAGE_SIZE),region->perms,1);
    }

}

/*
 * detach a passed region to a virt map structure. Will free the region
 */
void detach_region(struct virt_map *map,struct virtual_region *region){
    doubly_linked_list_remove_node_by_data_address(map->vm_regions,region);
    if (region->contiguous) {
        dealloc_va_range(map->top_level,region->va,region->num_pages);
        return;
    }

    for (size_t i = 0; i < region->num_pages; i++) {
        dealloc_va(map->top_level,region->va + (i * PAGE_SIZE));
    }
}



void attach_user_region(struct virt_map *map,struct virtual_region *region){
    if(map->vm_regions == NULL){
        map->vm_regions = kmalloc(sizeof(struct doubly_linked_list));
        doubly_linked_list_init(map->vm_regions);
    }
    doubly_linked_list_insert_tail(map->vm_regions,region);

    if (region->contiguous) {
        /*
         * Using kmalloc and removing the offset because kmalloc is locked phys_alloc is not
         * This is ugly and I will likely change it in the future
         */
        arch_map_pages(map->top_level,(uint64_t)umalloc((uint64_t)(region->num_pages)), (uint64_t *) region->va,region->perms,region->num_pages);
        return;
    }

    for (size_t i = 0; i < region->num_pages; i++) {
        map_pages(map->top_level,(uint64_t) umalloc((uint64_t)(1)),(uint64_t *) region->va + (i * PAGE_SIZE),region->perms,1);
    }

}

void detach_user_region(struct virt_map *map,struct virtual_region *region){
    doubly_linked_list_remove_node_by_data_address(map->vm_regions,region);
    if (region->contiguous) {
        dealloc_user_va_range(map->top_level,region->va,region->num_pages);
        return;
    }

    for (size_t i = 0; i < region->num_pages; i++) {
        dealloc_user_va(map->top_level,region->va + (i * PAGE_SIZE));
    }
}
