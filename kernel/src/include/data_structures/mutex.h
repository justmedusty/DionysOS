//
// Created by dustyn on 1/6/25.
//

#ifndef KERNEL_MUTEX_H
#define KERNEL_MUTEX_H
#define MUTEX_NAME_LENGTH 30
struct mutex {
    uint64_t locked;
    struct process *holder;
    int64_t cpu;
    char lock_name[32];
    uint64_t reserved;

};

void init_mutex(char *name, struct mutex *mutex);

void release_mutex(struct mutex *mutex);

void acquire_mutex(struct mutex *mutex);

#endif //KERNEL_MUTEX_H
