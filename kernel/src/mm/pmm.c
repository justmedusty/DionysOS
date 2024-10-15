//
// Created by dustyn on 6/21/24.
//
#include "include/types.h"
#include "include/mem/pmm.h"

#include <include/definitions.h>
#include <include/arch/arch_cpu.h>
#include <include/data_structures/spinlock.h>
#include "limine.h"
#include "stddef.h"
#include "include/mem/mem.h"
#include "include/arch/arch_paging.h"
#include "include/drivers/uart.h"
#include "include/data_structures/binary_tree.h"

static inline bool bitmap_get(void* bitmap, uint64 bit);
static inline void bitmap_set(void* bitmap, uint64 bit);
static inline void bitmap_clear(void* bitmap, uint64 bit);


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

static inline bool bitmap_get(void* bitmap, uint64 bit) {
    uint8* bitmap_byte = bitmap;
    return bitmap_byte[bit / 8] & (1 << (bit % 8));
}

static inline void bitmap_set(void* bitmap, uint64 bit) {
    uint8* bitmap_byte = bitmap;
    bitmap_byte[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(void* bitmap, uint64 bit) {
    uint8* bitmap_byte = bitmap;
    bitmap_byte[bit / 8] &= (0 << (bit % 8));
}

struct spinlock pmm_lock;

struct binary_tree buddy_free_list_zone[15];

struct buddy_block buddy_block_static_pool[STATIC_POOL_SIZE]; // should be able to handle ~8 GB of memory in 2 << max order size blocks and the rest can be taken from a slab

uint8* mem_map = NULL;
uint64 highest_page_index = 0;
uint64 last_used_index = 0;
uint64 usable_pages = 0;
uint64 used_pages = 0;
uint64 reserved_pages = 0;
uint64 hhdm_offset = 0;
int page_range_index = 0;

int allocation_model;

//Need to handle sizing of this better but for now this should be fine statically allocating a semi-arbitrary amount I doubt there will be more than 10 page runs for this
struct contiguous_page_range contiguous_pages[10] = {};

int phys_init() {

    initlock(&pmm_lock, PMM_LOCK);
    struct limine_memmap_response* memmap = memmap_request.response;
    struct limine_hhdm_response* hhdm = hhdm_request.response;
    struct limine_memmap_entry** entries = memmap->entries;
    uint64 highest_address = 0;
    hhdm_offset = hhdm->offset;

    for (uint64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = entries[i];

        switch (entry->type) {
        case LIMINE_MEMMAP_USABLE:
            contiguous_pages[page_range_index].start_address = entry->base;
            contiguous_pages[page_range_index].end_address = entry->base + entry->length;
            contiguous_pages[page_range_index].pages = entry->length / PAGE_SIZE;
            page_range_index++;
            usable_pages += (entry->length + (PAGE_SIZE - 1)) / PAGE_SIZE;
            highest_address = highest_address > (entry->base + entry->length)
                                  ? highest_address
                                  : (entry->base +
                                      entry->length);
            break;

        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            reserved_pages += (entry->length + (PAGE_SIZE - 1)) / PAGE_SIZE;
            break;

        default:
            break;



        }
    }
    /*
     *  Quick bubble sort for the small list
     */

    int changes = 1;

    while (changes) {
        int local_changes = 0;
        for (int i = 0; i < page_range_index - 1; i++) {
            if (contiguous_pages[i].pages < contiguous_pages[i + 1].pages) {
                struct contiguous_page_range placeholder = contiguous_pages[i];
                memcpy(&contiguous_pages[i], &contiguous_pages[i + 1], sizeof(struct contiguous_page_range));
                memcpy(&contiguous_pages[i + 1], &placeholder, sizeof(struct contiguous_page_range));
                local_changes++;
            }
        }
        if (local_changes == 0) {
            changes = 0;
            for (int i = 0; i < page_range_index; i++) {
                serial_printf("Page range %i Start Address %x.64 End Address %x.64 Pages %i\n", i,contiguous_pages[i].start_address, contiguous_pages[i].end_address,contiguous_pages[i].pages);
            }
        }
    }

    /*
     *  Set up the buddy blocks which we will use later to replace our bitmap allocation scheme, reason being buddy
     *  makes for easier contiguous allocation whereas bitmap or freelist can get pretty out-of-order pretty quickly
     *
     */

    int index = 0; /* Index into the static buddy block pool */

    for(int i = 0; i < page_range_index; i++) {

        for (int j = 0; j < contiguous_pages[i].pages; j+= 1 << MAX_ORDER) {

            buddy_block_static_pool[index].start_address = contiguous_pages[i].start_address + (j * (PAGE_SIZE - 1));
            buddy_block_static_pool[index].flags |= STATIC_POOL_FLAG;
            buddy_block_static_pool[index].order = MAX_ORDER;
            buddy_block_static_pool[index].zone = i;
            buddy_block_static_pool[index].is_free = FREE;


            if(index != 0 && buddy_block_static_pool[index - 1].zone == buddy_block_static_pool[index].zone) {
                buddy_block_static_pool[index - 1].next =  &buddy_block_static_pool[index];
            }
            index++;
            if(index == STATIC_POOL_SIZE) {
                panic("1000 hit"); /* A panic is excessive but it's fine for now*/
            }

        }
        buddy_block_static_pool[index - 1].next = &buddy_block_static_pool[index]; /* Since the logic above will not apply for the last entry putting this here */

    }

    highest_page_index = highest_address / PAGE_SIZE;
    uint64 bitmap_size = ((highest_page_index / 8) + (PAGE_SIZE - 1)) / PAGE_SIZE * PAGE_SIZE;

    for (uint64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        if (entry->length >= bitmap_size) {
            mem_map = (uint8*)(entry->base + hhdm->offset);

            memset(mem_map, 0xFF, bitmap_size);

            entry->length -= bitmap_size;
            entry->base += bitmap_size;

            break;
        }
    }

    for (uint64 i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        for (uint64 j = 0; j < entry->length; j += PAGE_SIZE) {
            bitmap_clear(mem_map, (entry->base + j) / PAGE_SIZE);
        }
    }
    uint32 pages_mib = (((usable_pages * 4096) / 1024) / 1024);


    /*
     *  Set up buddy blocks and insert them into proper trees
     */
    index = 0;
    for (int i = 0; i < page_range_index; i++) {

        init_tree(&buddy_free_list_zone[i],REGULAR_TREE,0);
        while (buddy_block_static_pool[index].zone == i) {
            insert_tree_node(&buddy_free_list_zone[i],&buddy_block_static_pool[index],buddy_block_static_pool[index].order);
            index++;

        }
    }

    serial_printf("Physical memory mapped %i mb found\n", pages_mib);
    return 0;
}

/*
 * Internal allocation function
 */
static void* __phys_alloc(uint64 pages, uint64 limit) {
    uint64 p = 0;

    while (last_used_index < limit) {
        if (!bitmap_get(mem_map, last_used_index++)) {
            if (++p == pages) {
                uint64 page = last_used_index - pages;
                for (uint64 i = page; i < last_used_index; i++) {
                    bitmap_set(mem_map, i);
                }
                return (void*)(page * PAGE_SIZE);
            }
        }
        else {
            p = 0;
        }
    }

    return NULL;
}
void* phys_alloc(uint64 pages) {
    acquire_interrupt_safe_spinlock(&pmm_lock);
    uint64 last = last_used_index;
    void* return_value = __phys_alloc(pages, highest_page_index);
    if (return_value == NULL) {
        last_used_index = 0;
        return_value = __phys_alloc(pages, last);
    }
    used_pages += pages;
    if (return_value != NULL) {
        memset(P2V(return_value), 0,PAGE_SIZE * pages);
    }
    release_interrupt_safe_spinlock(&pmm_lock);
    return return_value;
}

void phys_dealloc(void* address, uint64 pages) {
    uint64 page = (uint64)address / PAGE_SIZE;
    for (uint64 i = page; i < page + pages; i++) {
        bitmap_clear(mem_map, i);
    }
    used_pages -= pages;
}


static uint64 buddy_alloc(uint64 pages) {

    if(pages < (1 << MAX_ORDER)){
    }
}

static void buddy_free(void* address, uint64 pages) {

}

static struct buddy_block* buddy_split(uint64 order,struct binary_tree *free_list) {

}