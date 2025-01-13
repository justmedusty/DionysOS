//
// Created by dustyn on 1/6/25.
//
#include <stdint.h>
#include "stddef.h"
#include "include/data_structures/mutex.h"
#include "include/architecture/arch_atomic_operations.h"
#include "include/definitions/string.h"
#include "include/scheduling/sched.h"
#include "include/architecture/arch_cpu.h"

static bool try_mutex(struct mutex *lock) {
    if (!arch_atomic_swap_or_return(&lock->locked, 1)) {
        return false;
    }
    return true;
}

void acquire_mutex(struct mutex *mutex) {
    for (;;) {
        bool acquired = try_mutex(mutex);
        if (acquired) {
            break;
        }
        sched_sleep(mutex);
    }
    mutex->holder = current_process();
    mutex->cpu = mutex->holder->current_cpu->cpu_id;

}


void release_mutex(struct mutex *mutex) {
    mutex->locked = false;
    mutex->holder = NULL;
    mutex->cpu = -1;
}


void init_mutex(char *name, struct mutex *mutex) {

    mutex->locked = false;
    mutex->holder = NULL;
    mutex->cpu = -1;
    safe_strcpy(mutex->lock_name, name, MUTEX_NAME_LENGTH);
    mutex->lock_name[MUTEX_NAME_LENGTH +
                     1] = '\0'; // just in case there is some fuckery going on with the name we'll null terminate the last character just in case
    mutex->reserved = 0;
}