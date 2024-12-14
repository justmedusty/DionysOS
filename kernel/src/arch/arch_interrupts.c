//
// Created by dustyn on 8/11/24.
//

#include "../include/arch/arch_interrupts.h"
#include "../include/arch/x86_64/idt.h"

#ifdef __x86_64__

void arch_setup_interrupts(){
    irq_handler_init();
    idt_init();
}
//will do all this later, just thinking about the HAL now
void arch_register_irq(uint8_t vector,void *handler) {
    irq_register(vector,handler);
}
void arch_unregister_irq(uint8_t vector) {
    irq_unregister(vector);
}

#endif
