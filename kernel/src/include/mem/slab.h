//
// Created by dustyn on 6/22/24.
//
#pragma once

#include <include/drivers/serial/uart.h>

#include "include/types.h"
#include "include/definitions.h"

#define SLAB_HASH_SIZE 300
extern struct hash_table slab_hash;

typedef struct {
    void **first_free;
    uint64_t entry_size;
    void *start_address;
    void *end_address;
} slab_t;

/*
 * This will be used to create a more sophisticated data structure later
 */
typedef struct {
    slab_t *slab;
} header;

typedef struct {
    uint64_t pages;
    uint64_t size;
} metadata_t;

extern slab_t slabs[10];

static inline slab_t *heap_slab_for(uint64_t size) {
    for (uint64_t i = 0; i < (sizeof(slabs) / sizeof(slabs[0])); i++) {
        slab_t *slab = &slabs[i];

        if (slab->entry_size >= size) {
            return slab;
        }
    }

    return NULL;
}

int heap_init();
void heap_create_slab(slab_t *slab, uint64_t entry_size,uint64_t pages);
void *heap_allocate_from_slab(slab_t *slab);
void heap_free_in_slab(slab_t *slab, void *address);

