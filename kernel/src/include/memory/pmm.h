#ifndef PMM_H
#define PMM_H
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "limine.h"

extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_memmap_request memmap_request;
extern uint64_t highest_address;
#define KERNEL_POOL 0
#define USER_POOL 1
#define PAGE_SIZE 4096UL
#define BITMAP 0
#define BUDDY 1
/* For buddy */
#define MAX_ORDER 10
#define STATIC_POOL_FLAG BIT(5) /* So we know to return to the pool not try to call kfree on it */
#define FIRST_BLOCK_FLAG BIT(6) /* So that we dont coalesce into other areas or memory*/
#define IN_TREE_FLAG BIT(7)
#define STATIC_POOL_SIZE 48000UL
#define PHYS_ZONE_COUNT 15
#define FREE 0x1
#define USED 0x2
#define UNUSED 0xFFFFFFFFFFFFFFFFUL

#define DEFAULT_SLAB_SIZE_PAGES 32
extern uint64_t lowest_user_phys_addr;
extern uint64_t highest_user_phys_addr;
extern uint64_t hhdm_offset;
extern uint64_t total_allocated;
extern uint64_t usable_pages;

extern uint64_t page_range_index;
int phys_init();
void *phys_alloc(uint64_t pages,uint8_t zone);
void phys_dealloc(void *address);
uint64_t next_power_of_two(uint64_t x);
bool is_power_of_two(uint64_t x);
bool check_phys_addr_usage(void *addr) ;

struct contiguous_page_range {
      uint64_t start_address;
      uint64_t end_address;
      uint64_t pages;
      uint64_t reserved;
  };
extern struct contiguous_page_range contiguous_pages[20];

struct buddy_block {
    struct buddy_block *next;
    uint64_t zone;
    void* start_address;
    uint64_t flags;
    uint8_t order;
    uint8_t is_free;
    uint64_t buddy_chain_length;
    uint64_t reserved;
    uint64_t reserved_2;
};

#endif