//
// Created by dustyn on 8/25/24.
//

#pragma once
#include "include/types.h"

static inline uint64 rdmsr(uint32 msr){
    uint32 eax = 0;
    uint32 edx =  0;
    asm volatile("rdmsr" : "=a" (eax) ,"=d" (edx) : "c" (msr) : "memory");
    return (uint64)edx << 32 | eax;
}

static inline void wrmsr(uint32 msr, uint64 value){
    uint32 eax = (uint32) value;
    uint32 edx = (uint64) value >> 32;
    asm volatile("wrmsr" : : "a" (eax) ,"d" (edx), "c" (msr) : "memory");
}