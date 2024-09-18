//
// Created by dustyn on 9/14/24.
//

#include "vfs.h"
#include "include/arch/arch_cpu.h"

struct vnode *vnode_lookup(struct vnode *vnode ,const char *path) {

}

struct vnode *vnode_create(const char *path) {}


void vnode_destroy(struct vnode *vnode) {}


struct vnode *vnode_open(const char *path) {}


void vnode_close(struct vnode *vnode) {}


struct vnode *vnode_mkdir(const char *path) {}


struct vnode *vnode_mknod(const char *path) {}


struct vnode *vnode_mkfifo(const char *path) {}



struct vnode *parse_path(char *path) {

    struct vnode *base = vfs_root;

    if(path[0] != '/') {
        base = current_process()->current_working_dir;
    }
    char *token = path;
    while(*token != '/') {
        path++;

    }

}