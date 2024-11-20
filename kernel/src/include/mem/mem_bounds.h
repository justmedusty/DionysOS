//
// Created by dustyn on 6/23/24.
//
#pragma once
extern uint64_t kernel_min, kernel_max, kernel_size;
extern uint64_t kernel_phys_min, kernel_phys_max;
extern uint64_t virt_addr_min, virt_addr_max, virt_addr_size;
extern uint64_t userspace_addr_min, userspace_addr_max, userspace_addr_size;
void mem_bounds_init();