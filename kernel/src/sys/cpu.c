//
// Created by dustyn on 8/11/24.
//
#pragma once
#include "include/arch/arch_cpu.h"
#include "include/cpu.h"
#include "include/types.h"

cpu cpu_list[32];

void panic(const char *str){
    arch_panic(str);
}

cpu *mycpu(){
    return &cpu_list[arch_mycpu()];
}

