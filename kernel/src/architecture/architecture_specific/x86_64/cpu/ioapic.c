//
// Created by dustyn on 6/21/24.
//
#include <include/architecture/x86_64/idt.h>
#include "include/architecture/x86_64/pit.h"
#include <include/architecture/arch_cpu.h>
#include "include/architecture/arch_global_interrupt_controller.h"
#include <include/architecture/arch_paging.h>
#include "include/architecture/x86_64/asm_functions.h"
#include <include/drivers/serial/uart.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_DATA PIC1 + 1
#define PIC2_DATA PIC2 + 1

/*
 * Disable the legacy PIC , in case it is needed.
 * It may already be masked, but better safe than sorry
 */
void pic_disable() {
    //Mask all legacy PIC interrupts
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/*
 * These are the typical x86 IOAPIC functions.
 * Rather than explain each one I will link to the
 * documentation since there is nothing special here.
 *
 * https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
 *
 */
void write_ioapic(uint32_t ioapic, uint32_t reg, uint32_t value) {
    uint64_t base = (uint64_t)P2V(madt_ioapic_list[ioapic]->apic_addr);
    *((volatile uint32_t*)base) = reg;
    base += IOAPIC_IOWIN;
    *((uint32_t*)base) = value;
}

uint32_t read_ioapic(uint32_t ioapic, uint32_t reg) {
    uint64_t base = (uint64_t)P2V(madt_ioapic_list[ioapic]->apic_addr);
    *(uint32_t*)base = reg;
    base += IOAPIC_IOWIN;
    return *((volatile uint32_t*)base);
}

uint64_t ioapic_gsi_count(uint32_t ioapic) {
    if (ioapic > madt_ioapic_len) {
        return 0;
    }
    uint64_t value = (read_ioapic(ioapic, 1) & 0xFF0000) >> 16;
    return value;
}

uint32_t ioapic_get_gsi(uint32_t gsi) {
    for (uint64_t i = 0; i < madt_ioapic_len; i++) {
        if (madt_ioapic_list[i]->gsi_base <= gsi && madt_ioapic_list[i]->gsi_base + ioapic_gsi_count(i) > gsi) {
            return i;
        }
    }
    panic("Cannot determine IOAPIC from GSI\n");
}

void ioapic_redirect_gsi(uint32_t lapic_id, uint8_t vector, uint32_t gsi, uint16_t flags, uint8_t mask) {
    uint32_t ioapic = ioapic_get_gsi(gsi);
    uint64_t redirect = (uint64_t)vector;

    if ((flags & BIT(1)) != 0) {
        redirect |= BIT(13);
    }

    if ((flags & BIT(3)) != 0) {
        redirect |= BIT(15);
    }
    if(mask == 1) {
        redirect |= BIT(16);
    }
    redirect |= (uint64_t)lapic_id << 56;

    /*
     * If I don't put this empty print statement here then the redirection table is NOT written at all. I have no idea why. Is this gcc doing something funny? Is my memory access in write_ioapic buggy? Is this some QEMU bug or quirk?
     * I do not know. But I will leave this comment here until I figure out what in gods holy name is causing this. I thought maybe there was some weird lock contention with serial_printf but no I can remove the lock entirely and nothing changes..
     * Very mysterious.
     *
     * Update 1 : It appears to be the outb / port writing that causes this to suddenly work. Writing to serial port makes it work, writing to legacy pic makes it work, seems writing to any IO port makes this work.
     * This leads me to believe it has to be a QEMU quirk or bug. I will try it on real hardware later.
     *
     * Update 2 : Marking the redirection table volatile fixes the issue. This part makes sense, writing I/O ports fixing it doesn't make sense. I am going to leave this here as a little piece of history of this code base lol.
     */


    volatile uint32_t redir_table = (gsi - madt_ioapic_list[ioapic]->gsi_base) * 2 + 16;
    write_ioapic(ioapic, redir_table, (uint32_t)redirect);
    write_ioapic(ioapic, redir_table + 1, (uint32_t)redirect >> 32);


}

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t vector, uint8_t irq, uint8_t mask) {
    uint32_t index = 0;
    struct madt_iso* iso;

    while (index < madt_iso_len) {
        iso = madt_iso_list[index];
        if (iso->irq_src == irq) {
            if (mask) {
                serial_printf("IRQ %x.8  override\n", irq);
            }
            ioapic_redirect_gsi(lapic_id, vector, iso->gsi, iso->flags, mask);
            return;
        }
        index++;
    }
    ioapic_redirect_gsi(lapic_id, vector, irq, 0, mask);
}

