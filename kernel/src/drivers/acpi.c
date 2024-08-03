//
// Created by dustyn on 8/3/24.
//

#include "acpi.h"
#include "include/uart.h"
#include "limine.h"

__attribute__((used, section(".requests")))
static volatile struct limine_rsdp_request rdsp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

