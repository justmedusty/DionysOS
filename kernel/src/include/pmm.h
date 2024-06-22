#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "limine.h"

extern volatile struct limine_hhdm_request hhdm_request;

#define PAGE_SIZE 0x1000

int phys_init();
void *phys_alloc(uint64 pages);
void phys_dealloc(void *address, uint64 pages);