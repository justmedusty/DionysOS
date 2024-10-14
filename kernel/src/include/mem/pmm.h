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
    struct block *next;
    uint64 start_address;
    uint64 flags;
    uint64 order;
    uint64 is_free;
};