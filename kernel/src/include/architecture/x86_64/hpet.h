//
// Created by dustyn on 1/16/25.
//

#ifndef KERNEL_HPET_H
#define KERNEL_HPET_H
#pragma once

#include "stdint.h"
#define REGISTER_WIDTH 8
enum register_offsets {
    capabilities_id = 0x0,
    general_config = 0x10,
    general_interrupt_status = 0x20,
    main_counter_value = 0xF0,
    timer_n_config_capability = 0x100,
    timer_n_comparator_value = 0x108,
    timer_n_fsb_interrupt_route = 0x110
};

// Macros for Timer N registers, where N is the timer number (0, 1, 2, ...)
#define TIMER_N_CONFIG_CAPABILITY(N) (0x100 + 0x20 * (N))
#define TIMER_N_COMPARATOR_VALUE(N) (0x108 + 0x20 * (N))
#define TIMER_N_FSB_INTERRUPT_ROUTE(N) (0x110 + 0x20 * (N))

// Bit masks for General Capabilities and ID Register
#define HPET_COUNTER_CLK_PERIOD_MASK 0xFFFFFFFF00000000
#define HPET_VENDOR_ID_MASK 0x0000FFFF00000000
#define HPET_LEG_RT_CAP_MASK 0x0000000000008000
#define HPET_COUNT_SIZE_CAP_MASK 0x0000000000002000
#define HPET_NUM_TIM_CAP_MASK 0x0000000000001F00
#define HPET_REV_ID_MASK 0x00000000000000FF

// Bit masks for General Configuration Register
#define HPET_LEG_RT_CNF_MASK 0x0000000000000002
#define HPET_ENABLE_CNF_MASK 0x0000000000000001

// Bit masks for General Interrupt Status Register
#define HPET_TN_INT_STS_MASK(N) (1UL << (N))  // N-th timer status bit

// Bit masks for Timer N Configuration and Capability Register
#define HPET_TN_INT_ROUTE_CAP_MASK 0xFFFFFFFF00000000
#define HPET_TN_FSB_INT_DEL_CAP_MASK 0x0000000000008000
#define HPET_TN_FSB_EN_CNF_MASK 0x0000000000004000
#define HPET_TN_INT_ROUTE_CNF_MASK 0x0000000000003E00
#define HPET_TN_32MODE_CNF_MASK 0x0000000000000100
#define HPET_TN_VAL_SET_CNF_MASK 0x0000000000000040
#define HPET_TN_SIZE_CAP_MASK 0x0000000000000020
#define HPET_TN_PER_INT_CAP_MASK 0x0000000000000010
#define HPET_TN_TYPE_CNF_MASK 0x0000000000000008
#define HPET_TN_INT_ENB_CNF_MASK 0x0000000000000004
#define HPET_TN_INT_TYPE_CNF_MASK 0x0000000000000002

struct address_structure {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed));



struct hpet {
    uint8_t hardware_rev_id;
    uint8_t comparator_count: 5;
    uint8_t counter_size: 1;
    uint8_t reserved: 1;
    uint8_t legacy_replacement: 1;
    uint16_t pci_vendor_id;
    struct address_structure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));
#endif //KERNEL_HPET_H
