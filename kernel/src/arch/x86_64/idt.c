//
// Created by dustyn on 6/20/24.
//
extern void isr_wrapper();
#include "include/gdt.h"
#include "include/x86.h"
#include "include/idt.h"
struct gate_desc gates[64] = {};
struct idtr_desc idtr = {
        .off = (uint64_t)&gates,
        .sz = sizeof(gates) - 1,
};

void create_interrupt_gate(struct gate_desc *gate_desc, void *isr) {
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

void idt_init(void) {
  for(int i = 0; i < 64;i++){
      create_interrupt_gate(&gates[i],&isr_wrapper);
  }

    // https://forum.osdev.org/viewtopic.php?p=316295#p316295
    outb(0x11, 0x20); // initialize, pic1_cmd
    outb(0x11, 0xA0); // initialize, pic2_cmd
    outb(0x20, 0x21); // 32 (offset), pic1_data
    outb(40, 0xA1);   // 40 (offset), pic2_data
    outb(0x04, 0x21); // slave PIC at IRQ2, pic1_data
    outb(0x02, 0xA1); // cascade identity, pic2_data
    outb(0x01, 0x21); // 8086 mode, pic1_data
    outb(0x01, 0xA1); // 8086 mode, pic2_data
    outb(0xF8, 0x21); // 0b11111000 mask, pic1_data -- unmask irq 0, 1, 2
    outb(0xEF, 0xA1); // 0b11101111 mask, pic2_data -- unmask irq 12

    load_idtr(&idtr);

    //enable interrupts (locally)
    __asm__ volatile("sti");

}