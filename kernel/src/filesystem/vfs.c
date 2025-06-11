//
// Created by dustyn on 9/14/24.
//
#include "include/filesystem/vfs.h"

#include <include/data_structures/doubly_linked_list.h>
#include <include/definitions/string.h>
#include <include/data_structures/singly_linked_list.h>
#include <include/drivers/display/framebuffer.h>
#include <include/drivers/serial/uart.h>
#include <include/memory/kmalloc.h>
#include "include/architecture/arch_cpu.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_timer.h"


//Used to hold the free nodes in the static pool
struct singly_linked_list vnode_static_pool = {};
//unused so far but may use this in case loose nodes not being freed becomes a problem
struct singly_linked_list vnode_used_pool = {};

//static pool
struct vnode static_vnode_pool[VFS_STATIC_POOL_SIZE];

//Root node
struct vnode vfs_root;

//VFS lock
struct spinlock vfs_lock;
struct spinlock list_lock;

//static prototype
static struct vnode *parse_path(char *path);

static int64_t vnode_update_children_array(const struct vnode *vnode);

static int8_t get_new_file_handle(struct virtual_handle_list *list);

/*
 *The vfs_init function simply initializes a linked list of all of the vnode static pool
 *and marks each of them with a VNODE_STATIC_POOL flag
 *
 *It also initializes the lock for the VFS system.
 */
void vfs_init() {
    kprintf("Initializing Virtual Filesystem...\n");
    singly_linked_list_init(&vnode_static_pool, 0);
    singly_linked_list_init(&vnode_used_pool, 0);

    for (int i = 0; i < VFS_STATIC_POOL_SIZE; i++) {
        singly_linked_list_insert_head(&vnode_used_pool, &static_vnode_pool[i]);
        //Mark each one as being part of the static pool
        static_vnode_pool[i].vnode_flags |= VNODE_STATIC_POOL;
    }
    initlock(&vfs_lock, VFS_LOCK);
    initlock(&list_lock, VFS_LOCK);
    memset(&vfs_root, 0, sizeof(struct vnode));
    vfs_root.vnode_inode_number = 0;
    vfs_root.vnode_type = VNODE_DIRECTORY;
    vfs_root.vnode_parent = NULL;
    vfs_root.vnode_ops = NULL;
    vfs_root.vnode_filesystem_id = VNODE_FS_VNODE_ROOT;
    vfs_root.node_lock = kmalloc(sizeof (struct spinlock));
    kprintf("Virtual Filesystem Initialized\n");
    serial_printf("VFS initialized\n");
}

/*
 *  vnode_alloc is required with the static pool so that nodes may be taken from the pool if any are free,
 *  otherwise kalloc is invoked.
 */
struct vnode *vnode_alloc() {
    acquire_spinlock(&list_lock);
    if (vnode_static_pool.head == NULL) {
        struct vnode *new_node = kmalloc(sizeof(struct vnode));
        DEBUG_PRINT("VNODE ALLOC %x.64\n", new_node);
        release_spinlock(&list_lock);
        new_node->node_lock = kzmalloc(sizeof(struct spinlock));
        initlock(new_node->node_lock, VFS_LOCK);
        return new_node;
    }

    struct vnode *vnode = vnode_static_pool.head->data;
    singly_linked_list_remove_head(&vnode_static_pool);
    DEBUG_PRINT("VNODE ALLOC %x.64\n", vnode);
    vnode->node_lock = kzmalloc(sizeof(struct spinlock));
    initlock(vnode->node_lock,VFS_LOCK);
    release_spinlock(&list_lock);
    return vnode;
}

void vnode_directory_alloc_children(struct vnode *vnode) {
    if (vnode->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED) {
        return;
    }
    vnode->vnode_children = kmalloc(sizeof(struct vnode *) * VNODE_MAX_DIRECTORY_ENTRIES);
    vnode->vnode_flags |= VNODE_CHILD_MEMORY_ALLOCATED;

    for (size_t i = 0; i < VNODE_MAX_DIRECTORY_ENTRIES; i++) {
        vnode->vnode_children[i] = NULL;
    }
}

/*
 * Same as above except for freeing
 *
 * Checks if mount point,
 * Checks if ref_count not 0,
 * Checks if part of static pool
 * Checks if the node is root
 */
void vnode_free(struct vnode *vnode) {
    acquire_spinlock(&list_lock);

    if (vnode == NULL) {
        release_spinlock(&list_lock);
        return;
    }

    //Do not free if any processes either have this open or it is their CWD
    if (vnode->vnode_active_references != 0) {
        release_spinlock(&list_lock);
        return;
    }

    if (vnode->is_mount_point) {
        //No freeing mount points
        release_spinlock(&list_lock);
        return;
    }

    /* If it is part of the static pool, put it back, otherwise free it  */
    if (vnode->vnode_flags & VNODE_STATIC_POOL) {
        kfree(vnode->node_lock);
        singly_linked_list_insert_tail(&vnode_static_pool, vnode);
        release_spinlock(&list_lock);
        return;
    }

    //don't try to free the root
    if (vnode == &vfs_root) {
        release_spinlock(&list_lock);
        return;
    }

    if (vnode->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED) {
        kfree(vnode->vnode_children);
    }
    kfree(vnode->node_lock);
    kfree(vnode);
    release_spinlock(&list_lock);
}

/*
 *This is the public facing function that makes use of the internal parse_path function
 *Takes a string path and returns the final vnode, if it is a valid path. Can take
 *absolute or relative path.
 */
struct vnode *vnode_lookup(char *path) {
    struct vnode *target = parse_path(path);
    return target;
}

struct vnode *vnode_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type) {
    if (vnode->vnode_type != VNODE_DIRECTORY) {
        return NULL;
    }

    if (new_vnode->vnode_type == VNODE_HARD_LINK || new_vnode->vnode_type == VNODE_SYM_LINK) {
        return NULL; // no links to other links
    }

    if (type != VNODE_HARD_LINK && type != VNODE_SYM_LINK) {
        return NULL;
    }

    struct vnode *child = vnode->vnode_ops->create(vnode, new_vnode->vnode_name, type);
    child->filesystem_info = vnode->filesystem_info;
    return child;
}

int64_t vnode_unlink(struct vnode *link) {
    if (link->vnode_type != VNODE_SYM_LINK && link->vnode_type != VNODE_HARD_LINK) {
        return KERN_WRONG_TYPE;
    }

    link->vnode_ops->unlink(link);
    return KERN_SUCCESS;
}

struct vnode *vnode_create_special(struct vnode *parent, char *name, uint8_t vnode_type) {
    struct vnode *new_vnode = vnode_alloc();
    memset(new_vnode, 0, sizeof(struct vnode));
    safe_strcpy(new_vnode->vnode_name, name, VFS_MAX_NAME_LENGTH);
    switch (vnode_type) {
        case VNODE_BLOCK_DEV:
        case VNODE_CHAR_DEV:
        case VNODE_NET_DEV:
        case VNODE_SPECIAL_FILE:
        case VNODE_SPECIAL:
            break;
        default:
            panic("vnode_create_special: unknown vnode_type");
    }

    return new_vnode;
}

/*
 *  Create a vnode, invokes lookup , allocated a new vnode, and calls create on the parent.
 */
struct vnode *vnode_create(char *path, char *name, uint8_t vnode_type) {
    //If they include the new name in there then this could be an issue, sticking a proverbial pin in it with this comment
    //might need to change this later
    struct vnode *parent_directory = vnode_lookup(path);

    if (parent_directory == NULL) {
        warn_printf("PATH NOT VALID %s\n", path);
        //handle null response, maybe want to return something descriptive later
        return NULL;
    }

    if (parent_directory->is_mount_point) {
        parent_directory = parent_directory->mounted_vnode;
        panic("mount");
    }

    if (!(parent_directory->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED)) {
        vnode_directory_alloc_children(parent_directory);
    }

    struct vnode *new_vnode;

    if (vnode_type == VNODE_DIRECTORY || vnode_type == VNODE_FILE) {
        new_vnode = parent_directory->vnode_ops->create(parent_directory, name, vnode_type);
    } else {
        new_vnode = vnode_create_special(parent_directory, name, vnode_type);
    }

    if (vnode_type == VNODE_DIRECTORY) {
        new_vnode->is_cached = true;
    }

    new_vnode->vnode_parent = parent_directory;

    parent_directory->vnode_children[parent_directory->num_children++] = new_vnode;
    return new_vnode;
}

/*
 *This simply invokes the remove function pointer which is filesystem specific, afterward invokes vnode_free
*/
int32_t vnode_remove(struct vnode *vnode, char *path) {
    struct vnode *target;

    if (vnode != NULL) {
        target = vnode;
    } else if ((target = vnode_lookup(path)) == NULL) {
        return KERN_NOT_FOUND;
    }


    if (target->is_mount_point) {
        panic("Cannot delete mount point without first unmounting\n");
        // need to put logic here just putting this as a placeholder
    }

    if (vnode->vnode_active_references != 0) {
        release_spinlock(&vfs_lock);
        return KERN_BUSY;
    }

    target->vnode_ops->remove(target);
    vnode_update_children_array(target);
    vnode_free(target);
    return KERN_SUCCESS;
}

/*
 *  Open invokes vnode_open and checks for mounts in the target.
 *
 *  It then returns the vnode handle tied to the calling process.
 *
 *  This will need to return a handle at some point.
 */
int64_t vnode_open(char *path) {
    struct process *process = current_process();

    struct virtual_handle_list *list = process->handle_list;
    /* At the time of writing this I have not fleshed this out yet but I think this will be allocated on creation so no need to null check it */

    const int8_t ret = get_new_file_handle(list);

    if (ret < 0) {
        return KERN_MAX_REACHED;
    }

    struct vnode *vnode = vnode_lookup(path);

    if (vnode == NULL) {
        return KERN_NOT_FOUND;
    }

    if(vnode->filesystem_info){
        vnode->filesystem_info->filesystem_reference_count++;
    }

    vnode->vnode_refcount++;

    if (vnode->is_mount_point) {
        vnode = vnode->mounted_vnode;
    }

    struct virtual_handle *new_handle = kmalloc(sizeof(struct virtual_handle));
    memset(new_handle, 0, sizeof(struct virtual_handle));
    new_handle->vnode = vnode;
    new_handle->process = process;
    new_handle->offset = 0;
    new_handle->handle_id = (uint64_t)
    ret;

    doubly_linked_list_insert_head(list->handle_list, new_handle);
    list->num_handles++;
    return ret;
}


void vnode_close(uint64_t handle) {
    struct process *process = my_cpu()->running_process;
    struct virtual_handle_list *list = process->handle_list;
    struct doubly_linked_list_node *node = list->handle_list->head;


    while (node != NULL) {
        struct virtual_handle *virtual_handle = (struct virtual_handle *) node->data;
        if (virtual_handle->handle_id == handle) {
            virtual_handle->vnode->vnode_ops->close(virtual_handle->vnode, handle);
            virtual_handle->vnode->vnode_refcount--;

            if(virtual_handle->vnode->filesystem_info){
                virtual_handle->vnode->filesystem_info->filesystem_reference_count--;
            }

            kfree(virtual_handle);
            doubly_linked_list_remove_node_by_address(list->handle_list, node);
            return;
        }
    }
    panic("vnode_close invalid handle identifier");
}

void vnode_rename(struct vnode *vnode, char *new_name) {
    safe_strcpy(vnode->vnode_name, new_name, VFS_MAX_NAME_LENGTH);
    /*
     * If we were passed a string that is too long, just truncate it
     */
    if (strlen(new_name) > VFS_MAX_NAME_LENGTH) {
        vnode->vnode_name[VFS_MAX_NAME_LENGTH - 1] = '\0';
    }

    vnode->vnode_ops->rename(vnode, vnode->vnode_name);
}

/*
 *  This function is for finding directory entries and following a path, only a fraction of the path is passed as the token parameter.
 *
 *  It ensures the type is correct (directory)
 *
 *  It accounts for mount points
 *
 *  It checks if the child is cached, otherwise it looks it up.
 *
 *  It iterates through the children in the vnode until it finds the child or the end of the children.
 */
struct vnode *find_vnode_child(struct vnode *vnode, char *token) {
    acquire_spinlock(&vfs_lock);

    if (vnode->vnode_type != VNODE_DIRECTORY) {
        release_spinlock(&vfs_lock);
        return NULL;
    }

    if (vnode->is_mount_point) {
        vnode = vnode->mounted_vnode;
    }

    size_t index = 0;

    if (vnode->is_cached == false && vnode->num_children == 0) {
        warn_printf("NOT CACHED %s!\n", vnode->vnode_name);
        struct vnode *child = vnode->vnode_ops->lookup(vnode, token);
        vnode->is_cached = true;
        release_spinlock(&vfs_lock);
        return child;
    }

    if (!(vnode->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED)) {
        vnode_directory_alloc_children(vnode);
    }

    struct vnode *child = vnode->vnode_children[index];
    /* Will I need to manually set last dirent to null to make this work properly? Maybe, will stick a pin in it in case it causes issues later */
    while (index < vnode->num_children) {
        if (child && (safe_strcmp(child->vnode_name, token, VFS_MAX_NAME_LENGTH))) {
            release_spinlock(&vfs_lock);
            return child;
        }

        child = vnode->vnode_children[++index];
    }
    release_spinlock(&vfs_lock);
    DEBUG_PRINT("find_vnode_child: NULL RETURN\n");
    return NULL;;
}


/*
 * Mounts a vnode, will mark the target as a mount_point and link the vnode that is now mounted on it
 */
int64_t vnode_mount(struct vnode *mount_point, struct vnode *mounted_vnode) {
    acquire_spinlock(&vfs_lock);
    //handle already mounted case
    if (mount_point->is_mount_point) {
        release_spinlock(&vfs_lock);
        panic("here");
        return KERN_EXISTS;
    }

    if (mount_point->vnode_type != VNODE_DIRECTORY) {
        //may want to return an integer to indicate what went wrong but this is ok for now
        release_spinlock(&vfs_lock);
        panic("wrong type");
        return KERN_WRONG_TYPE;
    }


    //can you mount a mount point? I will say no for now that seems silly
    if (mounted_vnode->is_mount_point) {
        release_spinlock(&vfs_lock);
        panic("here");
        return KERN_EXISTS;
    }

    mounted_vnode->vnode_parent = mount_point;
    mounted_vnode->is_mounted = true;

    mount_point->is_mount_point = true;
    mount_point->mounted_vnode = mounted_vnode;

    //Set cached to true otherwise on lookup the entire array of children dentries will be queried and the mount will be removed
    mount_point->is_cached = true;
    release_spinlock(&vfs_lock);
    return KERN_SUCCESS;
}

/*
 * Clears mount data from the vnode, clearing it as a mount point.
 */
int64_t vnode_unmount(struct vnode *vnode) {
    acquire_spinlock(&vfs_lock);
    if (!vnode->is_mount_point) {
        release_spinlock(&vfs_lock);
        return KERN_NOT_FOUND;
    }
    //only going to allow mounting of whole filesystems so this works
    vnode->mounted_vnode->vnode_parent = NULL;
    vnode->is_mount_point = false;
    // I will need to think about how I want to handle freeing and alloc of vnodes, yes the buffer cache handles data but the actual vnode structure I will likely want a pool and free/alloc often
    vnode->mounted_vnode = NULL;

    //Set cached to false so that next time this vnode is queried it will fill its children array
    vnode->is_cached = false;
    release_spinlock(&vfs_lock);
    return KERN_SUCCESS;
}

/*
 * Mounts a vnode, will mark the target as a mount_point and link the vnode that is now mounted on it
 */
int64_t vnode_mount_path(char *mount_point_path, struct vnode *mounted_vnode) {
    acquire_spinlock(&vfs_lock);

    struct vnode *mount_point = parse_path(mount_point_path);
    //handle already mounted case
    if (mount_point->is_mount_point) {
        release_spinlock(&vfs_lock);
        panic("here");
        return KERN_EXISTS;
    }

    if (mount_point->vnode_type != VNODE_DIRECTORY) {
        //may want to return an integer to indicate what went wrong but this is ok for now
        release_spinlock(&vfs_lock);
        panic("wrong type");
        return KERN_WRONG_TYPE;
    }


    //can you mount a mount point? I will say no for now that seems silly
    if (mounted_vnode->is_mount_point) {
        release_spinlock(&vfs_lock);
        panic("here");
        return KERN_EXISTS;
    }

    mounted_vnode->vnode_parent = mount_point;
    mounted_vnode->is_mounted = 1;

    mount_point->is_mount_point = 1;
    mount_point->mounted_vnode = mounted_vnode;

    //Set cached to true otherwise on lookup the entire array of children dentries will be queried and the mount will be removed
    mount_point->is_cached = true;
    release_spinlock(&vfs_lock);
    return KERN_SUCCESS;
}

/*
 * Clears mount data from the vnode, clearing it as a mount point.
 */
int64_t vnode_unmount_path(char *path) {
    struct vnode *vnode = parse_path(path);
    acquire_spinlock(&vfs_lock);
    if (!vnode->is_mount_point) {
        release_spinlock(&vfs_lock);
        return KERN_NOT_FOUND;
    }
    //only going to allow mounting of whole filesystems so this works
    vnode->mounted_vnode->vnode_parent = NULL;
    vnode->is_mount_point = false;
    // I will need to think about how I want to handle freeing and alloc of vnodes, yes the buffer cache handles data but the actual vnode structure I will likely want a pool and free/alloc often
    vnode->mounted_vnode = NULL;

    //Set cached to false so that next time this vnode is queried it will fill its children array
    vnode->is_cached = false;
    release_spinlock(&vfs_lock);
    return KERN_SUCCESS;
}


/*
 * Invokes the read function pointer. Filesystem dependent.
 *
 * Handles mount points properly.
 */


int64_t vnode_read(struct vnode *vnode, const uint64_t offset, uint64_t bytes, char *buffer) {
    DEBUG_PRINT("HERE\n");
    acquire_spinlock(vnode->node_lock);
    if (vnode->is_mount_point) {
        panic("reading a directory");
    }

    // If the passed size is 0 read the whole thing
    if (bytes == 0) {
        bytes = vnode->vnode_size;
    }
    //Let the specific impl handle this
    const int64_t ret = vnode->vnode_ops->read(vnode, offset, buffer, bytes);
    release_spinlock(vnode->node_lock);
    return ret;
}


int64_t vnode_write(struct vnode *vnode, const uint64_t offset, const uint64_t bytes, const char *buffer) {
    DEBUG_PRINT("THERE\n");
    acquire_spinlock(vnode->node_lock);
    if (vnode->is_mount_point) {
        panic("writing to a directory");
    }
    int32_t ret =vnode->vnode_ops->write(vnode, offset, buffer, bytes);
    DEBUG_PRINT("DONE\n");
    release_spinlock(vnode->node_lock);
    DEBUG_PRINT("DONE2\n");
    return ret;
}

/*
 * Gets a canonical path, whether it be for a symlink or something git pu
 */
char *vnode_get_canonical_path(struct vnode *vnode) {
    //global lock to ensure that nothing gets changed or deleted under us
    acquire_spinlock(&vfs_lock);
    char *buffer = kzmalloc(PAGE_SIZE);
    char *final_buffer = kzmalloc(PAGE_SIZE);
    struct vnode *pointer = vnode;


    while (pointer && pointer != &vfs_root) {

        if (pointer->is_mounted) {
            pointer = pointer->vnode_parent;

        }

        strcat(buffer, "/");

        /*
         * This check exists because otherwise you end up with // when you get to root, which will cause issues with the way I implemented searching
         */
        if (*pointer->vnode_name != '/') {
            strcat(buffer, pointer->vnode_name);
        }
        pointer = pointer->vnode_parent;
    }

    const size_t token_length = strtok_count(buffer, '/');

    char *temp = kmalloc(PAGE_SIZE);
    memset(temp, 0, PAGE_SIZE);

    for (size_t i = token_length + 1; i > 0; i--) {
        strtok(buffer, '/', temp, i);
        strcat(final_buffer, "/");
        strcat(final_buffer, temp);
        memset(temp, 0, strlen(temp));
        // a bit heavy? I will leave it for now Id rather this then the 1024 byte stack allocs since that is a risky move
    }

    kfree(buffer);
    kfree(temp);
    release_spinlock(&vfs_lock);
    return final_buffer;
}

/*
 * This function makes uses of my in-house strtok function in order to fully parse a path and return a vnode for the specified
 * file or device.
 *
 * It checks whether the path is relative or absolute
 *
 * It parses token then finds the corresponding node by token invoking find_vnode_child.
 *
 * When it is done it frees the token memory and ensures that the final vnode is not NULL
 *
 * Returns target node.
 *
 */
static struct vnode *parse_path(char *path) {
    //Assign to the root node by default
    acquire_spinlock(&vfs_lock);
    struct vnode *current_vnode = &vfs_root;

    if (current_vnode->is_mount_point) {
        current_vnode = current_vnode->mounted_vnode;
    }

    char *current_token = kmalloc(VFS_MAX_NAME_LENGTH);

    if (path[0] != '/') {
        current_vnode = current_process()->current_working_dir;
    } else {
        path++;
    }

    while (*path == '/') {
        path++;
    }

    if (!*path) {
        return current_vnode;
    }
    //This holds the value I chose to return from strok, it either returns 1 or 0, 0 means this token is the last. It is part of the altered design choice I chose
    uint64_t last_token = 1;
    uint64_t index = 1;

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
        memset(current_token, 0, VFS_MAX_NAME_LENGTH);
    }

    if (current_vnode->is_mount_point) {
        current_vnode = current_vnode->mounted_vnode;
    }

    kfree(current_token);
    release_spinlock(&vfs_lock);
    return current_vnode;
}

//simple iteration
static int8_t get_new_file_handle(struct virtual_handle_list *list) {

    for (int8_t i = 0; i < NUM_HANDLES; i++) {
        if (!(list->handle_id_bitmap & BIT(i))) {
            list->handle_id_bitmap |= BIT(i);

            return i;
        }
    }

    return KERN_MAX_REACHED;
}


static int64_t vnode_update_children_array(const struct vnode *vnode) {
    acquire_spinlock(&vfs_lock);
    struct vnode *parent = vnode->vnode_parent;
    size_t index = 0;
    uint64_t size = parent->num_children;
    for (index = 0; index < size; index++) {
        if (parent->vnode_children[index] == vnode) {
            parent->vnode_children[index] = NULL;
            /* This just ensures that if its the last entry it will be zerod still*/
            parent->vnode_children[index] = parent->vnode_children[size];
            parent->num_children--;
            release_spinlock(&vfs_lock);
            return KERN_SUCCESS;
        }
    }
    panic("Vnode does not exist"); // panic since this means invalid state and needs to be investigated
    // unreachable code but makes clang-tidy shut up
    release_spinlock(&vfs_lock);
    return KERN_NOT_FOUND;
}


void vlock(struct vnode *vnode) {
    acquire_spinlock(vnode->node_lock);
}

void vunlock(struct vnode *vnode) {
    release_spinlock(vnode->node_lock);
}

struct vnode *handle_to_vnode(uint64_t handle_id) {
    struct vnode *ret;
    struct doubly_linked_list_node *node = current_process()->handle_list->handle_list->head;

    while (node != NULL) {
        const struct virtual_handle *handle = node->data;
        if (handle->handle_id == handle_id) {
            return handle->vnode;
        }
        node = node->next;
    }

    return NULL;
}

int64_t handle_to_offset(uint64_t handle_id) {
    const struct doubly_linked_list_node *node = current_process()->handle_list->handle_list->head;

    do {
        const struct virtual_handle *handle = node->data;
        if (handle->handle_id == handle_id) {
            return (int64_t) handle->offset;
        }
        node = node->next;
    } while (node->next != NULL);

    return KERN_NOT_FOUND;
}

bool is_valid_vnode_type(uint64_t type) {
    switch (type) {
        VALID_VNODE_CASES
            return true;
        default:
            return false;
    }
}


struct virtual_handle *find_handle(int64_t handle_id) {
    struct doubly_linked_list_node *node = current_process()->handle_list->handle_list->head;

    if (!node || node->data == NULL) {
        return NULL;
    }

    struct virtual_handle *handle = node->data;

    while (node->data && handle->handle_id != (uint64_t) handle_id) {
        node = node->next;
        if (!node) {
            return NULL;
        }
        handle = node->data;
    }

    if (handle == NULL) {
        return NULL;
    }

    return handle;
}

/*
 *  These will be the actual exposed functions that system calls invoke, kthreads and use etc. The above functions operating on vnodes
 *  these will operate on handles
 */

int64_t write(uint64_t handle, char *buffer, uint64_t bytes) {
    struct vnode *handle_vnode = handle_to_vnode(handle);
    if (!handle_vnode) {
        return KERN_NOT_FOUND;
    }

    const int64_t offset = handle_to_offset(handle);

    if (offset < 0) {
        return offset;
    }


    return vnode_write(handle_vnode, offset, bytes, buffer);
}

int64_t read(uint64_t handle, char *buffer, uint64_t bytes) {
    struct vnode *handle_vnode = handle_to_vnode(handle);
    if (!handle_vnode) {
        return KERN_NOT_FOUND;
    }

    const int64_t offset = handle_to_offset(handle);

    if (offset < 0) {
        return KERN_NOT_FOUND;
    }

    return vnode_read(handle_vnode, offset, bytes, buffer);
}


int64_t open(char *path) {
    const int64_t handle = vnode_open(path);

    if (handle < 0) {
        return KERN_NOT_FOUND;
    }

    return handle;
}

void close(uint64_t handle) {
    vnode_close(handle);
}

int64_t mount(char *mount_point, char *mounted_filesystem) {
    struct vnode *vnode = vnode_lookup(mount_point);

    if (!vnode) {
        return KERN_NOT_FOUND;
    }

    struct vnode *vnode_to_mount = vnode_lookup(mounted_filesystem);

    if (!vnode_to_mount) {
        return KERN_NOT_FOUND;
    }

    const int64_t ret = vnode_mount(vnode, vnode_to_mount);

    if (ret < 0) {
        return ret;
    }

    return KERN_SUCCESS;
}

int64_t unmount(char *path) {
    struct vnode *mount_point = vnode_lookup(path);

    if (!mount_point) {
        return KERN_NOT_FOUND;
    }

    int64_t ret = vnode_unmount(mount_point);

    if (ret < 0) {
        return ret;
    }

    return KERN_SUCCESS;
}

int64_t rename(char *path, char *new_name) {
    struct vnode *vnode = vnode_lookup(path);

    if (!vnode) {
        return KERN_NOT_FOUND;
    }

    vnode_rename(vnode, new_name);
    return KERN_SUCCESS;
}

int64_t create(char *path, char *name, uint64_t type) {
    if (safe_strlen(name, VFS_MAX_NAME_LENGTH) < 0) {
        return KERN_OVERFLOW;
    }

    if (!is_valid_vnode_type(type)) {
        return KERN_WRONG_TYPE;
    }

    struct vnode *vnode = vnode_create(path, name, type);

    if (vnode == NULL) {
        return KERN_UNEXPECTED;
    }

    return KERN_SUCCESS;
}

int64_t get_size(uint64_t handle) {
    struct vnode *handle_vnode = handle_to_vnode(handle);
    if (!handle_vnode) {
        return KERN_NOT_FOUND;
    }
    uint64_t size = handle_vnode->vnode_size;
    return (int64_t) size;
}

int64_t seek(uint64_t handle, uint64_t whence) {
    struct virtual_handle *vhandle = find_handle(handle);

    if (!vhandle) {
        return KERN_NOT_FOUND;
    }

    switch (whence) {
        case SEEK_BEGIN:
            vhandle->offset = 0;
            break;
        case SEEK_END:
            vhandle->offset = get_size(vhandle->offset);
            break;

        default:
            return KERN_INVALID_ARG;
    }

    return KERN_SUCCESS;
}
