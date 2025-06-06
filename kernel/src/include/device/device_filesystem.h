//
// Created by dustyn on 6/6/25.
//

#ifndef DIONYSOS_DEVICE_FILESYSTEM_H
#define DIONYSOS_DEVICE_FILESYSTEM_H
#include "stdint.h"
#include "include/device/device.h"

struct device_node {
    struct device_node *parent;
    struct device_node **children;
    uint64_t device_class;
    uint64_t device_major;
    uint64_t device_minor;
    struct device_driver *driver;
    struct vnode *vnode;
};
#endif //DIONYSOS_DEVICE_FILESYSTEM_H
