//
// Created by dustyn on 6/21/24.
//

#pragma once
#include "arch/x86_64/pit.h"

void arch_timer_init(){
    pit_init();
}