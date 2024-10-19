//
// Created by dustyn on 6/21/24.
//
#include "include/types.h"
#include "include/mem/pmm.h"
#include <include/definitions.h>
#include <include/arch/arch_cpu.h>
#include <include/data_structures/hash_table.h>
#include <include/data_structures/spinlock.h>
#include <include/mem/kalloc.h>
#include "limine.h"
#include "include/mem/mem.h"
#include "include/arch/arch_paging.h"
#include "include/drivers/uart.h"
#include "include/data_structures/binary_tree.h"

static inline bool bitmap_get(void* bitmap, uint64 bit);
static inline void bitmap_set(void* bitmap, uint64 bit);
static inline void bitmap_clear(void* bitmap, uint64 bit);

static void buddy_coalesce(struct buddy_block* block);
static struct buddy_block* buddy_alloc(uint64 pages);
static void buddy_free(void* address);
static void buddy_block_free(struct buddy_block* block);
static struct buddy_block* buddy_block_get();
static struct buddy_block* buddy_split(struct buddy_block* block);

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
struct spinlock buddy_lock;

uint64 zone_pointer; /* will just use one zone until it's all allocated then increment the zone pointer */
struct binary_tree buddy_free_list_zone[PHYS_ZONE_COUNT];

struct buddy_block buddy_block_static_pool[STATIC_POOL_SIZE];
// should be able to handle ~8 GB of memory in 2 << max order size blocks and the rest can be taken from a slab

struct singly_linked_list unused_buddy_blocks_list;

struct hash_table used_buddy_hash_table;

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
    initlock(&buddy_lock, BUDDY_LOCK);
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
                serial_printf("Page range %i Start Address %x.64 End Address %x.64 Pages %i\n", i,
                              contiguous_pages[i].start_address, contiguous_pages[i].end_address,
                              contiguous_pages[i].pages);
            }
        }
    }

    /*
     *  Set up the buddy blocks which we will use later to replace our bitmap allocation scheme, reason being buddy
     *  makes for easier contiguous allocation whereas bitmap or freelist can get pretty out-of-order pretty quickly
     *
     */

    int index = 0; /* Index into the static buddy block pool */

    for (int i = 0; i < page_range_index; i++) {
        for (int j = 0; j < contiguous_pages[i].pages; j += 1 << MAX_ORDER) {
            buddy_block_static_pool[index].start_address = contiguous_pages[i].start_address + (j * (PAGE_SIZE));
            buddy_block_static_pool[index].flags |= STATIC_POOL_FLAG;
            buddy_block_static_pool[index].order = MAX_ORDER;
            buddy_block_static_pool[index].zone = i;
            buddy_block_static_pool[index].is_free = FREE;


            if (index != 0 && buddy_block_static_pool[index - 1].zone == buddy_block_static_pool[index].zone) {
                buddy_block_static_pool[index - 1].next = &buddy_block_static_pool[index];
            }
            index++;
            if (index == STATIC_POOL_SIZE) {
                panic("Phys_init : Static pool size hit"); /* A panic is excessive but it's fine for now*/
            }
        }
        buddy_block_static_pool[index - 1].next = &buddy_block_static_pool[index];
        /* Since the logic above will not apply for the last entry putting this here */
    }

    singly_linked_list_init(&unused_buddy_blocks_list);

    for (uint64 i = index; i < STATIC_POOL_SIZE; i++) {
        buddy_block_static_pool[i].is_free = FREE;
        buddy_block_static_pool[i].zone = 0xFFFF;
        buddy_block_static_pool[i].next = NULL;
        buddy_block_static_pool[i].zone = 0xFFFFFFFF;
        buddy_block_static_pool[i].start_address = 0;

        singly_linked_list_insert_head(&unused_buddy_blocks_list, &buddy_block_static_pool[i]);
    }

    /*
     *  Set up buddy blocks and insert them into proper trees
     */
    index = 0;
    init_tree(&buddy_free_list_zone[0],REGULAR_TREE, 0);
    for (int i = 0; i < page_range_index; i++) {
        while (buddy_block_static_pool[index].zone == i ) {
            insert_tree_node(&buddy_free_list_zone[0], &buddy_block_static_pool[index],
                             buddy_block_static_pool[index].order);
            index++;
        }
    }

    hash_table_init(&used_buddy_hash_table,BUDDY_HASH_TABLE_SIZE);

    serial_printf("%i free block objects\n", unused_buddy_blocks_list.node_count);

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
/*
    uint64 last = last_used_index;

    void* return_value = __phys_alloc(pages, highest_page_index);

    if (return_value == NULL) {
        last_used_index = 0;
        return_value = __phys_alloc(pages, last);
    }

    used_pages += pages;

    // This is unnecessarily heavy just pinning that for later
    if (return_value != NULL) {
        memset(P2V(return_value), 0,PAGE_SIZE * pages);
    }
*/
    serial_printf("%i pages\n",pages);
    struct buddy_block *block = buddy_alloc(pages);

    void* return_value;

    if(block == NULL) {
        block = buddy_alloc(pages);
        panic("phys_alloc cannot allocate");
    }
    block->is_free = FALSE;
    return_value = (void*)block->start_address;
    release_interrupt_safe_spinlock(&pmm_lock);

    return return_value;
}

void phys_dealloc(void* address, uint64 pages) {
    /*
    uint64 page = (uint64)address / PAGE_SIZE;
    for (uint64 i = page; i < page + pages; i++) {
        bitmap_clear(mem_map, i);
    }
    used_pages -= pages;
    */

    buddy_free(address);

}


static struct buddy_block* buddy_alloc(uint64 pages) {
    if (pages > (1 << MAX_ORDER)) {
        serial_printf("pages %i \n",pages);
        //TODO Handle tall orders
    }
    for (uint64 i = 0; i < MAX_ORDER; i++) {
        if (pages == (1 << i)) {
            struct buddy_block* block = lookup_tree(&buddy_free_list_zone[zone_pointer], i,REMOVE_FROM_TREE);
            if (block == NULL) {
                uint64 index = i;
                index++;
                while(index <= MAX_ORDER) {
                    block = lookup_tree(&buddy_free_list_zone[zone_pointer], index,REMOVE_FROM_TREE);
                    if(block != NULL) {

                        while(block->order != i) {
                            block = buddy_split(block);
                        }
                        hash_table_insert(&used_buddy_hash_table,block->start_address,block);
                        return block;

                    }
                    index++;
                }
            }
            return block;
        }

        if (pages < (1 << i)) {
            struct buddy_block* block = lookup_tree(&buddy_free_list_zone[zone_pointer], 1 << i,REMOVE_FROM_TREE);

            if (block == NULL) {
                return NULL;
            }
            if (buddy_split(block) != NULL) {
                hash_table_insert(&used_buddy_hash_table,block->start_address,block);
                return block;
            }

            return NULL;
        }
    }

    return NULL;
}

/* It may be ideal to store pointers in the process object and pass the process object to easily find the block*/
/* Pages is also sort of redundant here but will keep it for now */

static void buddy_free(void* address) {
    struct singly_linked_list* bucket = hash_table_retrieve(&used_buddy_hash_table,hash((uint64)address, BUDDY_HASH_TABLE_SIZE));

    if (bucket == NULL) {
        panic("Buddy Dealloc: Buddy hash table not found"); /* Shouldn't ever happen so panicking for visibility if it does happen */
    }

    struct singly_linked_list_node* node = bucket->head;

    while (1) {
        if (node == NULL) {
            panic("Buddy Dealloc: Hash returned bucket without result"); /* This shouldn't happen */
        }
        struct buddy_block* block = node->data;

        if (block->start_address == (uint64)address) {
            singly_linked_list_remove_node_by_address(bucket, address);
            block->is_free = FREE;
            buddy_coalesce(block);
        }

        node = node->next;
    }
}

static struct buddy_block* buddy_split(struct buddy_block* block) {
    if (block->order == 0) {
        return NULL; /* Can't split a zero order */
    }

    if (block->is_free != FREE) {
        return NULL; /* Obviously */
    }

    struct buddy_block* new_block = buddy_block_get();

    if (new_block == NULL) {
        return NULL;
    }

    new_block->next = block->next;
    block->next = new_block;
    block->order--;
    new_block->order = block->order;
    new_block->start_address = block->start_address + (1 << new_block->order);
    new_block->zone = block->zone;
    new_block->is_free = FREE;

    const uint64 ret = insert_tree_node(&buddy_free_list_zone[0], new_block, new_block->order);

    if (ret == SUCCESS) {
        return block;
    }

    return NULL;
}

static struct buddy_block* buddy_block_get() {
    struct buddy_block* block = (struct buddy_block*)singly_linked_list_remove_head(&unused_buddy_blocks_list);
    if (block == NULL) {
        return kalloc(sizeof(struct buddy_block));
    }
    return block;
}


static void buddy_block_free(struct buddy_block* block) {
    if (block->flags & STATIC_POOL_FLAG) {
        block->is_free = UNUSED;
        block->order = UNUSED;
        block->zone = UNUSED;
        block->start_address = 0;
        block->next = NULL;
        singly_linked_list_insert_head(&unused_buddy_blocks_list, block);
        return;
    }
    kfree(block);
}

/*
 *  This assumes the block passed to the function is not currently in a free-tree, if it is then it will cause issues commenting this now
 *  in case it becomes an issue later
 */

static void buddy_coalesce(struct buddy_block* block) {

    if(block == NULL) {
        return;
    }

    if (block->next == NULL) {
        insert_tree_node(&buddy_free_list_zone[block->zone], block, block->order);
        /*
         *  Can't coalesce until the predecessor is free
         */
        return;
    }

    if (block->order == MAX_ORDER) {
        insert_tree_node(&buddy_free_list_zone[block->zone], block, block->order);
        return;
    }

    /* This may be unneeded with high level locking but will keep it in mind still for the time being */

    acquire_spinlock(&buddy_lock);
    /* Putting this here in the case of the expression evaluated true and by the time the lock is held it is no longer true */
    if ((block->order == block->next->order) && (block->is_free == FREE && block->next->is_free == FREE) && (block->zone
        == block->next->zone)) {
        struct buddy_block* next = block->next;
        /*
         * Reflect new order and new next block
         */
        block->next = next->next;
        block->order++;

        buddy_block_free(next);

        insert_tree_node(&buddy_free_list_zone[block->zone], block, block->order);
    }
    release_spinlock(&buddy_lock);
}

