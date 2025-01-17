//
// Created by dustyn on 1/16/25.
//

#include "include/architecture/x86_64/hpet.h"
#include "include/architecture/x86_64/acpi.h"
#include "include/architecture/x86_64/idt.h"
#include "include/architecture/x86_64/pit.h"

struct hpet hpet;

uint64_t address;

// Function to read a 64-bit value from an HPET register
static inline uint64_t hpet_read(uint64_t reg_offset) {
    return *((volatile uint64_t *) (address + reg_offset));
}


static inline void hpet_write(uint64_t reg_offset, uint64_t value) {
    *((volatile uint64_t *) (address + reg_offset)) = value;
}

void hpet_init() {
    address = (uint64_t) P2V(hpet.address.address);

    uint64_t config = hpet_read(general_config);
    config |= HPET_ENABLE_CNF_MASK;
    config |= HPET_LEG_RT_CNF_MASK;
    hpet_write(general_config, config);


    if (hpet_read(capabilities_id) & HPET_LEG_RT_CAP_MASK) {
        config |= HPET_LEG_RT_CNF_MASK;
        hpet_write(general_config, config);
    }

    config |= HPET_ENABLE_CNF_MASK;
    hpet_write(general_config, config);
}

uint64_t hpet_get_main_counter() {
    return hpet_read(main_counter_value);
}

void hpet_write_main_counter(uint64_t value) {
    hpet_write(main_counter_value, value);
}

void hpet_configure_timer(uint8_t timer_num, uint64_t config_value) {
    hpet_write(TIMER_N_CONFIG_CAPABILITY(timer_num), config_value);
}

void hpet_set_comparator(uint8_t timer_num, uint64_t comparator_value) {
    hpet_write(TIMER_N_COMPARATOR_VALUE(timer_num), comparator_value);
}

void hpet_initialize_and_enable_interrupts(uint64_t hz) {
    hpet_init();
    //To get the HZ value we will take 1 second worth of nanoseconds (1 billion) and divide by the passed number
    uint64_t second = 1000000000;
    uint64_t counter_value = second / hz;
    hpet_configure_timer(0, HPET_TN_INT_ENB_CNF_MASK | HPET_TN_VAL_SET_CNF_MASK);
    hpet_write_main_counter(0);
    hpet_set_comparator(0, counter_value);
    irq_register(0, x86_timer_interrupt);
}