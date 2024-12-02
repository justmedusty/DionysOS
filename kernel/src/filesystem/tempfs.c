//
// Created by dustyn on 9/18/24.
//

#include "include/data_structures/spinlock.h"
#include "include/definitions.h"
#include "include/filesystem/vfs.h"
#include "include/filesystem/tempfs.h"

#include <math.h>
#include <strings.h>
#include <include/definitions/string.h>
#include <include/drivers/serial/uart.h>
#include <include/mem/kalloc.h>
#include "include/drivers/block/ramdisk.h"
#include "include/mem/mem.h"

/*
 * We need to ensure that for each filesystem we have a separate lock
 * We'll only implement the one filesystem for now.
 */
#define NUM_FILESYSTEM_OBJECTS 10
struct spinlock tempfs_lock[10];
struct tempfs_superblock tempfs_superblock[10];
struct tempfs_filesystem tempfs_filesystem[10] = {
    [0] = {
        .filesystem_id = INITRAMFS,
        .lock = &tempfs_lock[0],
        .superblock = &tempfs_superblock[0],
        .ramdisk_id = INITRAMFS
    }
};

static void tempfs_clear_bitmap(struct tempfs_filesystem* fs, uint8_t type, uint64_t number);
static void tempfs_get_free_inode_and_mark_bitmap(struct tempfs_filesystem* fs,
                                                  struct tempfs_inode* inode_to_be_filled);
static uint64_t tempfs_get_free_block_and_mark_bitmap(struct tempfs_filesystem* fs);
static uint64_t tempfs_free_block_and_mark_bitmap(struct tempfs_filesystem* fs, uint64_t block_number);
static uint64_t tempfs_free_inode_and_mark_bitmap(struct tempfs_filesystem* fs, uint64_t inode_number);
static uint64_t tempfs_get_bytes_from_inode(struct tempfs_filesystem* fs, uint8_t* buffer, uint64_t buffer_size,
                                            uint64_t inode_number, uint64_t byte_start, uint64_t size_to_read);
static uint64_t tempfs_get_directory_entries(struct tempfs_filesystem* fs, struct tempfs_directory_entry* children,
                                             uint64_t inode_number, uint64_t children_size);
static struct vnode* tempfs_directory_entry_to_vnode(struct vnode* parent, struct tempfs_directory_entry* entry,
                                                     struct tempfs_filesystem* fs);
static struct tempfs_byte_offset_indices tempfs_indirection_indices_for_block_number(uint64_t block_number);
static void tempfs_write_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode);
static void tempfs_write_block_by_number(uint64_t block_number, uint8_t* buffer, struct tempfs_filesystem* fs,
                                         uint64_t offset, uint64_t write_size);
static void tempfs_read_block_by_number(uint64_t block_number, uint8_t* buffer, struct tempfs_filesystem* fs,
                                        uint64_t offset, uint64_t read_size);
static uint64_t tempfs_allocate_single_indirect_block(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);
static uint64_t tempfs_allocate_double_indirect_block(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);
static uint64_t tempfs_allocate_triple_indirect_block(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate);
static void tempfs_read_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode, uint64_t inode_number);
static uint64_t tempfs_get_relative_block_number_from_file(struct tempfs_inode* inode, uint64_t current_block,
                                                           struct tempfs_filesystem* fs);
static void free_blocks_from_inode(struct tempfs_filesystem* fs,
                                   uint64_t block_start, uint64_t num_blocks,
                                   struct tempfs_inode* inode);
static uint64_t tempfs_read_bytes_from_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode, uint8_t* buffer,
                                             uint64_t buffer_size,
                                             uint64_t offset, uint64_t read_size_bytes);
static uint64_t tempfs_write_bytes_to_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode, uint8_t* buffer,
                                            uint64_t buffer_size,
                                            uint64_t offset,
                                            uint64_t write_size_bytes);
static void tempfs_remove_file(struct tempfs_filesystem* fs, struct tempfs_inode* inode);
static void tempfs_recursive_directory_entry_free(struct tempfs_filesystem* fs,
                                                  struct tempfs_directory_entry* entry, uint64_t inode_number);
static uint64_t tempfs_directory_entry_free(struct tempfs_filesystem* fs, struct tempfs_directory_entry* entry,
                                            struct tempfs_inode* inode);
uint64_t tempfs_inode_allocate_new_blocks(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                          uint32_t num_blocks_to_allocate);
static uint64_t tempfs_write_dirent(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                    struct tempfs_directory_entry* entry);
static uint64_t tempfs_symlink_check(struct tempfs_filesystem* fs, struct vnode* vnode, int32_t expected_type);
static struct vnode* tempfs_follow_link(struct tempfs_filesystem* fs, struct vnode* vnode);
/*
 * VFS pointer functions for vnodes
 */
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

/*
 *  I have opted to go with this general flow.
 *
 *  Read ramdisk structure into temporary variable.
 *
 *  Write to temporary local variable.
 *
 *  Then write said variable to the ramdisk.
 *
 *  We can improve performance by passing pointers directly to ramdisk memory,
 *  but I will try this approach for now.
 *
 *  I think this will allow for more flexibility when it comes to not locking certain
 *  functions as it may help prevent race conditions so you don't read bad memory in a
 *  lockless read.
 *
 *  If it turns out to cause noticeable performance issues I may change this.
 *
 *  Anyway, putting this here as a bulletin of the approach being taken and why.
 */


/*
 * This function will do basic setup such as initing the tempfs lock, it will then setup the ramdisk driver and begin to fill ramdisk memory with a tempfs filesystem.
 * It will use the size DEFAULT_TEMPFS_SIZE and any other size, modifications need to be made to the superblock object.
 */
void tempfs_init(uint64_t filesystem_id) {
    if (filesystem_id >= NUM_FILESYSTEM_OBJECTS) {
        return;
    }
    initlock(tempfs_filesystem[filesystem_id].lock,TEMPFS_LOCK);
    ramdisk_init(DEFAULT_TEMPFS_SIZE, tempfs_filesystem[filesystem_id].ramdisk_id, "initramfs",TEMPFS_BLOCKSIZE);
    tempfs_mkfs(filesystem_id, &tempfs_filesystem[filesystem_id]);
};

/*
 * This will be the function that spins up an empty tempfs filesystem and then fill with a root directory and a few basic directories and files.
 *
 * Takes a ramdisk ID to specify which ramdisk to operate on
 */
void tempfs_mkfs(uint64_t ramdisk_id, struct tempfs_filesystem* fs) {
    uint8_t* buffer = kmalloc(PAGE_SIZE);
    fs->superblock->magic = TEMPFS_MAGIC;
    fs->superblock->version = TEMPFS_VERSION;
    fs->superblock->block_size = TEMPFS_BLOCKSIZE;
    fs->superblock->num_blocks = TEMPFS_NUM_BLOCKS;
    fs->superblock->num_inodes = TEMPFS_NUM_INODES;
    fs->superblock->inode_start_pointer = TEMPFS_START_INODES;
    fs->superblock->block_start_pointer = TEMPFS_START_BLOCKS;
    fs->superblock->inode_bitmap_pointers_start = TEMPFS_START_INODE_BITMAP;
    fs->superblock->block_bitmap_pointers_start = TEMPFS_START_BLOCK_BITMAP;
    fs->superblock->block_bitmap_size = TEMPFS_NUM_BLOCK_POINTER_BLOCKS;
    fs->superblock->inode_bitmap_size = TEMPFS_NUM_INODE_POINTER_BLOCKS;

    fs->superblock->total_size = DEFAULT_TEMPFS_SIZE;
    fs->ramdisk_id = ramdisk_id;

    //copy the contents into our buffer
    memcpy(buffer, &tempfs_superblock, sizeof(struct tempfs_superblock));

    //Write the new superblock to the ramdisk
    ramdisk_write(buffer,TEMPFS_SUPERBLOCK, 0, fs->superblock->block_size,PAGE_SIZE, ramdisk_id);

    memset(buffer, 0,PAGE_SIZE);

    /*
     * Fill in inode bitmap with zeros blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODE_BITMAP, 0,TEMPFS_NUM_INODE_POINTER_BLOCKS * fs->superblock->block_size,
                  fs->superblock->block_size, ramdisk_id);

    /*
     * Fill in block bitmap with zerod blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCK_BITMAP, 0,TEMPFS_NUM_BLOCK_POINTER_BLOCKS * fs->superblock->block_size,
                  fs->superblock->block_size, ramdisk_id);

    /*
     *Write zerod blocks for the inode blocks
     */
    for (int i = 0; i < TEMPFS_NUM_INODES; i++) {
    }
    ramdisk_write(buffer,TEMPFS_START_INODES, 0,TEMPFS_NUM_INODES * fs->superblock->block_size,
                  fs->superblock->block_size, ramdisk_id);
    /*
     *Write zerod blocks for the blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCKS, 0, TEMPFS_NUM_BLOCKS * fs->superblock->block_size,
                  fs->superblock->block_size, ramdisk_id);

    struct tempfs_inode root;
    struct tempfs_inode child;
    tempfs_get_free_inode_and_mark_bitmap(fs, &root);


    root.type = TEMPFS_DIRECTORY;
    strcpy((char*)&root.name, "root");
    root.parent_inode_number = root.inode_number;
    tempfs_write_inode(fs, &root);

    memset(&vfs_root, 0, sizeof(struct vnode));
    strcpy((char*)vfs_root.vnode_name, "root");
    vfs_root.filesystem_object = fs;
    vfs_root.vnode_inode_number = root.inode_number;
    vfs_root.vnode_type = TEMPFS_DIRECTORY;
    vfs_root.vnode_ops = &tempfs_vnode_ops;
    vfs_root.vnode_refcount = 1;
    vfs_root.vnode_parent = NULL;
    vfs_root.vnode_children = NULL;
    vfs_root.vnode_filesystem_id = VNODE_FS_TEMPFS;


    /*               START                   TESTING                              */

    struct vnode* new = tempfs_create(&vfs_root, "file.txt",TEMPFS_REG_FILE);


    serial_printf("Parent Size Blocks name %i %i %i %s\n", root.parent_inode_number, root.size, root.block_count,
                  &root.name);
    tempfs_read_inode(fs, &root, new->vnode_parent->vnode_inode_number);
    tempfs_write_inode(fs, &root);
    tempfs_read_inode(fs, &child, new->vnode_inode_number);

    struct tempfs_directory_entry* entries = kmalloc(sizeof(struct tempfs_directory_entry) * TEMPFS_MAX_FILES_IN_DIRECTORY);
    serial_printf("SIZE %i\n",root.size);
    tempfs_get_directory_entries(fs, entries, 0,TEMPFS_MAX_FILES_IN_DIRECTORY * sizeof(struct tempfs_directory_entry));

    serial_printf("ENTRY NUMBER %i ENTRY NAME %s\n", entries[0].inode_number, entries[0].name);

    tempfs_read_inode(fs, &root, root.inode_number);


    uint8_t* buffer2 = kmalloc(PAGE_SIZE);
    char* test ="This is some random file data we can write some shit here and see what the hell happens. Send er budThis is some random file data we can write some shit here and see what the hell happens. Send er budThis is some random file data we can write some shit here and see what the hell happens. Send er bud";
    strcpy(buffer2, test);
    uint64_t len = strlen(test);

    uint8_t* buffer3 = kmalloc(PAGE_SIZE * 128);

    strcpy(child.name, "file-updated.txt");
    tempfs_write_inode(fs,&child);
    tempfs_directory_entry_free(fs,NULL,&child);
    tempfs_read_inode(fs,&child, 0);
    struct vnode* new2 = tempfs_create(&vfs_root, "home",TEMPFS_DIRECTORY);
    struct tempfs_inode home;
    tempfs_read_inode(fs,&home,new2->vnode_inode_number);
    tempfs_read_inode(fs,&root,root.inode_number);

    serial_printf("ROOT SIZE %i blocks %i child name %s inode %i\n", root.size, root.block_count,home.name,home.inode_number);


    /*               END                   TESTING                              */
    kfree(buffer2);
    kfree(buffer3);
    kfree(buffer);

    serial_printf("Tempfs filesystem initialized of size %i , %i byte blocks\n",DEFAULT_TEMPFS_SIZE / TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE);
}


void tempfs_remove(struct vnode* vnode) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    tempfs_directory_entry_free(fs,NULL, &inode);
    release_spinlock(fs->lock);
}

void tempfs_rename(struct vnode* vnode, char* new_name) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    safe_strcpy(inode.name, new_name,MAX_FILENAME_LENGTH);
    tempfs_write_inode(fs, &inode);
    release_spinlock(fs->lock);
}

uint64_t tempfs_read(struct vnode* vnode, uint64_t offset, uint8_t* buffer, uint64_t bytes) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);

    if (vnode->vnode_type == VNODE_LINK) {
        if (tempfs_symlink_check(fs, vnode,TEMPFS_REG_FILE) != SUCCESS) {
            return UNEXPECTED_SYMLINK_TYPE;
        }
    }
    tempfs_get_bytes_from_inode(fs, buffer, bytes, vnode->vnode_inode_number, offset, bytes);
    release_spinlock(fs->lock);
    return SUCCESS;
}

uint64_t tempfs_write(struct vnode* vnode, uint64_t offset, uint8_t* buffer, uint64_t bytes) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);

    if (vnode->vnode_type == TEMPFS_DIRECTORY) {
        return CANNOT_WRITE_DIRECTORY;
    }

    if (vnode->vnode_type == TEMPFS_SYMLINK) {
        if (tempfs_symlink_check(fs, vnode,TEMPFS_REG_FILE) != SUCCESS) {
            return UNEXPECTED_SYMLINK_TYPE;
        }

        vnode = tempfs_follow_link(fs, vnode);
    }

    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    if (tempfs_write_bytes_to_inode(fs, &inode, buffer, bytes, offset, bytes) != SUCCESS) {
        return TEMPFS_ERROR;
    }

    release_spinlock(fs->lock);
    return SUCCESS;
}

uint64_t tempfs_stat(const struct vnode* vnode, uint64_t offset, uint8_t* buffer, uint64_t bytes) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);

    release_spinlock(fs->lock);
    return SUCCESS;
}

/*
 * A lookup function to be invoked on a vnode via the tempfs pointer function struct.
 *
 * Ensure the filesystem is tempfs.
 *
 * Ensure the type is directory.
 *
 * If vnode is not cached, set fill_vnode so that we will fill it with children
 *
 * If the allocation flag for children pointer is not set, call vnode_directory_alloc_children to allocate memory and set the flag
 *
 * Since I am not allowing indirection with directory entries, we only need to allocate num blocks in inode * block size
 *
 * We invoke tempfs_get_directory_entries() which will fill up the buffer with directory entries
 *
 * Iterate through each entry in the buffer,if fill_vnode flag is set, add each entry to the array of children on the parent
 *
 * Check that we found the directory we are searching for, check if child is NULL first otherwise we will need to span 128 byte chars for every single
 * entry even after we found the entry we were lookling for.
 *
 * Set cached to true
 *
 * Finally, return child
 */
struct vnode* tempfs_lookup(struct vnode* parent, char* name) {
    struct tempfs_filesystem* fs = parent->filesystem_object;

    if (parent->vnode_filesystem_id != VNODE_FS_TEMPFS || parent->vnode_type != VNODE_DIRECTORY) {
        return NULL;
    }

    uint64_t buffer_size = fs->superblock->block_size * NUM_BLOCKS_DIRECT;

    uint8_t* buffer = kmalloc(fs->superblock->block_size * NUM_BLOCKS_DIRECT);

    uint64_t ret = tempfs_get_directory_entries(fs, (struct tempfs_directory_entry*)&buffer, parent->vnode_inode_number,
                                                buffer_size);

    if (ret != SUCCESS) {
        kfree(buffer);
        return NULL;
    }

    uint64_t fill_vnode = 0;

    if (!parent->is_cached) {
        fill_vnode = 1;
    }


    if (fill_vnode && !(parent->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED)) {
        vnode_directory_alloc_children(parent);
    }

    struct vnode* child = NULL;

    uint8_t max_directories = parent->vnode_size / sizeof(struct tempfs_directory_entry) > VNODE_MAX_DIRECTORY_ENTRIES
                                  ? VNODE_MAX_DIRECTORY_ENTRIES
                                  : parent->vnode_size / sizeof(struct tempfs_directory_entry);

    for (uint64_t i = 0; i < max_directories; i++) {
        struct tempfs_directory_entry* entry = (struct tempfs_directory_entry*)&buffer[i];
        if (fill_vnode) {
            parent->vnode_children[i] = tempfs_directory_entry_to_vnode(parent, entry, fs);
        }
        /*
         * Check child so we don't strcmp every time after we find it in the case of filling the parent vnode with its children
         */
        if (child == NULL && safe_strcmp(name, entry->name,VFS_MAX_NAME_LENGTH)) {
            child = tempfs_directory_entry_to_vnode(parent, entry, fs);
            if (!fill_vnode) {
                goto done;
            }
        }

        if (i == max_directories) {
            memset(parent->vnode_children[i + 1], 0, sizeof(struct vnode));
        }
    }
    parent->is_cached = TRUE;

done:
    kfree(buffer);
    return child;
}

/*
 * Create a new tempfs file/directory with name : name, type : vnode type, and parent : parent.
 * return new vnode.
 *
 * Assigning all the necessary vnode attributes and inode attributes
 */
struct vnode* tempfs_create(struct vnode* parent, char* name, uint8_t vnode_type) {
    if (parent->vnode_filesystem_id != VNODE_FS_TEMPFS) {
        return NULL;
    }
    if (parent->vnode_type != VNODE_DIRECTORY) {
        return NULL;
    }

    struct tempfs_filesystem* fs = parent->filesystem_object;

    acquire_spinlock(fs->lock);

    struct vnode* new_vnode = vnode_alloc();
    struct tempfs_inode parent_inode;
    struct tempfs_inode inode;
    tempfs_get_free_inode_and_mark_bitmap(parent->filesystem_object, &inode);

    new_vnode->is_cached = false;
    new_vnode->vnode_type = vnode_type;
    new_vnode->vnode_inode_number = inode.inode_number;
    new_vnode->filesystem_object = parent->filesystem_object;
    new_vnode->vnode_refcount = 1;
    new_vnode->vnode_children = NULL;
    new_vnode->vnode_device_id = parent->vnode_device_id;
    new_vnode->vnode_ops = &tempfs_vnode_ops;
    new_vnode->vnode_parent = parent;
    safe_strcpy(new_vnode->vnode_name, name, MAX_FILENAME_LENGTH);


    inode.type = vnode_type;
    inode.block_count = 0;
    inode.parent_inode_number = parent->vnode_inode_number;
    safe_strcpy((char*)&inode.name, name, MAX_FILENAME_LENGTH);
    inode.uid = 0;
    tempfs_write_inode(parent->filesystem_object, &inode);
    tempfs_read_inode(parent->filesystem_object, &parent_inode, 0);
    struct tempfs_directory_entry entry = {0};
    entry.inode_number = inode.inode_number;
    entry.size = 0;
    safe_strcpy((char*)&entry.name, name, MAX_FILENAME_LENGTH);
    entry.parent_inode_number = inode.parent_inode_number;
    entry.device_number = fs->ramdisk_id;
    entry.type = inode.type;
    uint64_t ret = tempfs_write_dirent(fs, &parent_inode, &entry);
    tempfs_write_inode(fs, &parent_inode);
    release_spinlock(fs->lock);

    if (ret != SUCCESS) {
        return NULL;
    }
    return new_vnode;
}

void tempfs_close(struct vnode* vnode) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);


    release_spinlock(fs->lock);
}

uint64_t tempfs_open(struct vnode* vnode) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);


    release_spinlock(fs->lock);
    return SUCCESS;
}

struct vnode* tempfs_link(struct vnode* vnode, struct vnode* new_vnode) {
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);


    release_spinlock(fs->lock);
}

void tempfs_unlink(struct vnode* vnode) {
    struct tempfs_inode inode;
    struct tempfs_filesystem* fs = vnode->filesystem_object;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    if (inode.refcount <= 1) {
        tempfs_remove(vnode);
    }
    else {
        inode.refcount--;
        tempfs_write_inode(fs, &inode);
    }
}


/*
************
************
************
************
************
************
*/


/*
 *  These are all fairly self-explanatory internal helper functions for doing things such a reading and setting bitmaps,
 *  following block pointers to get data, getting specific bytes from a file and moving to a buffer, getting directory entries,
 *  and so on.
 *
 *  These will be hidden away since the implementation is ugly with a lot of calculation and iteration, they are meant to be used
 *  in conjunction with each other in specific ways so they will not be linked outside of this file.
 */


/*
 * type is passed as the macro pair BITMAP_TYPE_BLOCK or BITMAP_TYPE_INODE
 * number is the block or inode # so that its spot can be calculated
 * action is the macros BITMAP_ACTION_SET or BITMAP_ACTION_CLEAR
 *
 * This will likely just be used for freeing.
 *
 * I think I can get away with no locking here but I will let it stay for now.
 */

/*
 * Simple function for taking a tempfs directory entry and converting it a vnode,
 * and returning said vnode.
 */
static struct vnode* tempfs_directory_entry_to_vnode(struct vnode* parent, struct tempfs_directory_entry* entry,
                                                     struct tempfs_filesystem* fs) {
    struct vnode* vnode = vnode_alloc();
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    vnode->vnode_type = entry->type;
    vnode->vnode_device_id = entry->device_number;
    vnode->vnode_size = entry->size;
    vnode->vnode_inode_number = entry->inode_number;
    vnode->vnode_filesystem_id = VNODE_FS_TEMPFS;\
    vnode->vnode_ops = &tempfs_vnode_ops;
    vnode->vnode_refcount = inode.refcount;
    vnode->is_mount_point = FALSE;
    vnode->mounted_vnode = NULL;
    vnode->is_cached = FALSE;
    *vnode->vnode_name = *entry->name;
    vnode->last_updated = 0;
    vnode->num_children = entry->type == TEMPFS_DIRECTORY
                              ? entry->size
                              : 0;
    vnode->filesystem_object = fs;
    vnode->vnode_parent = parent;

    return vnode;
}


static uint64_t tempfs_symlink_check(struct tempfs_filesystem* fs, struct vnode* vnode, int32_t expected_type) {
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    uint8_t temp_buffer[TEMPFS_BLOCKSIZE] = {0};
    tempfs_read_block_by_number(inode.blocks[0], temp_buffer, fs, 0,TEMPFS_BLOCKSIZE);
    struct vnode* link = vnode_lookup(temp_buffer);

    if (link == NULL) {
        release_spinlock(fs->lock);
        return BAD_SYMLINK;
    }

    if (link->vnode_type != expected_type) {
        release_spinlock(fs->lock);
        return UNEXPECTED_SYMLINK_TYPE;
    }

    return SUCCESS;
}


static struct vnode* tempfs_follow_link(struct tempfs_filesystem* fs, struct vnode* vnode) {
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    uint8_t temp_buffer[TEMPFS_BLOCKSIZE] = {0};
    tempfs_read_block_by_number(inode.blocks[0], temp_buffer, fs, 0,TEMPFS_BLOCKSIZE);
    struct vnode* link = vnode_lookup(temp_buffer);
    return link;
}

/*
 * Can either pass a directory entry inside the target directory or an inode number of the directory
 *
 * This function must be used to remove a dirent , recursive deletion should not be used only called from this function.
 *
 */
static uint64_t tempfs_directory_entry_free(struct tempfs_filesystem* fs, struct tempfs_directory_entry* entry,
                                            struct tempfs_inode* inode) {
    struct tempfs_inode parent_inode;

    if (entry != NULL) {
        tempfs_read_inode(fs, &parent_inode, entry->parent_inode_number);
    }
    else {
        tempfs_read_inode(fs, &parent_inode, inode->parent_inode_number);
    }

    uint64_t inode_to_remove = entry == NULL ? inode->inode_number : entry->inode_number;
    uint8_t* buffer = kmalloc(PAGE_SIZE);
    struct tempfs_directory_entry* entries = (struct tempfs_directory_entry*)buffer;

    /*
     * Iterate through each dirent , loading the next block when the modulus operation resets back to 0
     * If there is more than 1 dirent when the target is found, move the last dirent to the place of the removed one.
     */
    for (uint64_t i = 0; i < parent_inode.size; i++) {
        if (i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK == 0) {
            tempfs_read_block_by_number(parent_inode.blocks[i / TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                        fs->superblock->block_size);
        }

        if (entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number == inode_to_remove) {
            if (entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK].type == TEMPFS_DIRECTORY) {
                tempfs_recursive_directory_entry_free(fs, &entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], 0);
            }

            else if (entries[i].type == TEMPFS_REG_FILE) {
                struct tempfs_inode temp_inode;
                tempfs_read_inode(fs, &temp_inode, entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number);

                /*
                 * If this is the only reference remove it , otherwise just drops to setting the entry memory only and decrementing refcount.
                 */

                if (temp_inode.refcount <= 1) {
                    tempfs_remove_file(fs, &temp_inode);
                }
                else {
                    temp_inode.refcount--;
                    tempfs_write_inode(fs, &temp_inode);
                }
            }
            uint8_t* shift_buffer = kmalloc(PAGE_SIZE);

            if (parent_inode.size > 1) {
                tempfs_read_block_by_number(parent_inode.blocks[parent_inode.size / TEMPFS_MAX_FILES_IN_DIRENT_BLOCK],
                                            shift_buffer, fs, 0,
                                            fs->superblock->block_size);

                struct tempfs_directory_entry* entries2 = (struct tempfs_directory_entry*)shift_buffer;

                memmove(&entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK],
                        &entries2[parent_inode.size % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK],
                        sizeof(struct tempfs_directory_entry));

                memset(&entries2[parent_inode.size % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], 0,
                       sizeof(struct tempfs_directory_entry));

                /*
                 *  If the just shifted dirent was the only entry in a block, free that block
                 */
                if (parent_inode.size % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK == 0) {
                    tempfs_free_block_and_mark_bitmap(
                        fs, parent_inode.blocks[parent_inode.size / TEMPFS_MAX_FILES_IN_DIRENT_BLOCK]);
                    parent_inode.block_count--;
                }
                parent_inode.size -= 1;
                //write the new block with the dirent taken out
                tempfs_write_block_by_number(parent_inode.blocks[i / TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                             fs->superblock->block_size);
            }
            else {
                memset(&entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], 0, sizeof(struct tempfs_directory_entry));
            }
            //write the new block from which we just removed the dirent from
            tempfs_write_block_by_number(parent_inode.blocks[i / TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                         fs->superblock->block_size);

            kfree(buffer);

            kfree(shift_buffer);

            return SUCCESS;
        }
    }

    kfree(buffer);
    return NOT_FOUND;
}

/*
 * This could pose an issue with stack space but we'll run with it. If it works it works, if it doesn't we'll either change it or allocate more stack space.
 */
static void tempfs_recursive_directory_entry_free(struct tempfs_filesystem* fs,
                                                  struct tempfs_directory_entry* entry, uint64_t inode_number) {
    struct tempfs_inode inode;

    if (entry != NULL) {
        tempfs_read_inode(fs, &inode, entry->inode_number);
    }
    else {
        tempfs_read_inode(fs, &inode, inode_number);
    }

    uint8_t* buffer = kmalloc(PAGE_SIZE);
    struct tempfs_directory_entry* entries = (struct tempfs_directory_entry*)buffer;

    for (uint64_t i = 0; i < inode.size; i++) {
        if (i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK == 0) {
            tempfs_read_block_by_number(inode.blocks[i / TEMPFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                        fs->superblock->block_size);
        }

        if (entries[i].type == TEMPFS_REG_FILE) {
            struct tempfs_inode temp_inode;
            tempfs_read_inode(fs, &temp_inode, entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number);
            /*
                        * If this is the only reference remove it , otherwise just drops to setting the entry memory only and decrementing refcount.
                        */
            if (temp_inode.refcount <= 1) {
                tempfs_remove_file(fs, &temp_inode);
            }
            else {
                temp_inode.refcount--;
                tempfs_write_inode(fs, &temp_inode);
            }
        }
        else if (entries[i].type == TEMPFS_DIRECTORY) {
            tempfs_recursive_directory_entry_free(fs, &entries[i], 0);
        }
        else {
            struct tempfs_inode temp_inode;
            tempfs_read_inode(fs, &temp_inode, entries[i % TEMPFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number);
            tempfs_remove_file(fs, &temp_inode);
        }
    }
    tempfs_write_inode(fs, &inode);
    kfree(buffer);
}

/*
 *Remove regular file, free all blocks, clear the inode out, free the inode
 *
 *TESTED
 */
static void tempfs_remove_file(struct tempfs_filesystem* fs, struct tempfs_inode* inode) {
    free_blocks_from_inode(fs, 0, inode->block_count, inode);
    memset(inode->name,0,MAX_FILENAME_LENGTH);
    tempfs_write_inode(fs, inode);
    tempfs_free_inode_and_mark_bitmap(fs, inode->inode_number);
}

/*
 * Clear a block or inode bitmap entry
 *
 * TESTED
 */
static void tempfs_clear_bitmap(struct tempfs_filesystem* fs, uint8_t type, uint64_t number) {
    if (type != BITMAP_TYPE_BLOCK && type != BITMAP_TYPE_INODE) {
        panic("Unknown type tempfs_clear_bitmap");
    }

    uint64_t block_to_read;
    uint64_t block;

    if (type == BITMAP_TYPE_BLOCK) {
        block_to_read = (number / (fs->superblock->block_size * 8));
        block = fs->superblock->block_bitmap_pointers_start + block_to_read;
    }
    else {
        block_to_read = number / (fs->superblock->block_size * 8);
        block = fs->superblock->inode_bitmap_pointers_start + block_to_read;
    }

    uint64_t byte_in_block = number / 8;

    uint64_t bit = number % 8;

    uint8_t* buffer = kmalloc(PAGE_SIZE);
    memset(buffer, 0, PAGE_SIZE);

    ramdisk_read(buffer,block,0,fs->superblock->block_size,PAGE_SIZE,fs->ramdisk_id);
    /*
     * 0 the bit and write it back Noting that this doesnt work for a set but Im not sure that
     * I will use it for that.
     */
    buffer[byte_in_block] &= ~(1 << bit);
    ramdisk_write(buffer,block,0,fs->superblock->block_size,PAGE_SIZE,fs->ramdisk_id);
    kfree(buffer);
}


/*
 * This function searches the bitmap for a free inode. It will keep track of the block , byte ,and bit so that once it is marked
 * we can return the block that is now marked.
 *
 * Because at the time of writing this will always be 1.5 pages (6 kib) of total bitmap to scan, we will allocate a 2 page buffer and read
 * the entire bitmap in.
 *
 *block, byte, bit ,buffer, buffer_size are straight forward.
 *
 *If all goes well, write the new bitmap block, and return the block number so that the caller can
 *add it to the target inode and begin to use it.
 *
 *Traversal is simple, read bytes to check for a non 0xFF value which means there is a free bit in this byte, then iterate the bits to find
 *the first free bit.
 *
 */
static void tempfs_get_free_inode_and_mark_bitmap(struct tempfs_filesystem* fs,
                                                  struct tempfs_inode* inode_to_be_filled) {
    uint64_t buffer_size = PAGE_SIZE * 8;
    uint8_t* buffer = kmalloc(buffer_size);
    uint64_t block = 0;
    uint64_t byte = 0;
    uint64_t bit = 0;
    uint64_t inode_number;


    uint64_t ret = ramdisk_read(buffer, fs->superblock->inode_bitmap_pointers_start, 0,
                                fs->superblock->block_size * TEMPFS_NUM_INODE_POINTER_BLOCKS, buffer_size,
                                fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap");
        panic("tempfs_get_free_inode_and_mark_bitmap ramdisk read failed");
    }

    while (1) {
        if (buffer[block * fs->superblock->block_size + byte] != 0xFF) {
            for (uint64_t i = 0; i <= 8; i++) {
                if (!(buffer[(block * fs->superblock->block_size) + byte] & (1 << i))) {
                    bit = i;
                    buffer[byte] |= (1 << bit);
                    inode_number = (block * fs->superblock->block_size * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte == fs->superblock->block_size) {
                block++;
                byte = 0;
            }
            if (block > TEMPFS_NUM_INODE_POINTER_BLOCKS) {
                panic("tempfs_get_free_inode_and_mark_bitmap: No free inodes");
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        }
        else {
            block++;
        }
    }


found_free:
    memset(inode_to_be_filled, 0, sizeof(struct tempfs_inode));
    inode_to_be_filled->inode_number = inode_number;

    ret = ramdisk_write(buffer, fs->superblock->inode_bitmap_pointers_start, 0,
                        fs->superblock->block_size * TEMPFS_NUM_INODE_POINTER_BLOCKS, buffer_size,
                        fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap");
        panic("tempfs_get_free_inode_and_mark_bitmap ramdisk read failed");
    }
    kfree(buffer);
}

/*
 * This function searches the bitmap for a free block. It will keep track of the block , byte ,and bit so that once it is marked
 * we can return the block that is now marked.
 *
 * Because at the time of writing this will always be 28.5 pages of total bitmap to scan, we will start with a 16 page read, then an 8 page read, then a 4 page read, then finally for
 * the last half page a 2 page read. This will make is easiest to search quickly.
 *
 *block, byte, bit ,buffer, buffer_size are straight forward.
 *
 *offset is used to subtract from the block number when a buffer runs dry with no hits found.
 * The reasoning is so that block doesn't need to be reset so we get absolute block number not
 * buffer-relative block number.
 *
 *If all goes well, write the new bitmap block, and return the block number so that the caller can
 *add it to the target inode and begin to use it.
 *
 *Traversal is simple, read bytes to check for a non 0xFF value which means there is a free bit in this byte, then iterate the bits to find
 *the first free bit.
 *
 */

static uint64_t tempfs_get_free_block_and_mark_bitmap(struct tempfs_filesystem* fs) {
    uint64_t buffer_size = PAGE_SIZE * 128;
    uint8_t* buffer = kmalloc(buffer_size);
    uint64_t block = 0;
    uint64_t byte = 0;
    uint64_t bit = 0;
    uint64_t offset = 0;
    uint64_t block_number = 0;

retry:

    /*
     *We do not free the buffer we simply write into a smaller and smaller portion of the buffer.
     *It is only freed after a block is found and the new bitmap block is written.
     *
     *We do not use the tempfs_read_block function because it only works with single blocks and we are reading
     *many blocks here so we will work with ramdisk functions directly.
     *
     */

    uint64_t ret = ramdisk_read(buffer, fs->superblock->block_bitmap_pointers_start, 0,
                                fs->superblock->block_size * TEMPFS_NUM_BLOCK_POINTER_BLOCKS, buffer_size,
                                fs->ramdisk_id);


    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap");
        panic("tempfs_get_free_inode_and_mark_bitmap ramdisk read failed");
        /* For diagnostic purposes , shouldn't happen if it does I want to know right away */
    }

    while (1) {
        if (buffer[block * fs->superblock->block_size + byte] != 0xFF) {
            for (uint64_t i = 0; i <= 8; i++) {
                if (!(buffer[(block * fs->superblock->block_size) + byte] & (1 << i))) {
                    bit = i;
                    buffer[(block * fs->superblock->block_size) + byte] |= (1 << bit);
                    block_number = (block * fs->superblock->block_size * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte == fs->superblock->block_size) {
                block++;
                byte = 0;
            }
            if (block > TEMPFS_NUM_BLOCK_POINTER_BLOCKS) {
                panic("tempfs_get_free_inode_and_mark_bitmap: No free inodes");
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        }
        else {
            byte++;
        }
    }


found_free:
    ret = ramdisk_write(buffer, fs->superblock->block_bitmap_pointers_start , 0,
                        fs->superblock->block_size * TEMPFS_NUM_BLOCK_POINTER_BLOCKS, buffer_size,
                        fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap ramdisk_write call")
        panic("tempfs_get_free_inode_and_mark_bitmap"); /* Extreme but that is okay for diagnosing issues */
    }

    kfree(buffer);
    /* Free the buffer, all other control paths lead to panic so until that changes this is the only place it is required */

    /*
     * It will be very important that this return value not be wasted because it will leave a block marked and not used.
     */
    return block_number;
}

/*
 *  These two functions are fairly self-explanatory.
 *
 *  WARNING , MUST MAKE SURE A FUNCTION WILL TAKE CARE OF MODIFYING INODE-LOCAL BLOCK POINTERS
 */
/*
 * I don't think I need locks on frees, I will find out one way or another if this is true
 */
static uint64_t tempfs_free_block_and_mark_bitmap(struct tempfs_filesystem* fs, uint64_t block_number) {
    uint8_t* buffer = kmalloc(PAGE_SIZE);
    memset(buffer, 0, fs->superblock->block_size);

    tempfs_write_block_by_number(block_number, buffer, fs, 0, fs->superblock->block_size);

    tempfs_clear_bitmap(fs,BITMAP_TYPE_BLOCK, block_number);

    kfree(buffer);
    return SUCCESS;
}

/*
 * I don't think I need locks on frees, I will find out one way or another if this is true
 */
static uint64_t tempfs_free_inode_and_mark_bitmap(struct tempfs_filesystem* fs, uint64_t inode_number) {
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, inode_number);
    memset(&inode, 0, sizeof(struct tempfs_inode));
    inode.inode_number = inode_number;
    tempfs_write_inode(fs, &inode); /* zero the inode leaving just the inode number */
    uint64_t inode_number_block = inode_number / TEMPFS_INODES_PER_BLOCK;
    uint64_t offset = inode_number % TEMPFS_INODES_PER_BLOCK;

    uint8_t* buffer = kmalloc(PAGE_SIZE);

    tempfs_write_block_by_number(fs->superblock->inode_start_pointer + inode_number_block, buffer, fs, offset,
                                 TEMPFS_INODE_SIZE);

    tempfs_clear_bitmap(fs,BITMAP_TYPE_INODE, inode_number);

    kfree(buffer);
    return SUCCESS;
}

/*
 * Unimplemented until tempfs_read_bytes_from_inode is complete
 */
static uint64_t tempfs_get_bytes_from_inode(struct tempfs_filesystem* fs, uint8_t* buffer, uint64_t buffer_size,
                                            uint64_t inode_number, uint64_t byte_start, uint64_t size_to_read) {
    struct tempfs_inode inode;
    tempfs_read_inode(fs, &inode, inode_number);
    return tempfs_read_bytes_from_inode(fs, &inode, buffer, buffer_size, byte_start % fs->superblock->block_size,
                                        size_to_read);
}

/*
 * This function will fill the children array with directory entries found in the inode number passed to it. May make more sense to pass the inode directly or just a pointer to it but this is ok for now.
 *
 *
 * Checks children_size to not write out of bounds.
 *
 * Get the inode via tempfs_get_inode
 *
 * Ensures this inode is actually a directory
 *
 * If all is kosher, iterate on directories_read, read a block of directory entries
 * into the front of the buffer and then assignment operator it into the children directory,
 * saving some buffer space.
 *
 * I will skip out on locking for now. I think it is not needed because writes are done in one fell swoop so either it will be there or
 * it won't, there won't be any intermediate state. You can just read again if you suspect something has been added.
 *
 */
static uint64_t tempfs_get_directory_entries(struct tempfs_filesystem* fs,
                                             struct tempfs_directory_entry* children, uint64_t inode_number,
                                             uint64_t children_size) {
    uint64_t buffer_size = PAGE_SIZE;
    uint8_t* buffer = kmalloc(buffer_size);
    struct tempfs_directory_entry* directory_entries = (struct tempfs_directory_entry*)buffer;
    struct tempfs_inode inode;
    uint64_t directory_entries_read = 0;
    uint64_t directory_block = 0;
    uint64_t block_number = 0;

    tempfs_read_inode(fs, &inode, inode_number);

    if (inode.type != TEMPFS_DIRECTORY) {
        kfree(buffer);
        return TEMPFS_NOT_A_DIRECTORY;
    }

    if (children_size < inode.block_count * fs->superblock->block_size) {
        return TEMPFS_BUFFER_TOO_SMALL;
    }


 while (1) {
     block_number = inode.blocks[directory_block++];
     /*Should be okay to leave this unrestrained since we check children size and inode size */

     tempfs_read_block_by_number(block_number, buffer, fs, 0, fs->superblock->block_size);

     for (uint64_t i = 0; i < (fs->superblock->block_size / sizeof(struct tempfs_directory_entry)); i++) {
         if (directory_entries_read == inode.size) {
             goto done;
         }
         if (directory_entries_read == inode.size) {
             goto done;
         }
         children[directory_entries_read++] = directory_entries[i];
     }
 }


    done:
    kfree(buffer);
    return SUCCESS;
}

/*
 * Fills a buffer with file data , following block pointers and appending bytes to the passed buffer.
 *
 * Took locks out since this will be called from a function that is locked.
 */
static uint64_t tempfs_read_bytes_from_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode, uint8_t* buffer,
                                             uint64_t buffer_size,
                                             uint64_t offset, uint64_t read_size_bytes) {
    uint64_t num_blocks_to_read = read_size_bytes / fs->superblock->block_size;

    if (inode->type != TEMPFS_REG_FILE) {
        panic("tempfs_write_bytes_to_inode bad type");
    }


    if (buffer_size < fs->superblock->block_size || buffer_size < fs->superblock->block_size * num_blocks_to_read) {
        /*
         * I'll set a sensible minimum buffer size
         */
        return TEMPFS_BUFFER_TOO_SMALL;
    }

    uint64_t start_block = offset / fs->superblock->block_size;

    if (offset > inode->size) {
        panic("tempfs_read_bytes_from_inode bad offset"); /* Should never happen, panic for visibility  */
    }

    uint64_t start_offset = offset % fs->superblock->block_size;

    uint64_t current_block_number = 0;
    uint64_t end_block = start_block + num_blocks_to_read;


    /*
     *  If 0 is passed, try to read everything
     */

    uint64_t bytes_read = 0;
    uint64_t bytes_to_read = read_size_bytes;

    for (uint64_t i = start_block; i <= end_block; i++) {
        uint64_t byte_size;
        if (fs->superblock->block_size - start_offset < bytes_to_read) {
            byte_size = fs->superblock->block_size - start_offset;
        }
        else {
            byte_size = bytes_to_read;
        }
        current_block_number = tempfs_get_relative_block_number_from_file(inode, i, fs);

        tempfs_read_block_by_number(current_block_number, buffer + bytes_read, fs, start_offset, byte_size);

        bytes_read += byte_size;
        bytes_to_read -= byte_size;

        if (start_offset) {
            /*
             * Offset is only for first and last block so set to 0 after the first block
             */
            start_offset = 0;
        }
    }

    return SUCCESS;
}

static uint64_t tempfs_write_dirent(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                    struct tempfs_directory_entry* entry) {
    if (inode->size == TEMPFS_MAX_FILES_IN_DIRECTORY) {
        return TEMPFS_CANT_ALLOCATE_BLOCKS_FOR_DIR;
    }

    uint64_t block = inode->size / TEMPFS_MAX_FILES_IN_DIRECTORY;

    uint64_t entry_in_block = (inode->size % TEMPFS_MAX_FILES_IN_DIRECTORY);

    //allocate a new block when needed
    if (entry_in_block == 0) {
        inode->blocks[block] = tempfs_get_free_block_and_mark_bitmap(fs);
        inode->block_count++;
    }

    uint8_t* read_buffer = kmalloc(PAGE_SIZE);
    struct tempfs_directory_entry* tempfs_directory_entries = (struct tempfs_directory_entry*)read_buffer;

    tempfs_read_block_by_number(inode->blocks[block], read_buffer, fs, 0, fs->superblock->block_size);

    memcpy(&tempfs_directory_entries[entry_in_block], entry, sizeof(struct tempfs_directory_entry));
    inode->size++;

    tempfs_write_block_by_number(inode->blocks[block], read_buffer, fs, 0, fs->superblock->block_size);
    tempfs_write_inode(fs, inode);

    kfree(read_buffer);
    return SUCCESS;
}


static uint64_t tempfs_write_bytes_to_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode, uint8_t* buffer,
                                            uint64_t buffer_size,
                                            uint64_t offset,
                                            uint64_t write_size_bytes) {
    if (inode->type != TEMPFS_REG_FILE) {
        panic("tempfs_write_bytes_to_inode bad type");
    }


    uint64_t num_blocks_to_write = write_size_bytes / fs->superblock->block_size;
    uint64_t start_block = offset / fs->superblock->block_size;
    uint64_t start_offset = offset % fs->superblock->block_size;
    uint64_t new_size = false;
    uint64_t new_size_bytes = 0;

    if ((start_offset + write_size_bytes) / fs->superblock->block_size && num_blocks_to_write == 0) {
        num_blocks_to_write++;
    }
    if (buffer_size < write_size_bytes) {
        return TEMPFS_BUFFER_TOO_SMALL;
    }

    if (offset > inode->size) {
        serial_printf("offset %i write_size_bytes %i inode size %i\n", offset, write_size_bytes, inode->size);
        panic("tempfs_write_bytes_from_inode bad offset"); /* Should never happen, panic for visibility  */
    }

    if (offset + write_size_bytes > inode->size) {
        new_size = true;
        new_size_bytes = (offset + write_size_bytes) - inode->size;
    }


    if (inode->size == 0) {
        inode->block_count += 1;
        inode->blocks[0] = tempfs_get_free_block_and_mark_bitmap(fs);
        tempfs_write_inode(fs, inode);
        tempfs_read_inode(fs, inode, inode->inode_number);
    }


    uint64_t current_block_number = 0;
    uint64_t end_block = start_block + num_blocks_to_write;

    if (end_block + 1 > inode->block_count) {
        tempfs_inode_allocate_new_blocks(fs, inode, (end_block + 1) - inode->block_count);
        inode->block_count += ((end_block + 1) - inode->block_count);
    }

    /*
     *  If 0 is passed, try to write everything
     */

    uint64_t bytes_written = 0;
    uint64_t bytes_left = write_size_bytes;

    for (uint64_t i = start_block; i <= end_block; i++) {
        uint64_t byte_size;

        if (fs->superblock->block_size - start_offset < bytes_left) {
            byte_size = fs->superblock->block_size - start_offset;
        }
        else {
            byte_size = bytes_left;
        }

        current_block_number = tempfs_get_relative_block_number_from_file(inode, i, fs);

        tempfs_write_block_by_number(current_block_number, buffer, fs, start_offset, byte_size);


        bytes_written += byte_size;
        bytes_left -= byte_size;
        buffer += bytes_written;

        if (start_offset) {
            /*
        * Offset is only for first and last block so set to 0 after the first block
        */
            start_offset = 0;
        }
    }

    if (new_size == true) {
        inode->size += new_size_bytes;
    }

    tempfs_write_inode(fs, inode);
    return SUCCESS;
}

/*
 * We will take the logical block number in the file and get the next to return it
 *
 * This could be done more efficiently such as transferring directly into a buffer but as knuth said,
 *
 * Premature optimization is the root of all evil.
 *
 * If this approach causes noticeable slowdowns we will revisit.
 *
 * The switch statement just separates what level the requested block is, and then the indices to get the block from there.
 */
static uint64_t tempfs_get_relative_block_number_from_file(struct tempfs_inode* inode, uint64_t current_block,
                                                           struct tempfs_filesystem* fs) {
    uint8_t* temp_buffer = kmalloc(PAGE_SIZE);
    const struct tempfs_byte_offset_indices indices = tempfs_indirection_indices_for_block_number(current_block);
    uint64_t current_block_number = 0;
    uint64_t* indirection_block = (uint64_t*)temp_buffer;

    switch (indices.levels_indirection) {
    case 0:
        current_block_number = inode->blocks[indices.direct_block_number];
        goto done;

    case 1:

        tempfs_read_block_by_number(inode->single_indirect, temp_buffer, fs, 0, fs->superblock->block_size);
        current_block_number = indirection_block[indices.first_level_block_number];
        goto done;

    case 2:

        tempfs_read_block_by_number(inode->double_indirect, temp_buffer, fs, 0, fs->superblock->block_size);
        current_block_number = indirection_block[indices.second_level_block_number];
        tempfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, fs->superblock->block_size);
        current_block_number = indirection_block[indices.first_level_block_number];
        goto done;

    case 3:

        tempfs_read_block_by_number(inode->triple_indirect, temp_buffer, fs, 0, fs->superblock->block_size);
        current_block_number = indirection_block[indices.third_level_block_number];
        tempfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, fs->superblock->block_size);
        current_block_number = indirection_block[indices.second_level_block_number];
        tempfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, fs->superblock->block_size);
        current_block_number = indirection_block[indices.first_level_block_number];
        goto done;

    default:
        panic("tempfs_get_next_logical_block_from_file: unknown indirection");
        return 0; /* Just to shut up the linter, this is unreachable */
    }

done:

    kfree(temp_buffer);
    return current_block_number;
}

/*
 * This function will allocate new blocks to an inode. If needed it will also slide pointers down if there is a write to the beginning or
 * to the end of the file
 */
uint64_t tempfs_inode_allocate_new_blocks(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                          uint32_t num_blocks_to_allocate) {
    struct tempfs_byte_offset_indices result = {0};
    uint8_t* buffer = kmalloc(PAGE_SIZE);

    // Do not allocate blocks for a directory since they hold enough entries (90 or so at the time of writing)
    if (inode->type == TEMPFS_DIRECTORY && (num_blocks_to_allocate + (inode->size / fs->superblock->block_size)) >
        NUM_BLOCKS_DIRECT) {
        serial_printf("tempfs_inode_allocate_new_block inode type not directory!\n");
        kfree(buffer);
        return TEMPFS_ERROR;
    }

    if (num_blocks_to_allocate + (inode->size / fs->superblock->block_size) > MAX_BLOCKS_IN_INODE) {
        serial_printf("tempfs_inode_allocate_new_block too many blocks to request!\n");
        kfree(buffer);
        return TEMPFS_ERROR;
    }

    if (inode->size % fs->superblock->block_size == 0) {
        result = tempfs_indirection_indices_for_block_number(inode->size / fs->superblock->block_size);
    }
    else {
        result = tempfs_indirection_indices_for_block_number((inode->size / fs->superblock->block_size) + 1);
    }

    bool higher_order;
    uint64_t block_number = 0;

    switch (result.levels_indirection) {
    case 0:
        goto level_zero;
    case 1:
        goto level_one;
    case 2:
        goto level_two;
    case 3:
        goto level_three;
    default:
        panic("tempfs_inode_allocate_new_block: unknown indirection");
    }


level_zero:
    for (uint64_t i = inode->block_count; i < NUM_BLOCKS_DIRECT; i++) {
        inode->blocks[i] = tempfs_get_free_block_and_mark_bitmap(fs);
        inode->block_count++;
        num_blocks_to_allocate--;
        if (num_blocks_to_allocate == 0) {
            goto done;
        }
    }
level_one:
    uint64_t num_allocated = inode->block_count - NUM_BLOCKS_DIRECT;
    uint64_t num_in_indirect = num_blocks_to_allocate + num_allocated > NUM_BLOCKS_IN_INDIRECTION_BLOCK
                                   ? NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated
                                   : num_blocks_to_allocate;
    tempfs_allocate_single_indirect_block(fs, inode, num_allocated, num_in_indirect,false, 0);
    inode->block_count += num_in_indirect;
    num_blocks_to_allocate -= num_in_indirect;
    if (num_blocks_to_allocate == 0) {
        goto done;
    }

level_two:
    num_allocated = inode->block_count - NUM_BLOCKS_DIRECT - NUM_BLOCKS_IN_INDIRECTION_BLOCK;
    num_in_indirect = num_blocks_to_allocate + num_allocated > NUM_BLOCKS_DOUBLE_INDIRECTION
                          ? NUM_BLOCKS_DOUBLE_INDIRECTION - num_allocated
                          : num_blocks_to_allocate;
    tempfs_allocate_double_indirect_block(fs, inode, num_allocated, num_in_indirect,false, 0);
    inode->block_count += num_in_indirect;
    num_blocks_to_allocate -= num_in_indirect;
    if (num_blocks_to_allocate == 0) {
        goto done;
    }

level_three:
    num_allocated = inode->block_count - NUM_BLOCKS_DIRECT - NUM_BLOCKS_IN_INDIRECTION_BLOCK -
        NUM_BLOCKS_DOUBLE_INDIRECTION;
    num_in_indirect = num_blocks_to_allocate + num_allocated > NUM_BLOCKS_TRIPLE_INDIRECTION
                          ? NUM_BLOCKS_TRIPLE_INDIRECTION - num_allocated
                          : num_blocks_to_allocate;
    tempfs_allocate_triple_indirect_block(fs, inode, num_allocated, num_in_indirect);
    inode->block_count += num_in_indirect;
    num_blocks_to_allocate -= num_in_indirect;

done:
    if (num_blocks_to_allocate != 0) {
        panic("tempfs_inode_allocate_new_block: num_blocks_to_allocate != 0!\n");
    }
    kfree(buffer);
    tempfs_write_inode(fs, inode);
    return SUCCESS;
}

/*
 * A much simpler index derivation function for getting a tempfs index based off a block number given. Could be for reading a block at an arbitrary offset or finding the end
 * of the file to allocate or write.BL
 *
 * It is fairly simple.
 *
 * We avoid off-by-ones by using the greater than as opposed to greater than or equal operator.
 *
 * Panic if the block number is too high
 */

static struct tempfs_byte_offset_indices tempfs_indirection_indices_for_block_number(uint64_t block_number) {
    struct tempfs_byte_offset_indices byte_offset_indices = {0};

    if (block_number < NUM_BLOCKS_DIRECT) {
        byte_offset_indices.direct_block_number = block_number;
        byte_offset_indices.levels_indirection = 0;
        return byte_offset_indices;
    }

    if (block_number < (NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT)) {
        block_number -= (NUM_BLOCKS_DIRECT);
        byte_offset_indices.levels_indirection = 1;
        byte_offset_indices.direct_block_number = 0;
        byte_offset_indices.first_level_block_number = block_number % NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        return byte_offset_indices;
    }

    if (block_number < (NUM_BLOCKS_DOUBLE_INDIRECTION + NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT)) {
        block_number -= (NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT);
        byte_offset_indices.levels_indirection = 2;
        byte_offset_indices.direct_block_number = 0;
        byte_offset_indices.second_level_block_number = (block_number / NUM_BLOCKS_IN_INDIRECTION_BLOCK);
        byte_offset_indices.first_level_block_number = block_number % NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        return byte_offset_indices;
    }

    if (block_number < (NUM_BLOCKS_TRIPLE_INDIRECTION + NUM_BLOCKS_DOUBLE_INDIRECTION + NUM_BLOCKS_IN_INDIRECTION_BLOCK
        + NUM_BLOCKS_DIRECT)) {
        block_number -= (NUM_BLOCKS_DOUBLE_INDIRECTION + NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT);
        byte_offset_indices.levels_indirection = 3;
        byte_offset_indices.direct_block_number = 0;
        byte_offset_indices.third_level_block_number = block_number / NUM_BLOCKS_DOUBLE_INDIRECTION;
        byte_offset_indices.second_level_block_number = (block_number - ((byte_offset_indices.third_level_block_number *
            NUM_BLOCKS_DOUBLE_INDIRECTION))) / NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        byte_offset_indices.first_level_block_number = block_number % (NUM_BLOCKS_IN_INDIRECTION_BLOCK);
        return byte_offset_indices;
    }


    panic("tempfs_indirection_indices_for_block_number invalid block number");
}

/*
 * 4 functions beneath just take some of the ramdisk calls out in favor of local functions for read and writing blocks and inodes
 */
static void tempfs_write_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode) {
    uint64_t inode_number_in_block = inode->inode_number % NUM_INODES_PER_BLOCK;
    uint64_t block_number = fs->superblock->inode_start_pointer + inode->inode_number / NUM_INODES_PER_BLOCK;
    uint64_t ret = ramdisk_write((uint8_t*)inode, block_number, sizeof(struct tempfs_inode) * inode_number_in_block,
                                 sizeof(struct tempfs_inode), sizeof(struct tempfs_inode), fs->ramdisk_id);
    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_inode");
        panic("tempfs_write_inode"); /* For diagnostic purposes */
    }
}

static void tempfs_read_inode(struct tempfs_filesystem* fs, struct tempfs_inode* inode, uint64_t inode_number) {
    uint64_t inode_number_in_block = inode_number % NUM_INODES_PER_BLOCK;
    uint64_t block_number = fs->superblock->inode_start_pointer + (inode_number / NUM_INODES_PER_BLOCK);
    uint64_t ret = ramdisk_read((uint8_t*)inode, block_number, sizeof(struct tempfs_inode) * inode_number_in_block,
                                sizeof(struct tempfs_inode), sizeof(struct tempfs_inode), fs->ramdisk_id);
    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_inode");
        panic("tempfs_write_inode"); /* For diagnostic purposes */
    }
}

static void tempfs_write_block_by_number(uint64_t block_number, uint8_t* buffer, struct tempfs_filesystem* fs,
                                         uint64_t offset, uint64_t write_size) {
    if (write_size > fs->superblock->block_size) {
        write_size = fs->superblock->block_size;
    }
    if (offset >= fs->superblock->block_size) {
        offset = 0;
    }
    if (write_size == 0) {
        serial_printf("WRITE_SIZE_ZERO\n");
    }

    uint64_t write_size_bytes = write_size;
    if (write_size_bytes + offset > fs->superblock->block_size) {
        write_size_bytes = fs->superblock->block_size - offset;
    }

    uint64_t ret = ramdisk_write(buffer, block_number + TEMPFS_START_BLOCKS, offset, write_size_bytes,
                                 fs->superblock->block_size, fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_block_by_number");
        panic("tempfs_write_block_by_number");
    }
}

static void tempfs_read_block_by_number(uint64_t block_number, uint8_t* buffer, struct tempfs_filesystem* fs,
                                        uint64_t offset, uint64_t read_size) {
    if (read_size > fs->superblock->block_size) {
        read_size = fs->superblock->block_size - offset;
    }

    if (offset >= fs->superblock->block_size) {
        offset = 0;
    }

    uint64_t read_size_bytes = read_size;
    if (read_size_bytes + offset > fs->superblock->block_size) {
        read_size_bytes = fs->superblock->block_size - offset;
    }

    uint64_t ret = ramdisk_read(buffer, block_number + TEMPFS_START_BLOCKS, offset, read_size_bytes, fs->superblock->block_size,
                                fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_read_block_by_number");
        panic("tempfs_read_block_by_number");
    }
}

/*
 * This function will start at block offset and go to num_blocks in order to free as many blocks as requested
 */
static void free_blocks_from_inode(struct tempfs_filesystem* fs,
                                   uint64_t block_start, uint64_t num_blocks,
                                   struct tempfs_inode* inode) {

    if (inode->block_count == 0) {
        return;
    }
    uint64_t block_number = block_start;
    uint64_t indirection_block;
    uint64_t double_indirection_block;
    uint64_t triple_indirection_block;
    uint8_t* buffer = kmalloc(PAGE_SIZE);
    uint8_t* buffer2 = kmalloc(PAGE_SIZE);
    uint8_t* buffer3 = kmalloc(PAGE_SIZE);
    uint64_t* blocks = (uint64_t*)buffer;
    uint64_t* blocks_2 = (uint64_t*)buffer2;
    uint64_t* blocks_3 = (uint64_t*)buffer3;

    struct tempfs_byte_offset_indices byte_offset_indices = tempfs_indirection_indices_for_block_number(block_number);

    switch (byte_offset_indices.levels_indirection) {
    case 0:
        goto level_zero;
    case 1:
        indirection_block = byte_offset_indices.first_level_block_number;
        goto level_one;
    case 2:
        double_indirection_block = byte_offset_indices.second_level_block_number;
        goto level_two;
    case 3:
        triple_indirection_block = byte_offset_indices.third_level_block_number;
        goto level_three;
    default:
        panic("tempfs_indirection_indices_for_block_number invalid block number");
    }


level_zero:
    for (uint64_t i = byte_offset_indices.direct_block_number; i < NUM_BLOCKS_DIRECT; i++) {
        tempfs_free_block_and_mark_bitmap(fs, inode->blocks[i]);
        num_blocks--;
        if (num_blocks == 0) {
            goto done;
        }
    }

    indirection_block = 0;

level_one:
    tempfs_read_block_by_number(inode->single_indirect, buffer, fs, 0, fs->superblock->block_size);

    for (uint64_t i = indirection_block; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i++) {
        tempfs_free_block_and_mark_bitmap(fs, blocks[i]);
        num_blocks--;
        if (num_blocks == 0) {
            goto done;
        }
    }

    double_indirection_block = 0;
level_two:
    tempfs_read_block_by_number(inode->double_indirect, buffer, fs, 0, fs->superblock->block_size);
    for (uint64_t i = double_indirection_block / NUM_BLOCKS_IN_INDIRECTION_BLOCK; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i
         ++) {
        tempfs_read_block_by_number(blocks[i], buffer2, fs, 0, fs->superblock->block_size);

        for (uint64_t j = double_indirection_block % NUM_BLOCKS_IN_INDIRECTION_BLOCK; j <
             NUM_BLOCKS_IN_INDIRECTION_BLOCK; j++) {
            tempfs_free_block_and_mark_bitmap(fs, blocks_2[j]);
            num_blocks--;
            if (num_blocks == 0) {
                goto done;
            }
        }
    }
    triple_indirection_block = 0;
level_three:
    tempfs_read_block_by_number(inode->triple_indirect, buffer, fs, 0, fs->superblock->block_size);
    for (uint64_t i = triple_indirection_block / NUM_BLOCKS_DOUBLE_INDIRECTION; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i
         ++) {
        tempfs_read_block_by_number(blocks[i], buffer2, fs, 0, fs->superblock->block_size);

        for (uint64_t j = triple_indirection_block / NUM_BLOCKS_IN_INDIRECTION_BLOCK; j <
             NUM_BLOCKS_IN_INDIRECTION_BLOCK; j++) {
            tempfs_read_block_by_number(blocks_2[j], buffer3, fs, 0, fs->superblock->block_size);

            for (uint64_t k = triple_indirection_block % NUM_BLOCKS_IN_INDIRECTION_BLOCK; k <
                 NUM_BLOCKS_IN_INDIRECTION_BLOCK; k++) {
                tempfs_free_block_and_mark_bitmap(fs, blocks_3[k]);
                num_blocks--;
                if (num_blocks == 0) {
                    goto done;
                }
            }
        }
    }


done:
    inode->block_count -= num_blocks;
    kfree(buffer);
    kfree(buffer2);
    kfree(buffer3);
}


/*
 *  Functions beneath are self-explanatory
 *  Easy allocate functions for different levels of
 *  indirection. Num to allocate is how many in this indirect block to allocate,
 *  num_allocated is how many have already been allocated in this indirect block
 */

static uint64_t tempfs_allocate_triple_indirect_block(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate) {
    uint8_t* buffer = kmalloc(PAGE_SIZE);
    uint64_t* block_array = (uint64_t*)buffer;
    uint64_t index;
    uint64_t allocated;
    uint64_t triple_indirect = inode->triple_indirect == 0
                                   ? tempfs_get_free_block_and_mark_bitmap(fs)
                                   : inode->triple_indirect;
    inode->triple_indirect = triple_indirect; // could be more elegant but this is fine

    tempfs_read_block_by_number(triple_indirect, buffer, fs, 0, fs->superblock->block_size);

    if (num_allocated == 0) {
        index = 0;
        allocated = 0;
    }
    else {
        index = num_allocated / NUM_BLOCKS_DOUBLE_INDIRECTION;
        allocated = num_allocated % NUM_BLOCKS_DOUBLE_INDIRECTION;
    }


    while (num_to_allocate > 0) {
        uint64_t amount = num_to_allocate < NUM_BLOCKS_DOUBLE_INDIRECTION
                              ? num_to_allocate
                              : NUM_BLOCKS_DOUBLE_INDIRECTION;
        block_array[index++] = tempfs_allocate_double_indirect_block(fs, inode, allocated, amount,true, 0);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
    }

    tempfs_write_block_by_number(triple_indirect, buffer, fs, 0, fs->superblock->block_size);
    kfree(buffer);
    return triple_indirect;
}

static uint64_t tempfs_allocate_double_indirect_block(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number) {
    uint64_t index;
    uint64_t allocated;
    uint8_t* buffer = kmalloc(PAGE_SIZE);
    uint64_t double_indirect;

    if (higher_order && block_number == 0) {
        double_indirect = tempfs_get_free_block_and_mark_bitmap(fs);
    }
    else if (!higher_order && block_number == 0) {
        double_indirect = inode->double_indirect == 0
                              ? tempfs_get_free_block_and_mark_bitmap(fs)
                              : inode->double_indirect;
        inode->double_indirect = double_indirect;
    }
    else {
        double_indirect = block_number;
    }


    tempfs_read_block_by_number(double_indirect, buffer, fs, 0, fs->superblock->block_size);
    uint64_t* block_array = (uint64_t*)buffer;

    if (num_allocated == 0) {
        index = 0;
        allocated = 0;
    }
    else {
        index = num_allocated / NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        allocated = num_allocated % NUM_BLOCKS_IN_INDIRECTION_BLOCK;
    }


    uint64_t block;

    if (num_allocated > 0) {
        block = block_array[index];
    }
    else {
        block = 0;
    }

    while (num_to_allocate > 0) {
        uint64_t amount = num_to_allocate < NUM_BLOCKS_IN_INDIRECTION_BLOCK
                              ? num_to_allocate
                              : NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        block_array[index] = tempfs_allocate_single_indirect_block(fs, inode, allocated, amount,true, block);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
        index++;
        block = block_array[index];
    }
    tempfs_write_block_by_number(double_indirect, buffer, fs, 0, fs->superblock->block_size);
    tempfs_write_inode(fs, inode);
    kfree(buffer);
    return double_indirect;
}


static uint64_t tempfs_allocate_single_indirect_block(struct tempfs_filesystem* fs, struct tempfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number) {
    if (num_allocated + num_to_allocate >= NUM_BLOCKS_IN_INDIRECTION_BLOCK) {
        num_to_allocate = NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated;
    }

    uint8_t* buffer = kmalloc(PAGE_SIZE);
    uint64_t single_indirect = block_number;

    if (higher_order && block_number == 0) {
        single_indirect = tempfs_get_free_block_and_mark_bitmap(fs);
    }
    else if (!higher_order && block_number == 0) {
        single_indirect = inode->single_indirect == 0
                              ? tempfs_get_free_block_and_mark_bitmap(fs)
                              : inode->single_indirect;
        inode->single_indirect = single_indirect;
    }
    else {
        single_indirect = block_number;
    }

    tempfs_read_block_by_number(single_indirect, buffer, fs, 0, fs->superblock->block_size);
    /* We can probably just write over the memory which makes these reads redundant, just a note for now */
    uint64_t* block_array = (uint64_t*)buffer;
    for (uint64_t i = num_allocated; i < num_to_allocate + num_allocated; i++) {
        block_array[i] = tempfs_get_free_block_and_mark_bitmap(fs);
    }
    tempfs_write_block_by_number(single_indirect, buffer, fs, 0, fs->superblock->block_size);
    tempfs_write_inode(fs, inode);
    kfree(buffer);
    return single_indirect;
}


