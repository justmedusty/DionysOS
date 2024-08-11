//
// Created by dustyn on 8/11/24.
//

#include "include/arch/arch_interrupts.h"
#include "idt.h"


void arch_setup_interrupts(){
    idt_init();
}
//will do all this later, just thinking about the HAL now
void register_irq(){}
void unregister_irq(){}