//
// Created by dustyn on 6/21/24.
//
#include "include/types.h"
#include "include/pmm.h"
#include "include/limine.h"
#include "stddef.h"
#include "include/mem.h"
#include "include/uart.h"

static inline bool

bitmap_get(void *bitmap, uint64 bit);

static inline void bitmap_set(void *bitmap, uint64 bit);

static inline void bitmap_clear(void *bitmap, uint64 bit);


__attribute__((used, section(".requests")))
volatile struct limine_memmap_request memmap_request = {
        .id = LIMINE_MEMMAP_REQUEST,
        .revision = 0,
};
__attribute__((used, section(".requests")))
volatile struct limine_hhdm_request hhdm_request = {
        .id = LIMINE_HHDM_REQUEST,
        .revision = 0
};

static inline bool bitmap_get(void *bitmap, uint64 bit) {
    uint8 *bitmap_byte = bitmap;
    return bitmap_byte[bit / 8] & (1 << (bit % 8));
}

static inline void bitmap_set(void *bitmap, uint64 bit) {
    uint8 *bitmap_byte = bitmap;
    bitmap_byte[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(void *bitmap, uint64 bit) {
    uint8 *bitmap_byte = bitmap;
    bitmap_byte[bit / 8] &= ~(1 << (bit % 8));
}


uint8 *mem_map= NULL;
uint64 highest_page_index = 0;
uint64 last_used_index = 0;
uint64 usable_pages = 0;
uint64 used_pages = 0;
uint64 reserved_pages = 0;


int phys_init() {
    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    struct limine_memmap_entry **entries = memmap->entries;
    uint64 highest_address = 0;

    for (uint64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                usable_pages += (entry->length + (PAGE_SIZE - 1)) / PAGE_SIZE;
                highest_address = highest_address > (entry->base + entry->length) ? highest_address : (entry->base +
                                                                                                       entry->length);
                break;

            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                reserved_pages += (entry->length + (PAGE_SIZE - 1)) / PAGE_SIZE;
                break;

            default:
                continue;
        }
    }

    highest_page_index = highest_address / PAGE_SIZE;
    uint64 bitmap_size = ((highest_page_index / 8) + (PAGE_SIZE - 1)) / PAGE_SIZE * PAGE_SIZE;

    for (uint64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        if (entry->length >= bitmap_size) {
            mem_map = (uint8 *) (entry->base + hhdm->offset);

            memset(mem_map, 0xFF, bitmap_size);

            entry->length -= bitmap_size;
            entry->base += bitmap_size;

            break;
        }
    }

    for (uint64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        for (uint64 j = 0; j < entry->length; j += PAGE_SIZE) {
            bitmap_clear(mem_map, (entry->base + j) / PAGE_SIZE);
        }
    }
    uint16 pages_mib = (((usable_pages * 4096) / 1024) / 1024);
    serial_printf("Physical Memory Init Complete. MiB found: %x.16\nReserved Pages : %x.16\nHighest Page Index : %x.16\n",pages_mib,reserved_pages,highest_page_index);
    return 0;
}

/*
 * Internal allocation function, just tries
 */
static void *__phys_alloc(uint64 pages, uint64 limit) {
    uint64 p = 0;

    while (last_used_index < limit) {
        if (!bitmap_get(mem_map, last_used_index++)) {
            if (++p == pages) {
                uint64 page = last_used_index - pages;
                for (uint64 i = page; i < last_used_index; i++) {
                    bitmap_set(mem_map, i);
                }
                return (void *) (page * PAGE_SIZE);
            }
        } else {
            p = 0;
        }
    }

    return NULL;
}

void *phys_alloc(uint64 pages) {
    uint64 last = last_used_index;
    void *return_value = __phys_alloc(pages, highest_page_index);
    if (return_value == NULL) {
        last_used_index = 0;
        return_value = __phys_alloc(pages, last);
    }
    used_pages += pages;
    return return_value;
}

void phys_dealloc(void *address, uint64 pages) {
    uint64 page = (uint64) address / PAGE_SIZE;
    for (uint64 i = page; i < page + pages; i++) {
        bitmap_clear(mem_map, i);
    }
    used_pages -= pages;
}