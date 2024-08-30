//
// Created by dustyn on 6/24/24.
//
#include <include/arch/arch_cpu.h>
#include "include/types.h"
#include "limine.h"
#include "include/uart.h"
#include <include/arch/arch_paging.h>

uint64 bootstrap_lapic_id;
uint64 cpu_count;


struct limine_smp_info **smp_info;

__attribute__((used, section(".requests")))
static volatile struct limine_smp_request smp_request = {
        .id = LIMINE_SMP_REQUEST,
        .revision = 0,
};

void arch_smp_query(){
    struct limine_smp_response *response = smp_request.response;
    if(!response){
        panic("SMP Response NULL");
    }
    bootstrap_lapic_id = response->bsp_lapic_id;
    cpu_count = response->cpu_count;
    smp_info = response->cpus;

    serial_printf("LAPIC ID : %x.8 \nCPU Count : %x.8 \n",bootstrap_lapic_id,cpu_count);
    uint8 i = 0;

    while(i < cpu_count) {
        if(i == 32) {
            break;
        }
        cpu_list[i].cpu_number = smp_info[i]->processor_id;
        cpu_list[i].lapic_id= smp_info[i]->lapic_id;
        serial_printf("  CPU %x.8  LAPIC %x.8  initialized inside cpu_list\n",smp_info[i]->processor_id, smp_info[i]->lapic_id);
        i++;
        //puts rest of CPUs online, works so will leave this commented out for now since I need to create or refactor functions for this
        //smp_info[i]->goto_address = arch_initialise_cpu;
    }


}

