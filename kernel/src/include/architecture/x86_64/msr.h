//
// Created by dustyn on 8/25/24.
//

#pragma once
#include "include/types.h"

static inline uint64_t rdmsr(uint32_t msr){
    uint32_t eax = 0;
    uint32_t edx =  0;
    asm volatile("rdmsr" : "=a" (eax) ,"=d" (edx) : "c" (msr) : "memory");
    return (uint64_t)edx << 32 | eax;
}

static inline void wrmsr(uint32_t msr, uint64_t value){
    uint32_t eax = (uint32_t) value;
    uint32_t edx = (uint64_t) value >> 32;
    asm volatile("wrmsr" : : "a" (eax) ,"d" (edx), "c" (msr) : "memory");
}