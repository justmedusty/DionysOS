//
// Created by dustyn on 9/14/24.
//
#include "include/filesystem/vfs.h"
#include <include/definitions/string.h>
#include <include/data_structures/singly_linked_list.h>
#include <include/drivers/serial/uart.h>
#include <include/mem/kalloc.h>
#include "include/arch/arch_cpu.h"
#include "include/mem/mem.h"

//Used to hold the free nodes in the static pool
struct singly_linked_list vnode_static_pool = {};
//unused so far but may use this in case loose nodes not being freed becomes a problem
struct singly_linked_list vnode_used_pool = {};

//static pool
struct vnode static_vnode_pool[50];

//Root node
struct vnode vfs_root;

//VFS lock
struct spinlock vfs_lock;

//static prototype
static struct vnode* __parse_path(char* path);


void vfs_init() {
    singly_linked_list_init(&vnode_static_pool, 0);
    singly_linked_list_init(&vnode_used_pool, 0);

    for (int i = 0; i < 50; i++) {
        singly_linked_list_insert_head(&vnode_used_pool, &static_vnode_pool[i]);
        //Mark each one as being part of the static pool
        static_vnode_pool[i].vnode_flags |= VNODE_STATIC_POOL;
    }
    initlock(&vfs_lock,VFS_LOCK);
    serial_printf("VFS initialized\n");
}


/* To make use of the static pool */
struct vnode* vnode_alloc() {
    acquire_spinlock(&vfs_lock);

    if (vnode_static_pool.head == NULL) {
        struct vnode* new_node = kalloc(sizeof(struct vnode));
        release_spinlock(&vfs_lock);
        return new_node;
    }

    struct vnode* vnode = vnode_static_pool.head->data;
    singly_linked_list_remove_head(&vnode_static_pool);
    release_spinlock(&vfs_lock);
    return vnode;
}


/* To make use of the static pool */
void vnode_free(struct vnode* vnode) {
    acquire_spinlock(&vfs_lock);

    if (vnode == NULL) {
        release_spinlock(&vfs_lock);
        return;
    }

    //Do not free if any processes either have this open or it is their CWD
    if (vnode->vnode_active_references != 0) {
        release_spinlock(&vfs_lock);
        return;
    }

    if (vnode->is_mount_point) {
        //No freeing mount points
        release_spinlock(&vfs_lock);
        return;
    }

    /* If it is part of the static pool, put it back, otherwise free it  */
    if (vnode->vnode_flags & VNODE_STATIC_POOL) {
        singly_linked_list_insert_tail(&vnode_static_pool, vnode);
        release_spinlock(&vfs_lock);
        return;
    }

    //don't try to free the root
    if (vnode == &vfs_root) {
        release_spinlock(&vfs_lock);
        return;
    }

    kfree(vnode);
    release_spinlock(&vfs_lock);
}


struct vnode* vnode_lookup(char* path) {
    struct vnode* target = __parse_path(path);

    if (target == NULL) {
        vnode_free(target);
        //sort of redundant isn't it, I will probably change this later
        return NULL;
    }

    return target;
}


/*
 *  This is going to need to take more information
 */
struct vnode* vnode_create(char* path, uint8 vnode_type) {
    //If they include the new name in there then this could be an issue, sticking a proverbial pin in it with this comment
    //might need to change this later
    struct vnode* parent_directory = vnode_lookup(path);
    if (parent_directory == NULL) {
        //handle null response, maybe want to return something descriptive later
        return NULL;
    }

    struct vnode* new_vnode = vnode_alloc();

    new_vnode = parent_directory->vnode_ops->create(parent_directory, new_vnode, vnode_type);
    return new_vnode;
}


void vnode_remove(struct vnode* vnode) {
    if (vnode->is_mount_point) {
        //should I make it a requirement to unmount before removing ?
    }
    //Probably want to be expressive with return codes so I will stick a proverbial pin in it
    //Should I free the memory here or in the specific function? Not sure yet
    vnode->vnode_ops->remove(vnode);
    vnode_free(vnode);
}


struct vnode* vnode_open(char* path) {
    struct vnode* vnode = vnode_alloc();
    vnode = vnode_lookup(path);

    if (vnode == NULL) {
        return NULL;
    }

    if (vnode->is_mount_point) {
        vnode = vnode->mounted_vnode;
    }
    //should probably return an FD but sticking a pin in it for now
    return vnode;
}


void vnode_close(struct vnode* vnode) {
}


struct vnode* find_vnode_child(struct vnode* vnode, char* token) {

    if (vnode->vnode_type != VNODE_DIRECTORY) {
        vnode_free(vnode);
        return NULL;
    }

    if (vnode->is_mount_point) {
        vnode = vnode->mounted_vnode;
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


uint64 vnode_read(struct vnode* vnode, uint64 offset, uint64 bytes, char* buffer) {
    if (vnode->is_mount_point) {
        return vnode->vnode_ops->read(vnode->mounted_vnode, offset,buffer, bytes);
    }
    //Let the specific impl handle this
    return vnode->vnode_ops->read(vnode, offset,buffer, bytes);
}


uint64 vnode_write(struct vnode* vnode, uint64 offset, uint64 bytes, char* buffer) {
    if (vnode->is_mount_point) {
        return vnode->vnode_ops->write(vnode->mounted_vnode, offset,buffer, bytes);
    }
    return vnode->vnode_ops->write(vnode->mounted_vnode, offset,buffer, bytes);
}


static struct vnode* __parse_path(char* path) {
    //Assign to the root node by default
    struct vnode* current_vnode = vnode_alloc();
    current_vnode = &vfs_root;

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
            kfree(current_token);
            return NULL;
        }

        //Clear the token to be filled again next go round, this is important
        memset(current_token, 0, VFS_MAX_PATH);
    }
    kfree(current_token);
    return current_vnode;
}


