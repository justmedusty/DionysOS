//
// Created by dustyn on 9/14/24.
//
#include <include/definitions/string.h>
#include "vfs.h"
#include <include/mem/kalloc.h>

#include "include/arch/arch_cpu.h"

struct vnode* vnode_lookup(struct vnode* vnode, const char* path) {
}

struct vnode* vnode_create(const char* path, uint8 vnode_type) {
}


void vnode_destroy(struct vnode* vnode) {
}


struct vnode* vnode_open(const char* path) {
}


void vnode_close(struct vnode* vnode) {
}


struct vnode* vnode_mkdir(const char* path) {
}


struct vnode* vnode_mknod(const char* path) {
}


struct vnode* vnode_mkfifo(const char* path) {
}

struct vnode* find_vnode_child(struct vnode* vnode, const char* token) {
}


struct vnode* parse_path(char* path) {
    //Assign to the root node by default
    struct vnode* current_vnode = vfs_root;

    char* current_token = kalloc(VFS_MAX_PATH);

    if (path[0] != '/') {
        /* This isn't implemented yet so it will be garbage until I finish it up */
        current_vnode = current_process()->current_working_dir;
    }
    else {
        path++;
    }

    uint64 last_token = 1;

    while (last_token != LAST_TOKEN) {
        last_token = strtok(path, '/', current_token);

        current_vnode = find_vnode_child(current_vnode, current_token);
        //skip over the
    }
    return current_vnode;
}


