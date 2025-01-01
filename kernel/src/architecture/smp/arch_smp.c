//
// Created by dustyn on 6/24/24.
//
#include <include/definitions/definitions.h>
#include <include/architecture/arch_cpu.h>
#include "include/definitions/types.h"
#include "limine.h"
#include "include/drivers/serial/uart.h"
#include <include/architecture/arch_paging.h>
#include <include/architecture/arch_smp.h>
#include <include/data_structures/spinlock.h>
#include <include/device/display/framebuffer.h>

uint64_t bootstrap_lapic_id;
uint64_t cpu_count;


struct limine_smp_info **smp_info;

__attribute__((used, section(".requests")))
static volatile struct limine_smp_request smp_request = {
        .id = LIMINE_SMP_REQUEST,
        .revision = 0,
};

uint8_t smp_enabled = 0;

uint8_t cpus_online = 1;

void smp_init(){

    initlock(&bootstrap_lock,SMP_BOOSTRAP_LOCK);
    struct limine_smp_response *response = smp_request.response;

    if(!response){
        panic("SMP Response NULL");
    }

    bootstrap_lapic_id = response->bsp_lapic_id;
    cpu_count = response->cpu_count;
    smp_info = response->cpus;

    serial_printf("LAPIC ID : %x.8 \nCPU Count : %x.8 \n",bootstrap_lapic_id,cpu_count);
    uint8_t i = 0;
    kprintf("%i CPUs Found\n",cpu_count);
    //For output cleanliness
    acquire_spinlock(&bootstrap_lock);
    while(i < cpu_count) {
        if(i == MAX_CPUS) {
            warn_printf("DionysOS only supports up to %i processors!\n", MAX_CPUS);
            cpus_online = cpu_count;
            break;
        }
        /*
         *  Index into the cpu array based on the LAPIC ID which should be easier to get in the case that they do not line up with processor id
         */
        cpu_list[smp_info[i]->lapic_id].cpu_number = smp_info[i]->processor_id;
        cpu_list[smp_info[i]->lapic_id].lapic_id= smp_info[i]->lapic_id;
        serial_printf("  CPU %x.8  LAPIC %x.8  initialized inside cpu_list\n",smp_info[i]->processor_id, smp_info[i]->lapic_id);
        cpu_list[smp_info[i]->lapic_id].scheduler_state = kmalloc(sizeof(struct gpr_state));
        //puts rest of CPUs online, works so will leave this commented out for now since I need to create or refactor functions for this
        smp_info[i]->goto_address = arch_initialise_cpu;
        i++;
    }
    release_spinlock(&bootstrap_lock);

    while((volatile uint8_t) cpus_online != cpu_count) {
        asm volatile("nop");
    }
    smp_enabled = 1;
    kprintf("All CPUs Online\n");
    serial_printf("\n\nAll CPUs online......\n\n\n");



}

