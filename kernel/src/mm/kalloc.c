//
// Created by dustyn on 7/6/24.
//
#include "include/types.h"
#include "include/mem/pmm.h"
#include "include/mem/kalloc.h"
#include "include/mem/slab.h"
#include "include/drivers/uart.h"
#include "include/mem/mem.h"
#include "include/arch/arch_paging.h"


/*
 * This is just using slab allocation for now but will incorporate other mechanisms in the future
 */

/*
 * Init Memory for Kernel Heap
 */
int heap_init() {
    int size = 8;
    for (uint64 i = 0; i < 9 ; i++) {

        if(size >= PAGE_SIZE){
            serial_printf("Entry too large , skipping slab alloc.\n");
            continue;
        }
        heap_create_slab(&slabs[i], size,1);
        size = (size << 1);

    }

    serial_printf("Kernel Heap Initialized\n");
    return 0;
}


void *kalloc(uint64 size) {

    if(size > PAGE_SIZE) {
        slab_t *slab = heap_slab_for(size);
        if (slab != NULL) {
            return heap_allocate_from_slab(slab);
        }
    }

    uint64 page_count = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;
    void *return_value = P2V(phys_alloc(page_count + 1));

    if (return_value == NULL) {
        return NULL;
    }

    metadata_t *metadata = (metadata_t *) return_value;

    metadata->pages = page_count;
    metadata->size = size;

    return return_value + PAGE_SIZE;
}

void *krealloc(void *address, uint64 new_size) {
    if (address == NULL) {
        return kalloc(new_size);
    }

    if (((uint64) address & 0xFFF) == 0) {
        metadata_t *metadata = (metadata_t *) (address - PAGE_SIZE);
        if (((metadata->size + (PAGE_SIZE - 1)) / PAGE_SIZE) == ((new_size + (PAGE_SIZE - 1)) / PAGE_SIZE)) {
            metadata->size = new_size;
            return address;
        }

        void *new_address = kalloc(new_size);
        if (new_address == NULL) {
            return NULL;
        }

        if (metadata->size > new_size) {
            memcpy(new_address, address, new_size);
        } else {
            memcpy(new_address, address, metadata->size);
        }

        kfree(address);
        return new_address;
    }

    header *slab_header = (header *) ((uint64) address & ~0xFFF);
    slab_t *slab = slab_header->slab;

    if (new_size > slab->entry_size) {
        void *new_address = kalloc(new_size);
        if (new_address == NULL) {
            return NULL;
        }

        memcpy(new_address, address, slab->entry_size);
        heap_free_in_slab(slab, address);
        return new_address;
    }

    return address;
}

void kfree(void *address) {
    if (address == NULL) {
        return;
    }

    if (((uint64) address & 0xFFF) == 0) {
        metadata_t *metadata = (metadata_t *) (address - PAGE_SIZE);
        phys_dealloc((void *) metadata - hhdm_request.response->offset, metadata->pages + 1);
        return;
    }

    header *slab_header = (header *) ((uint64) address & ~0xFFF);
    heap_free_in_slab(slab_header->slab, address);
}


