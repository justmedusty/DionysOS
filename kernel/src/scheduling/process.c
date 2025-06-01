//
// Created by dustyn on 12/7/24.
//

#include "include/scheduling/process.h"
#include "include/definitions/elf.h"
#include "include/filesystem/vfs.h"
#include "include/memory/vmm.h"
#include "include/scheduling/sched.h"

int32_t exec(char *path_to_executable, char **argv) {
    void *top_level_page_table;

}

__attribute__((noreturn))
void exit() {
    sched_exit();
}


void sleep(void *channel) {
    sched_sleep(channel);
}

void wakeup(void *channel) {
    sched_wakeup(channel);
}



