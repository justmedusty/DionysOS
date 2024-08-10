//
// Created by dustyn on 6/24/24.
//
#include "include/types.h"
#include "limine.h"
#include "include/uart.h"
#include "include/cpu.h"

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

}

