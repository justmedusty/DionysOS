//
// Created by dustyn on 6/6/25.
//

#ifndef DIONYSOS_DEVICE_FILESYSTEM_H
#define DIONYSOS_DEVICE_FILESYSTEM_H
#include "stdint.h"
#include "include/device/device.h"

#define DEVICE_FS_FILESYSTEM_ID 0xF1F2F3
#define MAX_DEVICE_NODE_CHILDREN 16
#define MAX_DEVICE_NODE_CHILDREN_SIZE (sizeof(uintptr_t) * MAX_DEVICE_NODE_CHILDREN)
struct device_node {
    struct device_node *parent;
    struct device_node **children;
    uint64_t device_class;
    uint64_t device_major;
    uint64_t device_minor;
    struct device_driver *driver;
    struct vnode *vnode;
    struct device *device;
};

struct vnode *device_lookup(struct vnode *vnode, char *name);
struct vnode *device_create(struct vnode *parent, char *name, uint8_t vnode_type);
void device_remove(const struct vnode *vnode);
void device_rename(const struct vnode *vnode, char *new_name);
int64_t device_write(struct vnode *vnode, uint64_t offset, const char *buffer, uint64_t bytes);
int64_t device_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes);
struct vnode *device_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type);
void device_unlink(struct vnode *vnode);
int64_t device_open(struct vnode *vnode);
void device_close(struct vnode *vnode, uint64_t handle);





#endif //DIONYSOS_DEVICE_FILESYSTEM_H
