//
// Created by dustyn on 6/25/24.
//

#pragma once
#include "include/types.h"
#include "include/arch_vmm.h"

//commenting out not yet implemented data structures
typedef struct{
    uint64 cpu_id;
    struct virt_map* page_map;
    //struct queue *local_rq;
    //struct proc *curr_proc
} cpu;

void panic(const char* str);

