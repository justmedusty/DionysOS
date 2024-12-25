//
// Created by dustyn on 6/22/24.
//
#pragma once

#include <include/drivers/serial/uart.h>

#include "include/definitions/types.h"
#include "include/definitions/definitions.h"

#define NUM_SLABS 10
extern struct hash_table slab_hash;

struct slab{
    void **first_free;
    uint64_t entry_size;
    void *start_address;
    void *end_address;
} ;

/*
 * This will be used to create a more sophisticated data structure later
 */
struct header {
    struct slab *slab;
};

struct metadata {
    uint64_t pages;
    uint64_t size;
};

extern struct slab slabs[10];

static inline struct slab *heap_slab_for(uint64_t size) {
    for (uint64_t i = 0; i < (sizeof(slabs) / sizeof(slabs[0])); i++) {
        struct slab *slab = &slabs[i];

        if (slab->entry_size >= size) {
            return slab;
        }
    }

    return NULL;
}

int heap_init();
void heap_create_slab(struct slab *slab, uint64_t entry_size,uint64_t pages);
void *heap_allocate_from_slab(struct slab *slab);
void heap_free_in_slab(struct slab *slab, void *address);

