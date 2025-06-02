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
    struct vnode *node;
    struct elfhdr elf_header;
    struct proghdr program_header;
    struct process *current = current_process();

    node = vnode_lookup(path_to_executable);

    if(!node){
        warn_printf("Exec: Cannot exec, bad path %s\n",path_to_executable);
        return KERN_NOT_FOUND;
    }





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



