//
// Created by dustyn on 9/14/24.
//

#include "vfs.h"


struct vnode *vnode_lookup(struct vnode *vnode ,const char *path) {

}

struct vnode *vnode_create(const char *path) {}


void vnode_destroy(struct vnode *vnode) {}


struct vnode *vnode_open(const char *path) {}


void vnode_close(struct vnode *vnode) {}


struct vnode *vnode_mkdir(const char *path) {}


struct vnode *vnode_mknod(const char *path) {}


struct vnode *vnode_mkfifo(const char *path) {}



/* I will wait until a ramdisk driver is written up here */
char *path_to_name_token(char *path) {
    char *token = path;
    while(*token != '\0') {

    }

}