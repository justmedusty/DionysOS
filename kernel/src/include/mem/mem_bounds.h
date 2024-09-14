//
// Created by dustyn on 6/23/24.
//
#pragma once
extern uint64 kernel_min, kernel_max, kernel_size;
extern uint64 kernel_phys_min, kernel_phys_max;
extern uint64 virt_addr_min, virt_addr_max, virt_addr_size;
extern uint64 userspace_addr_min, userspace_addr_max, userspace_addr_size;
void mem_bounds_init();