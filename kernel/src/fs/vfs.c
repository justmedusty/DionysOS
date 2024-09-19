//
// Created by dustyn on 9/14/24.
//

#include "vfs.h"

#include <include/mem/kalloc.h>

#include "include/arch/arch_cpu.h"

struct vnode *vnode_lookup(struct vnode *vnode ,const char *path) {

}

struct vnode *vnode_create(const char *path, uint8 vnode_type) {

}


void vnode_destroy(struct vnode *vnode) {}


struct vnode *vnode_open(const char *path) {}


void vnode_close(struct vnode *vnode) {}


struct vnode *vnode_mkdir(const char *path) {}


struct vnode *vnode_mknod(const char *path) {}


struct vnode *vnode_mkfifo(const char *path) {}

struct vnode *find_vnode_child(struct vnode *vnode, const char *token) {

}


struct vnode *parse_path(char *path) {

    struct vnode *base = vfs_root;
    char *current_token; //= kalloc();

    if(path[0] != '/') {
        /* This isn't implemented yet so it will be garbage until I finish it up */
        base = current_process()->current_working_dir;
    }else {
        path++;
    }

    uint32 index = 0;

    while(path != '\0') {
        current_token[index] = path[index];
        index++;

        if(path[index + 1] == '/') {
            base = find_vnode_child(base, current_token);
            //skip over the
            path += 2;
        }

        path++;

    }

}

char *get_next_token(char *token) {

}