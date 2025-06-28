//
// Created by dustyn on 6/6/25.
//

#include "include/filesystem/vfs.h"
#include "include/device/device.h"
#include "include/device/device_filesystem.h"
#include "include/memory/kmalloc.h"

struct filesystem_info device_filesystem_info = {0};

struct vnode_operations device_filesystem_ops = {
        .lookup = device_lookup,
        .create = device_create,
        .remove = device_remove,
        .rename = device_rename,
        .write  = device_write,
        .read   = device_read,
        .link   = device_link,
        .unlink = device_unlink,
        .open   = device_open,
        .close  = device_close
};

struct device_node *device_node_alloc(struct device *device){
    struct device_node *node = kzmalloc(sizeof(struct device_node));
    node->driver = device->driver;
    node->device_major = device->device_major;
    node->device_minor = device->device_minor;
    node->device_class = device->device_class;
    node->device = device;
    node->children = kzmalloc(MAX_DEVICE_NODE_CHILDREN_SIZE);

    device->node = node;

    return node;
}

//This will have to be called after children are handled recursively
void device_node_free(struct device_node *node){
    kfree(node->children);
    node->device->node = NULL;
    kfree(node);
}

struct device* vnode_to_device(struct vnode *vnode) {
    struct device* device;

    if (vnode->vnode_type != VNODE_BLOCK_DEV && vnode->vnode_type != VNODE_CHAR_DEV && vnode->vnode_type != VNODE_NET_DEV ) {
        return NULL;
    }

    device = vnode->filesystem_object;

    //Redundant? I will probably change this later and add a system for pointer errors
    if (!device) {
        return NULL;
    }

    return device;
}


struct vnode *device_lookup(struct vnode *vnode, char *name) {
    return NULL;
}

struct vnode *device_create(struct vnode *parent, char *name, uint8_t vnode_type) {
    return NULL;
}

void device_remove(const struct vnode *vnode) {
}

void device_rename(const struct vnode *vnode, char *new_name) {
}


int64_t device_write(struct vnode *vnode, uint64_t offset, const char *buffer, uint64_t bytes) {

    struct device *device = vnode_to_device(vnode);

    if (!device) {
        return KERN_BAD_HANDLE;
    }

    return 0;
}


int64_t device_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {
    return 0;
}


struct vnode *device_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type) {
    return NULL;
}


void device_unlink(struct vnode *vnode) {
}


int64_t device_open(struct vnode *vnode) {
    return 0;
}

void device_close(struct vnode *vnode, uint64_t handle) {
}