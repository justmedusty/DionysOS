//
// Created by dustyn on 6/6/25.
//

#include "include/filesystem/vfs.h"
#include "include/device/device.h"
#include "include/device/device_filesystem.h"
#include "include/memory/kmalloc.h"
#include "include/data_structures/binary_tree.h"
#include "include/definitions/string.h"
#include "include/drivers/display/framebuffer.h"
struct filesystem_info device_filesystem_info = {0};
struct vnode *dev_fs_root;
struct spinlock dev_fs_root_lock;

struct vnode *device_to_vnode(struct device *device);

struct vnode_operations device_filesystem_ops = {
    .lookup = device_lookup,
    .create = device_create,
    .remove = device_remove,
    .rename = device_rename,
    .write = device_write,
    .read = device_read,
    .link = device_link,
    .unlink = device_unlink,
    .open = device_open,
    .close = device_close
};

struct device_node *device_node_alloc(struct device *device) {
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
void device_node_free(struct device_node *node) {
    kfree(node->children);
    node->device->node = NULL;
    kfree(node);
}

struct device *vnode_to_device(struct vnode *vnode) {
    DEBUG_PRINT("vnode_to_device: name %s type %i fs obj %x.64\n",vnode->vnode_name,vnode->vnode_type,vnode->filesystem_object);
    if (vnode->vnode_type != VNODE_BLOCK_DEV && vnode->vnode_type != VNODE_CHAR_DEV && vnode->vnode_type !=
        VNODE_NET_DEV) {
        return NULL;
    }

    struct device *device = vnode->filesystem_object;

    //Redundant? I will probably change this later and add a system for pointer errors
    if (!device) {
        return NULL;
    }

    return device;
}

int64_t add_device_to_devfs_tree(struct device *device) {

    struct vnode *node = device_to_vnode(device);
    acquire_spinlock(&dev_fs_root_lock);
    char *name = get_device_node_name(device);

    safe_strcpy((char *) &node->vnode_name, name,128);
    kfree(name);

    if (dev_fs_root->num_children == VNODE_MAX_DIRECTORY_ENTRIES) {
       return KERN_NO_SPACE;
    }

    dev_fs_root->vnode_children[dev_fs_root->num_children++] = node;
    release_spinlock(&dev_fs_root_lock);
    return KERN_SUCCESS;
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
    vnode_rename(vnode, new_name);
}


int64_t device_write(struct vnode *vnode, uint64_t offset, const char *buffer, uint64_t bytes) {
   struct device *device = vnode_to_device(vnode);

    if (!device) {
        return KERN_BAD_HANDLE;
    }

    if (!device->driver) {
        return KERN_DEVICE_NOT_READY;
    }
        switch (device->device_major) {
            case DEVICE_MAJOR_FRAMEBUFFER:
            device->driver->device_ops->framebuffer_ops->draw_string(device,GREEN,(char *) buffer);
            return KERN_SUCCESS;
            default:
            return KERN_DEVICE_NOT_READY;
        }


    return device->driver->device_ops->vnode_ops->write(vnode, offset, buffer, bytes);
}


int64_t device_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {
    struct device *device = vnode_to_device(vnode);

    if (!device) {
        return KERN_BAD_HANDLE;
    }

    if (!device->driver) {
        return KERN_DEVICE_NOT_READY;
    }

    return device->driver->device_ops->vnode_ops->read(vnode, offset, buffer, bytes);
}


struct vnode *device_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type) {
    return NULL;
}


void device_unlink(struct vnode *vnode) {
}


int64_t device_open(struct vnode *vnode) {
    return vnode_open(vnode_get_canonical_path(vnode));
}

void device_close(struct vnode *vnode, uint64_t handle) {
    return vnode_close(handle);
}


struct vnode *device_to_vnode(struct device *device) {
    struct vnode *vnode = vnode_alloc();

    vnode->vnode_ops = &device_filesystem_ops;
    switch (device->device_major) {
        case DEVICE_MAJOR_AHCI:
        case DEVICE_MAJOR_RAMDISK:
        case DEVICE_MAJOR_TMPFS:
        case DEVICE_MAJOR_NVME:
            vnode->vnode_type = VNODE_BLOCK_DEV;
            break;

        case DEVICE_MAJOR_FRAMEBUFFER:
        case DEVICE_MAJOR_KEYBOARD:
        case DEVICE_MAJOR_MOUSE:
        case DEVICE_MAJOR_SERIAL:
        case DEVICE_MAJOR_USB_CONTROLLER:
            vnode->vnode_type = VNODE_CHAR_DEV;
            break;

        case DEVICE_MAJOR_NETWORK_CARD:
        case DEVICE_MAJOR_WIFI_ADAPTER:
            vnode->vnode_type = VNODE_NET_DEV;

        //May want some other kind of identifier because this would be invalid value
        default: vnode->vnode_type = VNODE_SPECIAL;
            break;
    }

    vnode->vnode_device_id = DEVICE_FS_FILESYSTEM_ID;


    return vnode;
}

/*
 * Function passed to for_each_node_in_tree, function pointer defined in the function prototype
 */
void tree_pluck(struct binary_tree_node *node) {
    const struct device_group *device_group = node->data.head->data;

    for (size_t i = 0; i < device_group->num_devices; i++) {
        struct device *dev = device_group->devices[i];
        add_device_to_devfs_tree(dev);
    }
}

void dev_fs_init() {
    initlock(&dev_fs_root_lock, VFS_LOCK);
    dev_fs_root = vnode_alloc();
    dev_fs_root->vnode_children = kzmalloc(sizeof(struct vnode *) * VNODE_MAX_DIRECTORY_ENTRIES);
    dev_fs_root->vnode_flags |= VNODE_CHILD_MEMORY_ALLOCATED;
    for_each_node_in_tree(&system_device_tree,tree_pluck);
    vnode_mount_path("/dev", dev_fs_root);
    kprintf("Device Filesystem Initialized\n");
}
