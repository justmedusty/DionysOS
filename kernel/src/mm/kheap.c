//
// Created by dustyn on 6/22/24.
//
#include <stdint.h>
#include <stddef.h>
#include "include/kheap.h"
#include "include/pmm.h"
#include "include/mem.h"
#include "include/uart.h"

//Kernel heap
slab_t slabs[10];

/*
 * Create a slab of physical memory,
 */
void heap_create_slab(slab_t *slab, uint64 entry_size) {

    slab->first_free = phys_alloc(1) + hhdm_request.response->offset;
    slab->entry_size = entry_size;

    uint64 header_offset = (sizeof(header) + (entry_size - 1)) / entry_size * entry_size;
    uint64 available_size = PAGE_SIZE - header_offset;
    header *slab_pointer = (header *) slab->first_free;

    slab_pointer->slab = slab;
    slab->first_free = (void **) ((void *) slab->first_free + header_offset);

    void **array = (void **) slab->first_free;

    uint64 max = available_size / entry_size - 1;
    uint64 fact = entry_size / sizeof(void *);

    for (uint64 i = 0; i < max; i++) {
        array[i * fact] = &array[(i + 1) * fact];
    }
    array[max * fact] = NULL;
}

void *heap_allocate_from_slab(slab_t *slab) {
    if (slab->first_free == NULL) {
        heap_create_slab(slab, slab->entry_size);
    }

    void **old_free = slab->first_free;
    slab->first_free = *old_free;
    memset(old_free, 0, slab->entry_size);

    return old_free;
}

/*
 * Free part of a slab, setting it new head to the first_free field
 * This can be useful
 */
void heap_free_in_slab(slab_t *slab, void *address) {
    if (address == NULL) {
        return;
    }
    void **new_head = address;
    *new_head = slab->first_free;
    slab->first_free = new_head;
}

/*
 * Init Memory for Kernel Heap
 */
int heap_init() {
    int size = 8;
    for (uint64 i = 0; i < 9 ; i++) {

        if(size >= PAGE_SIZE){
            write_string_serial("Entry too large , skipping slab alloc.\n");
            continue;
        }
        heap_create_slab(&slabs[i], size);
        // i *= 2
        size = (size << 1);

        serial_printf("Allocated slab of size %x\n",size);
    }

    write_string_serial("Kernel Heap Initialized\n");
    return 0;
}


void *kalloc(uint64 size) {
    slab_t *slab = heap_slab_for(size);
    if (slab != NULL) {
        return heap_allocate_from_slab(slab);
    }

    uint64 page_count = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;
    void *return_value = phys_alloc(page_count + 1);

    if (return_value == NULL) {
        return NULL;
    }

    return_value += hhdm_request.response->offset;
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