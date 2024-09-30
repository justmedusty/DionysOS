//
// Created by dustyn on 9/18/24.
//

#include "include/data_structures/spinlock.h"
#include "include/definitions.h"
#include "include/filesystem/vfs.h"
#include "include/filesystem/tempfs.h"


struct spinlock tempfs_lock;

struct vnode_operations tempfs_vnode_ops = {
  .lookup = tempfs_lookup,
  .create = tempfs_create,
  .read = tempfs_read,
  .write = tempfs_write,
  .open = tempfs_open,
  .close = tempfs_close,
  .remove = tempfs_remove,
  .rename = tempfs_rename,
  .link = tempfs_link,
  .unlink = tempfs_unlink,
  };

void tempfs_init() {
  initlock(&tempfs_lock,TEMPFS_LOCK);
};

void tempfs_remove(struct vnode *vnode) {

}

void tempfs_rename(struct vnode *vnode,char *new_name) {

}

uint64 tempfs_read(struct vnode *vnode,uint64 offset, char *buffer, uint64 bytes) {

}

uint64 tempfs_write(struct vnode *vnode,uint64 offset, char *buffer, uint64 bytes) {

}

uint64 tempfs_stat(struct vnode *vnode,uint64 offset, char *buffer, uint64 bytes) {

}

struct vnode *tempfs_lookup(struct vnode *vnode, char *name) {

}

struct vnode *tempfs_create(struct vnode *vnode,struct vnode *new_vnode,uint8 vnode_type) {

}

void tempfs_close(struct vnode *vnode) {

}

uint64 tempfs_open(struct vnode *vnode) {

}
struct vnode *tempfs_link(struct vnode *vnode,struct vnode *new_vnode) {

}

void tempfs_unlink(struct vnode *vnode) {

}
