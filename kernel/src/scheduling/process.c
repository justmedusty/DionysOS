//
// Created by dustyn on 12/7/24.
//

#include "include/scheduling/process.h"
#include "include/definitions/elf.h"
#include "include/filesystem/vfs.h"
#include "include/memory/vmm.h"
#include "include/architecture/arch_cpu.h"
#include "include/scheduling/sched.h"

// We will just have a 10mb sensible max for our elf files since I want to read the whole thing into memory on execute
#define SENSIBLE_FILE_SIZE (10 << 20)

int32_t exec(char *path_to_executable, char **argv) {
    uint64_t *top_level_page_table = alloc_virtual_map();
    struct process *current = current_process();
}

__attribute__((noreturn))
void exit() {
    sched_exit();
}

void sleep(void *channel) {
    sched_sleep(channel);
}

void wakeup(const void *channel) {
    sched_wakeup(channel);
}

int64_t spawn(char *path_to_executable,uint64_t flags, struct spawn_options *options) {

}



