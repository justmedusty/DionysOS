//
// Created by dustyn on 9/14/24.
//
#include <include/definitions/string.h>
#include "vfs.h"
#include <include/drivers/uart.h>
#include <include/mem/kalloc.h>
#include "include/arch/arch_cpu.h"
#include "include/mem/mem.h"

//static prototype
static struct vnode* __parse_path(char* path);


struct vnode* vnode_lookup(char* path) {
    struct vnode* target = __parse_path(path);

    if (target == NULL) {
        return NULL;
    }

    return target;
}

struct vnode* vnode_create(char* path, uint8 vnode_type) {
    //If they include the new name in there then this could be an issue, sticking a proverbial pin in it with this comment
    //might need to change this later
    struct vnode* parent_directory = vnode_lookup(path);
    if (parent_directory == NULL) {
        //handle null response, maybe want to return something descriptive later
        return NULL;
    }

    struct vnode* new_vnode = kalloc(sizeof(struct vnode));

    parent_directory->vnode_ops->create(parent_directory, new_vnode);
}


void vnode_remove(struct vnode* vnode) {
    if (vnode->is_mount_point) {
        //should I make it a requirement to unmount before removing ?
    }
    //Probably want to be expressive with return codes so I will stick a proverbial pin in it
    vnode->vnode_ops->remove(vnode);
}

/*
 *  Open and close won't be implemented until more process and FD stuff is written
 */
struct vnode* vnode_open(const char* path) {
}


void vnode_close(struct vnode* vnode) {
}


struct vnode* find_vnode_child(struct vnode* vnode, char* token) {
    if (vnode->vnode_type != VNODE_DIRECTORY) {
        return NULL;
    }

    if (vnode->is_mount_point) {
        struct vnode* child = vnode->mounted_vnode;
    }

    if (!vnode->is_cached) {
        struct vnode* child = vnode->vnode_ops->lookup(vnode, token);
        /* Handle cache stuff when I get there */
        return child;
    }

    uint32 index = 0;

    struct vnode* child = vnode->vnode_children[index];

    /* Will I need to manually set last dirent to null to make this work properly? Maybe, will stick a pin in it in case it causes issues later */
    while (child && !strcmp(child->vnode_name, token)) {
        child = vnode->vnode_children[index++];
    }

    /* Pending the validity of my concern in the comment above being taken care of, this should automatically return NULL should the end be reached so no explicit NULL return needed */
    return child;
}

/*
 *  May want to pass a path here but will just keep a vnode for now
 */
uint64 vnode_mount(struct vnode* mount_point, struct vnode* mounted_vnode) {
    //handle already mounted case
    if (mount_point->is_mount_point) {
        return ALREADY_MOUNTED;
    }

    if (mount_point->vnode_type != VNODE_DIRECTORY) {
        //may want to return an integer to indicate what went wrong but this is ok for now
        return WRONG_TYPE;
    }

    //can you mount a mount point? I will say no for now that seems silly
    if (mounted_vnode->is_mount_point) {
        return ALREADY_MOUNTED;
    }

    mount_point->is_mount_point = TRUE;
    mount_point->mounted_vnode = mounted_vnode;
    return SUCCESS;
}

uint64 vnode_unmount(struct vnode* vnode) {
    if (!vnode->is_mount_point) {
        return NOT_MOUNTED;
    }

    vnode->is_mount_point = FALSE;
    // I will need to think about how I want to handle freeing and alloc of vnodes, yes the buffer cache handles data but the actual vnode structure I will likely want a pool and free/alloc often
    vnode->mounted_vnode = NULL;
    return SUCCESS;
}

uint64 vnode_read(struct vnode* vnode, uint64 offset, uint64 bytes) {
    if (vnode->is_mount_point) {
        return vnode->vnode_ops->read(vnode->mounted_vnode, offset, bytes);
    }
    //Let the specific impl handle this
    return vnode->vnode_ops->read(vnode, offset, bytes);
}


uint64 vnode_write(struct vnode* vnode, uint64 offset, uint64 bytes) {
    if (vnode->is_mount_point) {
        return vnode->vnode_ops->write(vnode->mounted_vnode, offset, bytes);
    }
    return vnode->vnode_ops->write(vnode->mounted_vnode, offset, bytes);
}

static struct vnode* __parse_path(char* path) {
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
    //This holds the value I chose to return from strok, it either returns 1 or 0, 0 means this token is the lasty. It is part of the altered design choice I chose
    uint64 last_token = 1;
    uint64 index = 1;

    while (last_token != LAST_TOKEN) {
        last_token = strtok(path, '/', current_token, index);
        index++;
        current_vnode = find_vnode_child(current_vnode, current_token);

        //I may want to use special codes rather than just null so we can know invalid path, node not found, wrong type, etc
        if (current_vnode == NULL) {
            return NULL;
        }

        //Clear the token to be filled again next go round, this is important
        memset(current_token, 0, VFS_MAX_PATH);
    }
    return current_vnode;
}


