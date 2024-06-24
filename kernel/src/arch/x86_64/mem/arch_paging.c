//
// Created by dustyn on 6/24/24.
//
#include "include/limine.h"

__attribute__((used, section(".requests")))
static volatile struct limine_paging_mode_request arch_page_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0
};