//
// Created by dustyn on 8/11/24.
//

#pragma once
#include "include/cpu.h"
#include "arch_vmm.h"
#include "include/types.h"

typedef struct{
    uint64 cpu_id;
    struct virt_map page_map;
    //struct queue local_rq;
    //struct proc *curr_proc;
} cpu;

//static data structure for now this all just chicken scratch for the time being but I don't see a point of a linked list for cpus since it will never be more than 4 probably
extern cpu cpu_list[32];
cpu *mycpu();
void panic(const char *str);


