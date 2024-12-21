//
// Created by dustyn on 6/23/24.
//

#include <include/architecture/arch_vmm.h>
#include "include/types.h"
#include "include/architecture/arch_paging.h"
#include "limine.h"
#include "include/drivers/serial/uart.h"
#include "include/memory/pmm.h"


uint64_t kernel_min, kernel_max, kernel_size;
uint64_t kernel_phys_min, kernel_phys_max, kernel_phys_size;
uint64_t virt_addr_min, virt_addr_max, virt_addr_size;
uint64_t userspace_addr_min, userspace_addr_max, userspace_addr_size;

__attribute__((used, section(".requests")))
static volatile struct limine_kernel_address_request kernel_address_request = {
        .id = LIMINE_KERNEL_ADDRESS_REQUEST,
        .revision = 0
};

void mem_bounds_init() {
    // kernel's virtual memory bounds
    kernel_min = kernel_address_request.response->virtual_base;
    kernel_max = (uint64_t) data_end;
    kernel_size = kernel_max - kernel_min;

    /* The kernel's phys memory bounds */
    kernel_phys_min = kernel_address_request.response->physical_base;
    kernel_phys_max = kernel_phys_min + kernel_size;

    /* The userland's virtual memory bounds.*/
    userspace_addr_min = 0x1000;
    userspace_addr_max = 0x00007FFFFFFFFFFF;
    userspace_addr_size = (userspace_addr_max / PAGE_SIZE) * PAGE_SIZE;
    userspace_addr_size = userspace_addr_max - userspace_addr_min;

    serial_printf("Bounds Discovered. Kernel size : %x\n",kernel_size);
    serial_printf("Kernel upper virtual limit : %x\n",kernel_max);
    serial_printf("Kernel lower virtual limit : %x\n",kernel_min);
    serial_printf("Kernel upper physical limit : %x\n",kernel_phys_max);
    serial_printf("Kernel lower physical limit : %x\n",kernel_phys_min);
    serial_printf("Userspace size : %x\n",userspace_addr_size);
    serial_printf("Userspace lower limit : %x\n",userspace_addr_min);
    serial_printf("Userspace upper limit : %x\n",userspace_addr_max);

}