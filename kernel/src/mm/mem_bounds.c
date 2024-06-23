//
// Created by dustyn on 6/23/24.
//

#include "include/types.h"
#include "include/paging.h"
#include "include/limine.h"
#include "include/uart.h"

extern char _kernel_end[];

uint64 kernel_min, kernel_max, kernel_size;
uint64 kernel_phys_min, kernel_phys_max, kernel_phys_size;
uint64 virt_addr_min, virt_addr_max, virt_addr_size;
uint64 userspace_addr_min, userspace_addr_max, userspace_addr_size;

__attribute__((used, section(".requests")))
static volatile struct limine_kernel_address_request kernel_address_request = {
        .id = LIMINE_KERNEL_ADDRESS_REQUEST,
        .revision = 0
};

void mem_bounds_init() {
    // kernel's virtual memory bounds
    kernel_min = kernel_address_request.response->virtual_base;
    kernel_max = (uint64 *) &_kernel_end;
    kernel_size = kernel_max - kernel_min;

    /* The kernel's phys memory bounds */
    kernel_phys_min = kernel_address_request.response->physical_base;
    kernel_phys_max = kernel_phys_min + kernel_size;

    /* The userland's virtual memory bounds.*/
    userspace_addr_min = 0x1000;
    userspace_addr_max = 0x00007FFFFFFFFFFF;
    userspace_addr_size = (userspace_addr_max / PAGE_SIZE) * PAGE_SIZE;
    userspace_addr_size = userspace_addr_max - userspace_addr_min;

    write_string_serial("Bounds Initialized. Kernel size : ");
    write_hex_serial(kernel_size, 64);
    write_string_serial("\nKernel upper virtual limit : ");
    write_hex_serial(kernel_max, 64);
    write_string_serial("\nKernel lower virtual limit : ");
    write_hex_serial(kernel_min, 64);
    write_string_serial("\nKernel upper physical limit : ");
    write_hex_serial(kernel_phys_max, 64);
    write_string_serial("\nKernel lower physical limit : ");
    write_hex_serial(kernel_phys_min, 64);
    write_string_serial("\nUserspace size : ");
    write_hex_serial(userspace_addr_size, 64);
    write_string_serial("\nUserspace lower limit : ");
    write_hex_serial(userspace_addr_min, 16);
    write_string_serial("\nUserspace upper limit : ");
    write_hex_serial(userspace_addr_max, 64);
    write_serial('\n');
}