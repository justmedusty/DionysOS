//
// Created by dustyn on 12/22/24.
//

#include "include/filesystem/tmpfs.h"


struct vnode_operations tmpfs_ops = {
    .close = tmpfs_close,
    .create = tmpfs_create,
    .link = tmpfs_link,
    .lookup = tmpfs_lookup,
    .open = tmpfs_open,
    .read = tmpfs_read,
    .remove =  tmpfs_remove,
    .rename =  tmpfs_rename,
    .unlink = tmpfs_unlink,
    .write = tmpfs_write
};

struct vnode *tmpfs_lookup(struct vnode *vnode, char *name) {

}

struct vnode *tmpfs_create(struct vnode *parent, char *name,uint8_t type) {

}

void tmpfs_rename(const struct vnode *vnode, char *name) {

}

void tmpfs_remove(const struct vnode *vnode) {

}
uint64_t tmpfs_write(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {

}


uint64_t tmpfs_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {

}

struct vnode *tmpfs_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type) {
}

void tmpfs_unlink(struct vnode *vnode) {

}

uint64_t tmpfs_open(struct vnode *vnode) {

}

void tmpfs_close(struct vnode *vnode, uint64_t handle) {

}