//
// Created by dustyn on 11/2/24.
//

#ifndef MATH_H
#define MATH_H
#include "include/definitions/types.h"
/*
 * Simpe power function returns base^power
 */
static inline uint64_t pow(uint64_t base, uint64_t power) {
    uint64_t working_value = base;
    uint64_t index = power;
    /*
     * Use index to track how many times we have raises working_value to base, and also check for rollover in case we go over uint64_t_MAX
     */
    while (index - 1 != 0 && working_value >= base) {
        working_value = working_value * base;
        index--;
    }
    if (working_value < base) {
        return 0; //try to detect overflow
    }

    return working_value;
}

#endif //MATH_H
