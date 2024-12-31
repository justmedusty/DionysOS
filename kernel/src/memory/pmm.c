//
// Created by dustyn on 6/21/24.
//
#include "include/definitions/types.h"
#include "include/memory/pmm.h"
#include <include/definitions/definitions.h>
#include <include/architecture/arch_cpu.h>
#include <include/data_structures/hash_table.h>
#include <include/data_structures/spinlock.h>
#include <include/device/display/framebuffer.h>
#include <include/memory/kalloc.h>
#include <include/memory/slab.h>
#include "limine.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_paging.h"
#include "include/drivers/serial/uart.h"
#include "include/data_structures/binary_tree.h"

/*
 * Static prototypes
 */
static void buddy_coalesce(struct buddy_block *block);

static struct buddy_block *buddy_alloc(uint64_t pages,uint8_t zone);

static void buddy_free(void *address);

static void buddy_block_free(struct buddy_block *block);

static struct buddy_block *buddy_block_get();

static struct buddy_block *buddy_split(struct buddy_block *block);

/*
 * Bootloader requests for the memory and HHDM offset
 */
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

#define KERNEL_POOL 0
#define USER_POOL 1

uint64_t total_allocated = 0;

struct spinlock pmm_lock;
struct spinlock buddy_lock;

 /* will just use one zone until it's all allocated then increment the zone pointer */
struct binary_tree buddy_free_list_zone[2];

struct buddy_block buddy_block_static_pool[STATIC_POOL_SIZE];
// should be able to handle ~8 GB of memory in 2 << max order size blocks and the rest can be taken from a slab

struct singly_linked_list unused_buddy_blocks_list;

struct hash_table used_buddy_hash_table;

uint64_t highest_page_index = 0;
uint64_t last_used_index = 0;
uint64_t usable_pages = 0;
uint64_t used_pages = 0;
uint64_t reserved_pages = 0;
uint64_t hhdm_offset = 0;
uint64_t page_range_index = 0;

//Need to handle sizing of this better but for now this should be fine statically allocating a semi-arbitrary amount I doubt there will be more than 10 page runs for this
struct contiguous_page_range contiguous_pages[10] = {};


/*
 * This init function just goes through the memory map passed by the bootloader which is then mapped into the contiguous page ranges depending on how many there are.
 * After this a bubble sort is applied to the contiguous page range array in order from largest to smallest
 *
 * Once this is done, the static pool is filled with MAX_ORDER size buddy blocks and each buddy block struct is assigned its zone pointer, start address, next block , etc
 *
 * As of right now, my indicator for the start of a max order block is with the flag FIRST_BLOCK_FLAG being tied to the start address of the original block.
 * This is not perfect because it can allow this scenario:
 *
 * First - > second merged with third -> fourth -> Next First
 *
 * I am aware of this, and will fix this at a later date.
 *
 * After this, the binary tree buddy_free_list_zone is filled. I was originally going to have a tree for each contiguous zone however
 * I have just hardcoded the first tree for now. I have yet to make a final decision and it is not important right now so it can wait.
 *
 * All free blocks are inserted into the tree.
 *
 *
 * After this , the hash table is initialized. The hashmap (used_buddy_hash_table). The way it works is when a block is allocated, the start addressed is hashed and put into the table.
 * When memory is freed, the address is hashed and looked up in the hashtable to remove it.
 *
 *
 */
uint64_t highest_address = 0;
int phys_init() {
    kprintf("Initializing Physical Memory Manager...\n");
    initlock(&buddy_lock, BUDDY_LOCK);
    initlock(&pmm_lock, PMM_LOCK);

    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    struct limine_memmap_entry **entries = memmap->entries;
    hhdm_offset = hhdm->offset;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

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
                memmove(&contiguous_pages[i], &contiguous_pages[i + 1], sizeof(struct contiguous_page_range));
                memmove(&contiguous_pages[i + 1], &placeholder, sizeof(struct contiguous_page_range));
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
        for (uint64_t j = 0; j < contiguous_pages[i].pages; j += 1 << MAX_ORDER) {
            buddy_block_static_pool[index].start_address = (void *) (
                contiguous_pages[i].start_address + (j * (PAGE_SIZE)) & ~(PAGE_SIZE -1));
            buddy_block_static_pool[index].flags = STATIC_POOL_FLAG;
            buddy_block_static_pool[index].order = MAX_ORDER;
            buddy_block_static_pool[index].zone = i;
            buddy_block_static_pool[index].is_free = FREE;


            if (index != 0 && (buddy_block_static_pool[index - 1].zone == buddy_block_static_pool[index].zone)) {
                buddy_block_static_pool[index - 1].next = &buddy_block_static_pool[index];
                buddy_block_static_pool[index - 1].flags |= FIRST_BLOCK_FLAG;
            }
            index++;
            if (index == STATIC_POOL_SIZE) {
                panic("Phys_init : Static pool size hit"); /* A panic is excessive but it's fine for now*/
            }
        }
        buddy_block_static_pool[index - 1].next = &buddy_block_static_pool[index];
        /* Since the logic above will not apply for the last entry putting this here */
    }

    singly_linked_list_init(&unused_buddy_blocks_list, 0);
    for (uint64_t i = index; i < STATIC_POOL_SIZE; i++) {
        buddy_block_static_pool[i].is_free = FREE;
        buddy_block_static_pool[i].zone = 0xFFFF;
        buddy_block_static_pool[i].next = NULL;
        buddy_block_static_pool[i].zone = UNUSED;
        buddy_block_static_pool[i].start_address = 0;
        buddy_block_static_pool[i].flags = STATIC_POOL_FLAG;
        buddy_block_static_pool[i].order = UNUSED;

        singly_linked_list_insert_head(&unused_buddy_blocks_list, &buddy_block_static_pool[i]);
    }

    /*
     *  Set up buddy blocks and insert them into proper trees
     */
    index = 0;
    init_tree(&buddy_free_list_zone[KERNEL_POOL],REGULAR_TREE, 0);
    init_tree(&buddy_free_list_zone[USER_POOL],REGULAR_TREE, 0);
    struct binary_tree *tree;
    uint64_t kcount = 0;
    uint64_t ucount = 0;
    for (int i = 0; i < page_range_index; i++) {
        while (buddy_block_static_pool[index].zone == i) {


            if ((uintptr_t) buddy_block_static_pool[index].start_address > USER_SPAN_SIZE) {
                tree = &buddy_free_list_zone[KERNEL_POOL];
                buddy_block_static_pool[index].zone = KERNEL_POOL;
                kcount++;
            }else {
                tree = &buddy_free_list_zone[USER_POOL];
                buddy_block_static_pool[index].zone = USER_POOL;
                ucount++;
            }


            insert_tree_node(tree, &buddy_block_static_pool[index],
                             buddy_block_static_pool[index].order);
            index++;
        }
    }
    kprintf("kcount %i ucount %i\n", kcount, ucount);

    hash_table_init(&used_buddy_hash_table,BUDDY_HASH_TABLE_SIZE);

    serial_printf("%i free block objects\n", unused_buddy_blocks_list.node_count);
    highest_page_index = highest_address / PAGE_SIZE;
    uint32_t pages_mib = (usable_pages * 4096) >> 20;

    kprintf("Physical Memory Manager Initialized\n");
    info_printf("%i MB of Memory Found\n", pages_mib);
    serial_printf("Physical memory mapped %i mb found\n", pages_mib);
    return 0;
}

/*
 * The phys_alloc function calls buddy_alloc which attempts to find a buddy block of an appropriate size to the request in pages. It panics on failure. The block
 * is marked USED and the return value is the start address of the buddy block that was found.
 */

void *phys_alloc(uint64_t pages,uint8_t zone) {
    struct buddy_block *block = buddy_alloc(pages,zone);
    if (block == NULL) {
        panic("phys_alloc cannot allocate");
    }
    block->is_free = USED;
    total_allocated += 1 << block->order;
    void *return_value = (void *) block->start_address;
    return return_value;
}

/*
 * The phys_dealloc simply calls buddy_free.
 */
void phys_dealloc(void *address) {
    buddy_free(address);
}


/*
 *  buddy_alloc first checks what order is required to meet the page demand
 *  It will iterate from 1 to MAX_ORDER checking if this # of pages matches, or if it is between two orders.
 *  When a match is found, it will attempt to allocate the right size.
 *
 *  If the right size gets no hits it looks for
 *  right size + 1, this continues until either a block is found or MAX_ORDER is reached.
 *
 * If the block found is not the ideal size, buddy_split is called in a loop until a block of appropriate size is returned.
 *
 * When the ideal block is finally attained, the start address is hashed and inserted into the hash table so that it can be
 * found when the address is freed.
 *
 * There is a single check on if the index is bigger than the order. This is because during debugging I uncovered that one particular block ALWAYS ends up
 * in the wrong tree node. Of the counterless thousands of operations, only one address was doing this. Since it was only 1 block doing this ,
 * I just put a check for it and to remove it from the tree so that it can never be allocated which removes the issue entirely.
 * At some point I should fix it but again, it is only one block out of thousands that does this, so it is not of high importance.
 *
 *
 */

bool is_power_of_two(uint64_t x) {
    return x && ((x & (x - 1)) == 0);
}

uint64_t next_power_of_two(uint64_t x) {
    if (x == 0) {
        return 1;
    }
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    return x + 1;
}

/*
 * My approach for tall orders is as follows :
 *
 * Alloc MAX_ORDER buddy block, and iterate through all of its MAX_ORDER blocks in the chain until there is enough to satisfy the order.
 *
 * The number of max order blocks is stored in the start block's buddy_chain_length field
 *
 * Freeing will just require an iterative approach to free everything and add it to the binary search tree
 */

static struct buddy_block *buddy_alloc_large_range(uint64_t pages, uint8_t zone) {
    uint64_t max_blocks = pages / (1 << MAX_ORDER) + (pages % 1 << MAX_ORDER != 0);
    struct buddy_block *block = NULL;
retry:
    uint64_t current_blocks = 1;
    block = lookup_tree(&buddy_free_list_zone[zone], MAX_ORDER,REMOVE_FROM_TREE);
    struct buddy_block *pointer = block;

    if (!block) {
        goto retry;
    }

    if (block->order != MAX_ORDER) {
        while (block != NULL && block->order != MAX_ORDER) {
            block = lookup_tree(&buddy_free_list_zone[zone], MAX_ORDER,REMOVE_FROM_TREE);
        }
    }

    if (!block) {
        goto retry;
    }
    while (pointer->next->order == MAX_ORDER && current_blocks < max_blocks && pointer->zone == pointer->next->zone) {
        pointer = pointer->next;
        current_blocks++;
    }

    if (current_blocks < max_blocks) {
        insert_tree_node(&buddy_free_list_zone[zone], block, block->order);
        /* Since it will be inserted at the tail we can do this and it wont keep grabbing the same block*/
        goto retry;
    }
    block->buddy_chain_length = max_blocks;
    block->flags &= ~IN_TREE_FLAG;
    current_blocks = 2;
    pointer = block->next;
    while (current_blocks <= max_blocks) {
        remove_tree_node(&buddy_free_list_zone[zone], pointer->order, pointer,NULL);
        pointer->is_free = USED;
        pointer = pointer->next;
        current_blocks++;
    }

    total_allocated += (1 << MAX_ORDER) * block->buddy_chain_length;

    return block;
}

static struct buddy_block *buddy_alloc(uint64_t pages,uint8_t zone) {
    if (pages > (1 << MAX_ORDER)) {
        return buddy_alloc_large_range(pages,zone);
    }

    if (!is_power_of_two(pages)) {
        pages = next_power_of_two(pages);
    }

    for (uint64_t i = 0; i < MAX_ORDER; i++) {
        if (pages == (1 << i)) {
            struct buddy_block *block = lookup_tree(&buddy_free_list_zone[zone], i,REMOVE_FROM_TREE);

            if (block == NULL) {
                uint64_t index = i + 1;

                while (index <= MAX_ORDER) {
                    block = lookup_tree(&buddy_free_list_zone[zone], index,REMOVE_FROM_TREE);

                    if (block != NULL && block->start_address && block->order >= index) {
                        block->flags &= ~IN_TREE_FLAG;
                        while (block->order != i) {
                            if (block->order > MAX_ORDER) {
                                serial_printf("block->order = %i block addr = %x.64 block start addr = %x.64\n",
                                              block->order, block, block->start_address);
                                panic("order higher than max order in buddy_alloc");
                            }
                            block = buddy_split(block);
                            if (block == NULL) {
                                panic("buddy_alloc cannot split block");
                            }
                        }

                        hash_table_insert(&used_buddy_hash_table, (uint64_t) block->start_address, block);
                        return block;
                    }
                    //find out where pointers are being manipulated so I can take this out
                    /*
                     * Note : there is only one
                     */

                    if (block != NULL && block->order < index) {
                        remove_tree_node(&buddy_free_list_zone[zone], index, block,NULL);
                        continue;
                    }
                    index++;
                }
            } else {
                hash_table_insert(&used_buddy_hash_table, (uint64_t) block->start_address, block);
                block->flags &= ~IN_TREE_FLAG;
                return block;
            }
        }
    }

    return NULL;
}

/*
 * The buddy_free function is relatively simple.
 * It looks up the address in the hash table.
 * The hash table returns the bucket rather than the entry. This may change in the future.
 * Because of that, a loop to find the address in the hash bucket is provided in this function.
 * Once it is found, buddy_coalesce is called, the function returns.
 */
static void buddy_free(void *address) {
    struct singly_linked_list *bucket = hash_table_retrieve(&used_buddy_hash_table,
                                                            hash((uint64_t) address, BUDDY_HASH_TABLE_SIZE));

    if (bucket == NULL) {
        panic("Buddy Dealloc: Buddy hash table not found");
        /* Shouldn't ever happen so panicking for visibility if it does happen */
    }

    struct singly_linked_list_node *node = bucket->head;

    while (1) {
        if (node == NULL) {
            /*
             *
             * If the node cannot be found in the hash bucket, this means that it is a slab entry that is right on a page line.
             * Because of this, if we do not find the address in the hash bucket we will invoke the slab free functions on the virtual
             * equivalent of the passed physical address and we will return
             *
             */
            struct header *slab_header = (struct header *) (
                (uint64_t) P2V(address) & ~((DEFAULT_SLAB_SIZE_PAGES * PAGE_SIZE) - 1));
            heap_free_in_slab(slab_header->slab, P2V(address));
            return;
        }
        struct buddy_block *block = node->data;

        if ((uint64_t) block->start_address == (uint64_t) address) {
            uint64_t ret = singly_linked_list_remove_node_by_address(bucket, block);
            if (ret == NODE_NOT_FOUND) {
                panic("Buddy Dealloc: Address not found");
            }
            block->is_free = FREE;
            block->flags &= ~IN_TREE_FLAG;
            total_allocated -= 1 << block->order;
            buddy_coalesce(block);
            return;
        }

        node = node->next;
    }
}

/*
 *  buddy_split takes a block and splits it in two.
 *
 *  It first ensures that the block is not in the free tree (it should never be there but its just for sanity)
 *
 *  It ensures the block isnt 0 because of course a page can't be split.
 *
 *  It ensured that the block is not USED
 *
 *  It calls buddy_block_get to get a spare buddy_block object
 *
 *  It fills in the new block as the buddy of the first, altering the start address and order
 *
 *  It puts the new block into the free tree, and returns the original, now split in half block
 */
static struct buddy_block *buddy_split(struct buddy_block *block) {
    if (block->flags & IN_TREE_FLAG) {
        remove_tree_node(&buddy_free_list_zone[KERNEL_POOL], block->order, block,NULL);
        block->flags &= ~IN_TREE_FLAG;
    }
    if (block->order > MAX_ORDER) {
        panic("Buddy Split : Illegal Block order");
    }
    if (block->order == 0) {
        panic("Buddy Split: Trying to split 0");
        return NULL; /* Can't split a zero order */
    }

    if (block->is_free == USED) {
        return NULL; /* Obviously */
    }

    struct buddy_block *new_block = buddy_block_get();

    if (new_block == NULL) {
        return NULL;
    }

    new_block->next = block->next;
    block->next = new_block;
    block->order--;
    new_block->order = block->order;
    new_block->start_address = block->start_address + (((1 << block->order) * PAGE_SIZE));
    new_block->zone = block->zone;
    new_block->is_free = FREE;


    new_block->flags |= IN_TREE_FLAG;
    const uint64_t ret = insert_tree_node(&buddy_free_list_zone[KERNEL_POOL], new_block, new_block->order);

    if (ret == SUCCESS) {
        return block;
    } else {
        panic("NULL");
    }

    return NULL;
}

/*
 * This function just attempts to get a spare block from the static pool list
 * If none is found, kalloc is invoked in instead.
 */
static struct buddy_block *buddy_block_get() {
    struct buddy_block *block = (struct buddy_block *) singly_linked_list_remove_head(&unused_buddy_blocks_list);
    if (block == NULL) {
        struct buddy_block *ret = _kalloc(sizeof(struct buddy_block));
        return ret;
    }
    return block;
}

/*
 * This function first does a sanity check ensuring that the block is not in a tree (this is a remnant of debugging , its not needed anymore but why not)
 *
 *If the block has the static pool flag, all values are set to 0 / NULL / UNUSED accordingly and inserted into the free list of static blocks
 *
 * If not, _kfree( is invoked.
 */
static void buddy_block_free(struct buddy_block *block) {
    if (block->flags & IN_TREE_FLAG) {
        remove_tree_node(&buddy_free_list_zone[KERNEL_POOL], block->order, block,NULL);
    }
    if (block->flags & STATIC_POOL_FLAG) {
        block->is_free = UNUSED;
        block->order = UNUSED;
        block->zone = UNUSED;
        block->start_address = 0;
        block->next = NULL;
        block->flags = STATIC_POOL_FLAG;
        block->buddy_chain_length = 0;
        singly_linked_list_insert_head(&unused_buddy_blocks_list, block);
        return;
    }
    _kfree(block);
}

/*
 *  This assumes the block passed to the function is not currently in a free-tree, if it is then it will cause issues commenting this now
 *  in case it becomes an issue later
 */


/*
 *This function tries to merge this block with the next block if it is free.
 *
 *Basic sanity checks, if the next block is NULL (miniscule chance but we check anyway) it is put into the free tree.
 *
 *If it is of order MAX_ORDER , it is put back in the tree.
 *
 *If the order of this block and the next, the is_free status of this block and the next, the zone (contiguous area of memory)
 *of this block and the next are ALL the same AND the next block does not have the FIRST_BLOCK_FLAG (it is the beginning of a MAX_ORDER area)
 *then merge the two blocks.
 *
 *AS-IS this is a naive implementation because as mentioned above it is possible to end up with blocks between boundaries merging inappropriately.
 *I will fix this at some point, but in terms of actual problems that will cause in practical terms it is next to zero. You would need to be using essentially all of the available memory and try
 *to make a large allocation for this to cause problems.
 */

static void buddy_coalesce(struct buddy_block *block) {
    if (block == NULL) {
        return;
    }

    if (block->next == NULL) {
        block->flags |= IN_TREE_FLAG;
        insert_tree_node(&buddy_free_list_zone[block->zone], block, block->order);
        /*
         *  Can't coalesce until the predecessor is free
         */
        return;
    }

    if (block->order == MAX_ORDER) {
        block->flags |= IN_TREE_FLAG;
        insert_tree_node(&buddy_free_list_zone[block->zone], block, block->order);
        return;
    }
    /* This may be unneeded with high level locking but will keep it in mind still for the time being */

    /* Putting this here in the case of the expression evaluated true and by the time the lock is held it is no longer true */
    if ((block->order == block->next->order) && (block->is_free == FREE && block->next->is_free == FREE) && (
            block->zone == block->next->zone && !(block->next->flags & FIRST_BLOCK_FLAG))) {
        struct buddy_block *next = block->next;
        /*
         * Reflect new order and new next block
         */
        block->next = next->next;
        block->order++;
        buddy_block_free(next);
        block->flags |= IN_TREE_FLAG;
        insert_tree_node(&buddy_free_list_zone[block->zone], block, block->order);
    }
}

