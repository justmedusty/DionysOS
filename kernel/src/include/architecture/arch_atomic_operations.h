//
// Created by dustyn on 7/2/24.
//

#ifndef DIONYSOS_ARCH_ATOMIC_OPERATIONS_H
#define DIONYSOS_ARCH_ATOMIC_OPERATIONS_H
#pragma once
#include <stdbool.h>

void arch_atomic_swap(uint64_t *field, uint64_t new_value);
bool arch_atomic_swap_or_return(uint64_t *field, uint64_t new_value);
#endif //DIONYSOS_ARCH_ATOMIC_OPERATIONS_H
