//
// Created by dustyn on 8/11/24.
//

#pragma once
#include "include/definitions/types.h"

#include "include/architecture/x86_64/idt.h"
 void arch_setup_interrupts();
void arch_register_irq(uint8_t vector,void *handler);
void arch_unregister_irq(uint8_t vector);

