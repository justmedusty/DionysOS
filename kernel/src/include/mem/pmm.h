#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "limine.h"

extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_memmap_request memmap_request;


#define PAGE_SIZE 4096
#define BITMAP 0
#define BUDDY 1
/* For buddy */
#define MAX_ORDER 11 /* Going with the number Linux uses*/

#define STATIC_POOL_FLAG 1 << 5 /* So we know to return to the pool not try to call kfree on it */
#define FIRST_BLOCK_FLAG 1 << 6 /* So that we dont coalesce into other areas or memory*/
#define IN_TREE_FLAG 1 << 7

#define STATIC_POOL_SIZE 4096 /* Changine this complete changes everything for some reason.. Things break like crazy */
#define PHYS_ZONE_COUNT 15
#define FREE 0x1
#define USED 0x2
#define UNUSED 0x3

#define BUDDY_HASH_TABLE_SIZE 300

extern uint64 hhdm_offset;

int phys_init();
void *phys_alloc(uint64 pages);
void phys_dealloc(void *address, uint64 pages);


struct contiguous_page_range {
      uint64 start_address;
      uint64 end_address;
      uint64 pages;
  };


struct buddy_block {
    struct buddy_block *next;
    uint8 zone;
    void* start_address;
    uint8 flags;
    uint8 order;
    uint8 is_free;
};