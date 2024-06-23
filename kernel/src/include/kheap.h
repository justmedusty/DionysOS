//
// Created by dustyn on 6/22/24.
//
#pragma once

#include "types.h"

typedef struct {
    void **first_free;
    uint64 entry_size;
} slab_t;

/*
 * This will be used to create a more sophisticated data structure later
 */
typedef struct {
    slab_t *slab;
} header;

typedef struct {
    uint64 pages;
    uint64 size;
} metadata_t;

extern slab_t slabs[15];

static inline slab_t *heap_slab_for(uint64 size) {
    for (uint64 i = 0; i < (sizeof(slabs) / sizeof(slabs[0])); i++) {
        slab_t *slab = &slabs[i];

        if (slab->entry_size >= size) {
            return slab;
        }
    }

    return NULL;
}

int heap_init();
void heap_create_slab(slab_t *slab, uint64 entry_size);
void *heap_allocate_from_slab(slab_t *slab);
void heap_free_in_slab(slab_t *slab, void *address);
void *kalloc(uint64 size);
void *krealloc(void *address, uint64 new_size);
void kfree(void *address);
