//
// Created by dustyn on 6/21/24.
//

#pragma once
#include "include/arch/x86_64/pit.h"

static inline void arch_timer_init(){
    pit_init();
}

static inline uint64_t arch_timer_get_ticks() {
    return get_pit_ticks();
}