//
// Created by dustyn on 6/24/24.
//
#include "include/limine.h"
#include "include/types.h"
#include "include/uart.h"

__attribute__((used, section(".requests")))
static volatile struct limine_paging_mode_request arch_page_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL
};

/*
 * Likely redundant be we want to be sure 4 level paging is enabled not 5
 */
void arch_paging_init(){
     struct limine_paging_mode_response *response = arch_page_request.response;
     if(response->mode != LIMINE_PAGING_MODE_X86_64_4LVL){
         bootleg_panic("Paging not 4 level!");
     }
    serial_printf("4 Level Paging Confirmed\n");

}