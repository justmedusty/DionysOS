//
// Created by dustyn on 6/20/24.
//

#include "gdt.h"
#include "include/arch/x86_64/arch_asm_functions.h"
#include "include/types.h"
#include "idt.h"

#include <include/arch/arch_cpu.h>

#include "include/arch/arch_traps.h"
#include "include/uart.h"
#include "include/arch/arch_global_interrupt_controller.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include "include/arch/arch_smp.h"

extern void isr_wrapper_0();
extern void isr_wrapper_1();
extern void isr_wrapper_2();
extern void isr_wrapper_3();
extern void isr_wrapper_4();
extern void isr_wrapper_5();
extern void isr_wrapper_6();
extern void isr_wrapper_7();
extern void isr_wrapper_8();

extern void isr_wrapper_10();
extern void isr_wrapper_11();
extern void isr_wrapper_12();
extern void isr_wrapper_13();
extern void isr_wrapper_14();

extern void isr_wrapper_16();
extern void isr_wrapper_17();
extern void isr_wrapper_18();
extern void isr_wrapper_19();

void* irq_routines[256] = {};

uint8 idt_free_vec = 32;

//Going to index this array when setting up the idt
void (*isr_wrappers[32])() = {
    isr_wrapper_0,
    isr_wrapper_1,
    isr_wrapper_2,
    isr_wrapper_3,
    isr_wrapper_4,
    isr_wrapper_5,
    isr_wrapper_6,
    isr_wrapper_7,
    isr_wrapper_8,
    0,
    isr_wrapper_10,
    isr_wrapper_11,
    isr_wrapper_12,
    isr_wrapper_13,
    isr_wrapper_14,
    0,
    isr_wrapper_16,
    isr_wrapper_17,
    isr_wrapper_18,
    isr_wrapper_19,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0

};

struct gate_desc gates[256] = {};

struct idtr_desc idtr = {
    .off = (uint64_t)&gates,
    .sz = sizeof(gates) - 1,
};

void create_interrupt_gate(struct gate_desc* gate_desc, void* isr){
    // Select 64-bit code segment of the GDT. See
    // https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md#x86_64.
    gate_desc->segment_selector = (struct segment_selector){
        .index = GDT_SEGMENT_RING0_CODE,
    };
    // Don't use IST.
    gate_desc->ist = 0;
    // ISR.
    gate_desc->gate_type = SSTYPE_INTERRUPT_GATE;
    // Run in ring 0.
    gate_desc->dpl = 0;
    // Present bit.
    gate_desc->p = 1;
    // Set offsets slices.
    gate_desc->off_1 = (size_t)isr;
    gate_desc->off_2 = ((size_t)isr >> 16) & 0xffff;
    gate_desc->off_3 = (size_t)isr >> 32;
}


void create_void_gate(struct gate_desc* gate_desc){
    // Select 64-bit code segment of the GDT. See
    // https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md#x86_64.
    gate_desc->segment_selector = (struct segment_selector){
        .index = GDT_SEGMENT_RING0_CODE,
    };
    // Don't use IST.
    gate_desc->ist = 0;
    // ISR.
    gate_desc->gate_type = SSTYPE_INTERRUPT_GATE;
    // Run in ring 0.
    gate_desc->dpl = 0;
    // Present bit.
    gate_desc->p = 0;
    // Set offsets slices.
    gate_desc->off_1 = 0xff;
    gate_desc->off_2 = 0xff;
    gate_desc->off_3 = 0xffff;
}

void idt_init(void){
    //just exceptions
    for (int i = 0; i < 31; i++){
        if (exceptions[i] != T_NONE){
            create_interrupt_gate(&gates[i], isr_wrappers[i]);
        }
        else{
            create_void_gate(&gates[i]);
        }
    }
    load_idtr(&idtr);
    //enable interrupts (locally)
    asm volatile("sti");
    write_string_serial("IDT Loaded, ISRs mapped\n");
}

uint8 idt_get_vector(){
  if(idt_free_vec == 255){
    panic("idt_get_vector() table full\n");
  	}
      return idt_free_vec++;
}
uint8 idt_get_irq_vector(){
  if(idt_free_vec == 255){
    panic("idt_get_irq_vector out of space");
  }
  return idt_free_vec++;
}
void irq_register(uint8 vec, void* handler){
   irq_routines[vec + 32] = handler;
   ioapic_redirect_irq(bootstrap_lapic_id,vec + 32, vec,1);
   serial_printf("IRQ %x.8  loaded\n",vec);
}

void irq_unregister(uint8 vec) {
    irq_routines[vec]  = NULL;
}