//
// Created by dustyn on 12/7/24.
//

#include "include/scheduling/process.h"
#include "include/definitions/elf.h"
#include "include/filesystem/vfs.h"
#include "include/memory/vmm.h"
#include "include/scheduling/sched.h"

// We will just have a 10mb sensible max for our elf files since I want to read the whole thing into memory on execute
#define SENSIBLE_FILE_SIZE (10 << 20)

int32_t exec(char *path_to_executable, char **argv) {
    uint64_t *top_level_page_table = alloc_virtual_map();
    struct elfhdr elf_header;
    struct proghdr program_header;
    struct process *current = current_process();

    struct vnode *node = vnode_lookup(path_to_executable);

    if(!node){
        warn_printf("Exec: Cannot exec, bad path %s\n",path_to_executable);
        return KERN_NOT_FOUND;
    }

    if (node->vnode_size > SENSIBLE_FILE_SIZE) {
        return KERN_TOO_BIG;
    }

    char *elf_image = kzmalloc(node->vnode_size);

    int64_t result = vnode_read(node,0,0,elf_image);

    if (result != KERN_SUCCESS) {
        kfree(elf_image);
        return KERN_UNEXPECTED;
    }

    struct elfhdr *header = (struct elfhdr *)elf_image;

    if (header->magic != ELF_MAGIC) {
        kfree(elf_image);
        return KERN_WRONG_TYPE;
    }

    map_kernel_address_space(top_level_page_table);

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



