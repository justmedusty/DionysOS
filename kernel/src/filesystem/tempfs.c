//
// Created by dustyn on 9/18/24.
//

#include "include/data_structures/spinlock.h"
#include "include/definitions.h"
#include "include/filesystem/vfs.h"
#include "include/filesystem/tempfs.h"
#include <include/definitions/string.h>
#include <include/drivers/serial/uart.h>
#include <include/mem/kalloc.h>
#include "include/drivers/block/ramdisk.h"
#include "include/mem/mem.h"
#include "include/definitions/math.h"
#include "include/arch/arch_timer.h"
struct spinlock tempfs_lock;
struct tempfs_superblock tempfs_superblock;

static uint64 tempfs_modify_bitmap(struct tempfs_superblock* sb, uint8 type, uint64 ramdisk_id, uint64 number,
                                   uint8 action);
static uint64 tempfs_get_inode(struct tempfs_superblock* sb, uint64 inode_number, uint64 ramdisk_id,
                               struct tempfs_inode* inode_to_be_filled);
static uint64 tempfs_get_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id);
static uint64 tempfs_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 block_number);
static uint64 tempfs_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 inode_number);
static uint64 tempfs_get_bytes_from_inode(struct tempfs_superblock* sb, uint64 ramdisk_id, uint8* buffer,
                                          uint64 buffer_size, uint64 inode_number, uint64 byte_start,
                                          uint64 size_to_read);
static uint64 tempfs_get_directory_entries(struct tempfs_superblock* sb, uint64 ramdisk_id,
                                           struct tempfs_directory_entry** children, uint64 inode_number,
                                           uint64 children_size);
static struct vnode* tempfs_directory_entry_to_vnode(struct tempfs_directory_entry* entry);
static struct tempfs_indirection_index tempfs_indirection_indices(uint64 block_count);

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
 *  I have opted to go with this flow.
 *
 *  Read ramdisk structure into temporary variable.
 *
 *  Write temporary local variable.
 *
 *  Write said variable to the ramdisk.
 *
 *  We can improve performance by passing pointers directly to ramdisk memory
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
void tempfs_init() {
    initlock(&tempfs_lock,TEMPFS_LOCK);
    ramdisk_init(DEFAULT_TEMPFS_SIZE,INITRAMFS, "initramfs");
    tempfs_mkfs(INITRAMFS);
};

void tempfs_remove(struct vnode* vnode) {
}

void tempfs_rename(struct vnode* vnode, char* new_name) {
}

uint64 tempfs_read(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes) {
}

uint64 tempfs_write(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes) {
}

uint64 tempfs_stat(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes) {
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
struct vnode* tempfs_lookup(struct vnode* vnode, char* name) {
    if (vnode->vnode_filesystem_id != VNODE_FS_TEMPFS || vnode->vnode_type != VNODE_DIRECTORY) {
        return NULL;
    }

    uint64 buffer_size = TEMPFS_BLOCKSIZE * TEMPFS_NUM_BLOCK_POINTERS_PER_INODE;

    uint8* buffer = kalloc(TEMPFS_BLOCKSIZE * TEMPFS_NUM_BLOCK_POINTERS_PER_INODE);

    uint64 ret = tempfs_get_directory_entries(&tempfs_superblock, vnode->vnode_device_id,
                                              (struct tempfs_directory_entry**)&buffer, vnode->vnode_inode_number,
                                              buffer_size);

    if (ret != SUCCESS) {
        kfree(buffer);
        return NULL;
    }

    uint64 fill_vnode = 0;

    if (!vnode->is_cached) {
        fill_vnode = 1;
    }

    if (fill_vnode && !(vnode->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED)) {
        vnode_directory_alloc_children(vnode);
    }

    struct vnode* child = NULL;

    uint8 max_directories = vnode->vnode_size / sizeof(struct tempfs_directory_entry) > VNODE_MAX_DIRECTORY_ENTRIES
                                ? VNODE_MAX_DIRECTORY_ENTRIES
                                : vnode->vnode_size / sizeof(struct tempfs_directory_entry);

    for (uint64 i = 0; i < max_directories; i++) {
        struct tempfs_directory_entry* entry = (struct tempfs_directory_entry*)&buffer[i];
        if (fill_vnode) {
            vnode->vnode_children[i] = tempfs_directory_entry_to_vnode(entry);
        }
        /*
         * Check child so we don't strcmp every time after we find it in the case of filling the parent vnode with its children
         */
        if (child == NULL && safe_strcmp(name, entry->name,VFS_MAX_NAME_LENGTH)) {
            child = tempfs_directory_entry_to_vnode(entry);
            if (!fill_vnode) {
                goto done;
            }
        }

        if (i == max_directories) {
            memset(vnode->vnode_children[i + 1], 0, sizeof(struct vnode));
        }
    }
    vnode->is_cached = TRUE;

done:
    return child;
}

struct vnode* tempfs_create(struct vnode* vnode, struct vnode* new_vnode, uint8 vnode_type) {
    if (vnode->vnode_filesystem_id != VNODE_FS_TEMPFS) {
        return NULL;
    }
}

void tempfs_close(struct vnode* vnode) {
}

uint64 tempfs_open(struct vnode* vnode) {
}

struct vnode* tempfs_link(struct vnode* vnode, struct vnode* new_vnode) {
}

void tempfs_unlink(struct vnode* vnode) {
}

/*
 * This will be the function that spins up an empty tempfs filesystem and then fill with a root directory and a few basic directories and files.
 *
 * Takes a ramdisk ID to specify which ramdisk to operate on
 */
void tempfs_mkfs(uint64 ramdisk_id) {
    uint8* buffer = kalloc(PAGE_SIZE);

    tempfs_superblock.magic = TEMPFS_MAGIC;
    tempfs_superblock.version = TEMPFS_VERSION;
    tempfs_superblock.block_size = TEMPFS_BLOCKSIZE;
    tempfs_superblock.num_blocks = ((sizeof(tempfs_superblock.block_bitmap_pointers) / 8) * TEMPFS_BLOCKSIZE) * 8;
    tempfs_superblock.num_inodes = ((sizeof(tempfs_superblock.inode_bitmap_pointers) / 8) * TEMPFS_BLOCKSIZE) * 8;
    tempfs_superblock.inode_start_pointer = TEMPFS_START_INODES;
    tempfs_superblock.block_start_pointer = TEMPFS_START_BLOCKS;

    for (uint64 i = 0; i < TEMPFS_NUM_INODE_POINTER_BLOCKS; i++) {
        tempfs_superblock.inode_bitmap_pointers[i] = TEMPFS_START_INODE_BITMAP + i;
    }

    for (uint64 i = 0; i < TEMPFS_NUM_BLOCK_POINTER_BLOCKS; i++) {
        tempfs_superblock.block_bitmap_pointers[i] = TEMPFS_START_BLOCK_BITMAP + i;
    }

    tempfs_superblock.total_size = DEFAULT_TEMPFS_SIZE;

    //copy the contents into our buffer
    memcpy(buffer, &tempfs_superblock, sizeof(struct tempfs_superblock));

    //Write the new superblock to the ramdisk
    ramdisk_write(buffer,TEMPFS_SUPERBLOCK, 0,TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

    memset(buffer, 0,PAGE_SIZE);

    /*
     * Fill in inode bitmap with zeros blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODE_BITMAP, 0,TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE,INITRAMFS);

    /*
     * Fill in block bitmap with zerod blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCK_BITMAP, 0,TEMPFS_NUM_BLOCK_POINTER_BLOCKS * TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE,INITRAMFS);

    /*
     *Write zerod blocks for the inode blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODES, 0,TEMPFS_NUM_INODES * TEMPFS_BLOCKSIZE,TEMPFS_BLOCKSIZE,INITRAMFS);
    /*
     *Write zerod blocks for the blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCKS, 0, TEMPFS_NUM_BLOCKS * TEMPFS_BLOCKSIZE,TEMPFS_BLOCKSIZE,INITRAMFS);

    kfree(buffer);
    serial_printf("Tempfs empty filesystem initialized\n");
}


/*
*******************************************************************************************
*******************************************************************************************
*******************************************************************************************
*******************************************************************************************
*******************************************************************************************
*******************************************************************************************
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
static struct vnode* tempfs_directory_entry_to_vnode(struct tempfs_directory_entry* entry) {
    struct vnode* vnode = vnode_alloc();
    vnode->vnode_type = entry->type;
    vnode->vnode_device_id = entry->device_number;
    vnode->vnode_size = entry->size;
    vnode->vnode_inode_number = entry->inode_number;
    vnode->vnode_filesystem_id = VNODE_FS_TEMPFS;\
    vnode->vnode_ops = &tempfs_vnode_ops;
    vnode->vnode_refcount = entry->refcount;
    vnode->vnode_parent = vnode;
    vnode->is_mount_point = FALSE;
    vnode->mounted_vnode = NULL;
    vnode->is_cached = FALSE;
    *vnode->vnode_name = *entry->name;
    vnode->last_updated = 0;
    vnode->num_children = entry->type == TEMPFS_DIRECTORY
                              ? entry->size / sizeof(struct tempfs_directory_entry)
                              : 0;

    return vnode;
}

static uint64 tempfs_modify_bitmap(struct tempfs_superblock* sb, uint8 type, uint64 ramdisk_id, uint64 number,
                                   uint8 action) {
    acquire_spinlock(&tempfs_lock);
    if (type != BITMAP_TYPE_BLOCK && type != BITMAP_TYPE_INODE) {
        goto unknown_code;
    }

    uint64 block_to_read;
    uint64 block;

    if (type == BITMAP_TYPE_BLOCK) {
        block_to_read = sb->block_bitmap_pointers[number / (TEMPFS_BLOCKSIZE * 8)];
        block = sb->block_bitmap_pointers[block_to_read];
    }
    if (type == BITMAP_TYPE_INODE) {
        block_to_read = sb->inode_bitmap_pointers[number / (TEMPFS_BLOCKSIZE * 8)];
        block = sb->inode_bitmap_pointers[block_to_read];
    }

    uint64 byte_in_block = number / 8;

    uint64 bit = number % 8;

    uint8* buffer = kalloc(PAGE_SIZE);
    uint64 ret = ramdisk_read(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_modify_bitmap read")
        release_spinlock(&tempfs_lock);
        return ret;
    }
    /*
     * 0 the bit and write it back Noting that this doesnt work for a set but Im not sure that
     * I will use it for that.
     */
    buffer[byte_in_block] = buffer[byte_in_block] & (action << bit);

    ret = ramdisk_write(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_modify_bitmap read")
        release_spinlock(&tempfs_lock);
        return ret;
    }

    kfree(buffer);
    release_spinlock(&tempfs_lock);

    return SUCCESS;

unknown_code :
    panic("Unknown type tempfs_modify_bitmap");
}

/*
 *  Reads inode at place inode_number on ramdisk of ramdisk_id and fills the passed inode_to_be_filled with the ramdisk inode
 *  I amn going to try and go lock-less since I think I can get away with it for this function.
 */
static uint64 tempfs_get_inode(struct tempfs_superblock* sb, uint64 inode_number, uint64 ramdisk_id,
                               struct tempfs_inode* inode_to_be_filled) {
    uint64 inode_block = sb->inode_start_pointer + (sb->block_size / TEMPFS_INODE_SIZE);
    uint64 inode_in_block = inode_number % (sb->block_size / TEMPFS_INODE_SIZE);

    uint8* block = kalloc(PAGE_SIZE);
    /* I will need to add the tempfs block size to the heap until then I'll just allocate a page*/

    uint64 ret = ramdisk_read(block, inode_block, inode_in_block * TEMPFS_INODE_SIZE,TEMPFS_INODE_SIZE,PAGE_SIZE,
                              ramdisk_id);

    if (ret == SUCCESS) {
        *inode_to_be_filled = *(struct tempfs_inode*)block;
        kfree(block);
        return SUCCESS;
    }

    HANDLE_RAMDISK_ERROR(ret, "tempfs_get_inode")
    kfree(block);
    return ret;
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
static uint64 tempfs_get_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id,
                                                    struct tempfs_inode* inode_to_be_filled) {
    acquire_spinlock(&tempfs_lock);
    uint64 buffer_size = PAGE_SIZE * 2;
    uint8* buffer = kalloc(buffer_size);
    uint64 block = 0;
    uint64 byte = 0;
    uint64 bit = 0;
    uint64 inode_number;


    uint64 ret = ramdisk_read(buffer, sb->inode_bitmap_pointers[0], 0,
                              TEMPFS_BLOCKSIZE * TEMPFS_NUM_INODE_POINTER_BLOCKS, buffer_size, ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap");
        panic("tempfs_get_free_inode_and_mark_bitmap ramdisk read failed");
    }

    while (1) {
        if (buffer[(block * TEMPFS_BLOCKSIZE) + byte] != 0xFF) {
            for (uint64 i = 0; i <= 8; i++) {
                if (!(buffer[(block * TEMPFS_BLOCKSIZE) + byte] & (1 << i))) {
                    bit = i;
                    buffer[byte] |= (1 << bit);
                    inode_number = (block * TEMPFS_BLOCKSIZE * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte == TEMPFS_BLOCKSIZE) {
                block++;
                byte = 0;
            }
            if (block > TEMPFS_NUM_INODE_POINTER_BLOCKS) {
                panic("tempfs_get_free_inode_and_mark_bitmap: No free inodes");
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        }
    }


found_free:
    ret = ramdisk_write(&buffer[block], sb->inode_bitmap_pointers[block], 0,TEMPFS_BLOCKSIZE, buffer_size, ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap ramdisk_write call")
        panic("tempfs_get_free_inode_and_mark_bitmap ramdisk write failed");
    }

    kfree(buffer);
    release_spinlock(&tempfs_lock);

    return inode_number;
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
static uint64 tempfs_get_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id) {
    acquire_spinlock(&tempfs_lock);
    uint64 buffer_size = PAGE_SIZE * 16;
    uint8* buffer = kalloc(buffer_size);
    uint64 block = 0;
    uint64 byte = 0;
    uint64 bit = 0;
    uint64 offset = 0;
    uint64 block_number = 0;

retry:

    /*
     *We do not free the buffer we simply write into a smaller and smaller portion of the buffer.
     *It is only freed after a block is found and the new bitmap block is written.
     */

    uint64 ret = ramdisk_read(buffer, sb->block_bitmap_pointers[offset], 0, buffer_size, buffer_size, ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap");
        release_spinlock(&tempfs_lock);
        return ret;
    }

    while (1) {
        if (buffer[(block * TEMPFS_BLOCKSIZE) + byte] != 0xFF) {
            /* Iterate through every bit in the byte to find the first free bit */
            for (uint64 i = 0; i <= 8; i++) {
                if (!(buffer[buffer[(block * TEMPFS_BLOCKSIZE)] + byte] & (1 << i))) {
                    bit = i;
                    buffer[byte] |= (1 << bit);
                    block_number = ((block * TEMPFS_BLOCKSIZE * 8) + (8 * byte)) + i;
                    goto found_free;
                }
            }

            byte++;

            if (byte == TEMPFS_BLOCKSIZE) {
                block++;
                byte = 0;
                /* We need to reset byte every block increment to avoid having a bad block number calculation at the end */
            }

            if (block == TEMPFS_NUM_BLOCK_POINTER_BLOCKS) {
                panic("tempfs_get_free_block_and_mark_bitmap: No free blocks");
                /* Panic for visibility, so I can tweak this if it happens */
            }

            if (block * TEMPFS_BLOCKSIZE == buffer_size) {
                offset = block * TEMPFS_BLOCKSIZE;
                offset += buffer_size; /* Keep track of how far into the search we are */
                /* Total size is 28.5 pages at the time of writing. We start with 16 pages, then 8 , then 4 , then finally we can get the last half page in a 2 page buffer*/
                goto retry;
            }
        }
    }


found_free:
    ret = ramdisk_write(&buffer[block * TEMPFS_BLOCKSIZE], sb->block_bitmap_pointers[block], 0, TEMPFS_BLOCKSIZE,
                        buffer_size - (block * TEMPFS_BLOCKSIZE), ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap ramdisk_write call")
        panic("tempfs_get_free_inode_and_mark_bitmap"); /* Extreme but that is okay for diagnosing issues */
    }

    kfree(buffer);
    /* Free the buffer, all other control paths lead to panic so until that changes this is the only place it is required */

    release_spinlock(&tempfs_lock);
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
static uint64 tempfs_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 block_number) {
    acquire_spinlock(&tempfs_lock);

    uint8* buffer = kalloc(PAGE_SIZE);
    memset(buffer, 0, TEMPFS_BLOCKSIZE);
    uint64 block = sb->block_start_pointer + block_number;
    uint64 ret;
    ret = ramdisk_write(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);
    // zero it , this is heavy but that is ok for now

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap ramdisk_write call")
        panic("tempfs_free_block"); /* Extreme but that is okay for diagnosing issues */
    }

    ret = tempfs_modify_bitmap(sb,BITMAP_TYPE_BLOCK, ramdisk_id, block_number,BITMAP_ACTION_CLEAR);

    if (ret != SUCCESS) {
        panic("tempfs_free_block_and_mark_bitmap"); /* For visibility for changes later */
    }
    kfree(buffer);
    release_spinlock(&tempfs_lock);
    return SUCCESS;
}

/*
 * I don't think I need locks on frees, I will find out one way or another if this is true
 */
static uint64 tempfs_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 inode_number) {
    uint64 inode_number_block = inode_number / TEMPFS_INODES_PER_BLOCK;
    uint64 offset = inode_number % TEMPFS_INODES_PER_BLOCK;

    uint8* buffer = kalloc(PAGE_SIZE);

    uint64 ret;
    ret = ramdisk_write(buffer, sb->inode_start_pointer + inode_number_block, offset * TEMPFS_INODE_SIZE,
                        TEMPFS_INODE_SIZE,PAGE_SIZE, ramdisk_id); // zero it , this is heavy but that is ok for now

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap ramdisk_write call")
        panic("tempfs_free_block"); /* Extreme but that is okay for diagnosing issues */
    }

    ret = tempfs_modify_bitmap(sb,BITMAP_TYPE_INODE, ramdisk_id, inode_number,BITMAP_ACTION_CLEAR);
    kfree(buffer);
    if (ret != SUCCESS) {
        panic("tempfs_free_inode_and_mark_bitmap"); /* For visibility for changes later */
    }
    return SUCCESS;
}

/*
 * Unimplemented until follow_block_pointers is complete
 */
static uint64 tempfs_get_bytes_from_inode(struct tempfs_superblock* sb, uint64 ramdisk_id, uint8* buffer,
                                          uint64 buffer_size, uint64 inode_number, uint64 byte_start,
                                          uint64 size_to_read) {
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
static uint64 tempfs_get_directory_entries(struct tempfs_superblock* sb, uint64 ramdisk_id,
                                           struct tempfs_directory_entry** children, uint64 inode_number,
                                           uint64 children_size) {
    uint64 buffer_size = PAGE_SIZE; // 16 blocks
    uint8* buffer = kalloc(buffer_size);

    struct tempfs_inode inode;

    uint64 ret = tempfs_get_inode(sb, inode_number, ramdisk_id, &inode);

    if (ret != SUCCESS) {
        panic("tempfs_get_inode"); /* For easy diagnosis, shouldn't happen anyway */
    }

    if (inode.type != TEMPFS_DIRECTORY) {
        kfree(buffer);
        return TEMPFS_NOT_A_DIRECTORY;
    }

    uint64 directory_entries_read = 0;
    uint64 block_number = 0;

    while (directory_entries_read <= (children_size / sizeof(struct tempfs_directory_entry)) && directory_entries_read <
        (inode.size / sizeof(struct tempfs_directory_entry))) {
        block_number = inode.blocks[directory_entries_read++];
        ret = ramdisk_read(buffer, block_number, 0,TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);
        if (ret != SUCCESS) {
            HANDLE_RAMDISK_ERROR(ret, "tempfs_get_directory_entries ramdisk_read call")
            panic("tempfs_free_block"); /* Extreme but that is okay for diagnosing issues */
        }

        *children[directory_entries_read] = *(struct tempfs_directory_entry*)buffer;
    }

    kfree(buffer);
    return SUCCESS;
}

/*
 * Under Construction (imagine some big traffic cones around) \
 *
 * I will put locks just in case it is being written at the time of this block following escapade
 */
static uint64 follow_block_pointers(struct tempfs_superblock* sb, uint64 ramdisk_id, struct tempfs_inode* inode,
                                    uint64 num_blocks_to_read, uint8* buffer, uint64 buffer_size, uint64 offset,
                                    uint64 read_size) {
    acquire_spinlock(&tempfs_lock);
    if (buffer_size < TEMPFS_BLOCKSIZE) {
        /*
         * I'll set a sensible minimum buffer size
         */
        release_spinlock(&tempfs_lock);
        return TEMPFS_BUFFER_TOO_SMALL;
    }


    uint64 start_block = offset / TEMPFS_BLOCKSIZE;
    uint64 end_block = start_block + num_blocks_to_read;
    uint64 current_block_to_read = offset / TEMPFS_BLOCKSIZE;
    uint64 actual_block_number = 0;

    /*
     *  If 0 is passed, try to read everything
     */

    uint64 bytes_to_read = read_size == 0 ? inode->size : read_size;
    uint64 bytes_read = 0;

    uint8* temp_buffer = kalloc(TEMPFS_BLOCKSIZE);

    /*
     * We need to calculate whether we have indirect blocks to deal with
     */
    uint64 extra_blocks = (inode->size > (TEMPFS_NUM_BLOCK_POINTERS_PER_INODE * 1024))
                              ? ((inode->size - (TEMPFS_NUM_BLOCK_POINTERS_PER_INODE * 1024)) / TEMPFS_BLOCKSIZE)
                              : 0;
    uint64 max_indirection_in_one_block = pow(NUM_BLOCKS_IN_INDIRECTION_BLOCK,MAX_LEVEL_INDIRECTIONS);
    uint64 total_levels_max_indirection;
    if (extra_blocks) {
        total_levels_max_indirection = extra_blocks / max_indirection_in_one_block;
    }
    else {
        total_levels_max_indirection = 0;
    }

    uint64 top_level_block;
    uint64 indirect_block_1;
    uint64 indirect_block_2;
    uint64 indirect_block_3;

    if (total_levels_max_indirection > 0) {
    }
    else {
        actual_block_number = inode->blocks[current_block_to_read];
        goto no_indirection;
    }

indirection:


no_indirection:
    while (bytes_read < bytes_to_read) {
        uint64 bytes_left = buffer_size < TEMPFS_BLOCKSIZE ? buffer_size : TEMPFS_BLOCKSIZE - offset;

        uint64 ret = ramdisk_read(temp_buffer, actual_block_number, offset, bytes_to_read, bytes_left, ramdisk_id);

        if (ret != SUCCESS) {
            HANDLE_RAMDISK_ERROR(ret, "tempfs_follow_block_pointer ramdisk_read call")
            panic("tempfs_follow_block_pointer ramdisk_read error");
            /* Extreme but that is okay for diagnosing issues */
        }

        memcpy(buffer + bytes_read, temp_buffer, TEMPFS_BLOCKSIZE - offset);

        bytes_read += (buffer_size);
        buffer_size -= bytes_read;
        offset = 0;
        current_block_to_read++;

        if (current_block_to_read > TEMPFS_NUM_BLOCK_POINTERS_PER_INODE) {
            break;
        }

        if (buffer_size <= 0) {
            break;
        }

        actual_block_number = inode->blocks[current_block_to_read];
    }


    kfree(buffer);
    release_spinlock(&tempfs_lock);
    return SUCCESS;
}

/*
 * This function will allocate new blocks to an inode. If needed it will also slide pointers down if there is a write to the beginning or
 * to the end of the file
 */
uint64 tempfs_inode_allocate_new_blocks(struct tempfs_superblock* sb, uint64 ramdisk_id, struct tempfs_inode* inode,
                                        uint32 num_blocks_to_allocate) {
    // Do not allocate blocks for a directory since they hold enough entries (90 or so at the time of writing)
    if (inode->type == TEMPFS_DIRECTORY && (num_blocks_to_allocate + (inode->size / TEMPFS_BLOCKSIZE)) >
        TEMPFS_NUM_BLOCK_POINTERS_PER_INODE) {
        serial_printf("tempfs_inode_allocate_new_block inode type not directory!\n");
        return TEMPFS_ERROR;
    }

    if (num_blocks_to_allocate > MAX_BLOCKS_IN_INODE - TEMPFS_NUM_BLOCK_POINTERS_PER_INODE) {
        serial_printf("tempfs_inode_allocate_new_block too many blocks to request!\n");
        return TEMPFS_ERROR;
    }

    struct tempfs_indirection_index result = {};

    if (inode->size % TEMPFS_BLOCKSIZE == 0) {
        result = tempfs_indirection_indices(inode->size / TEMPFS_BLOCKSIZE);
    }
    else {
        result = tempfs_indirection_indices((inode->size / TEMPFS_BLOCKSIZE) + 1);
    }





    for (uint64 i = 0; i < num_blocks_to_allocate; i++) {
    }
}

static struct tempfs_indirection_index tempfs_indirection_indices(uint64 block_count) {
    struct tempfs_indirection_index result = {};

    if (block_count < TEMPFS_NUM_BLOCK_POINTERS_PER_INODE) {
        result.num_full_indirections = 0;
        result.num_second_indirections = 0;
        result.num_singular_indirections = 0;
        result.num_singular_indirections = 0;
        return result;
    }

    uint64 extra_blocks = block_count - TEMPFS_NUM_BLOCK_POINTERS_PER_INODE;

    result.num_full_indirections = extra_blocks / pow(NUM_BLOCKS_IN_INDIRECTION_BLOCK, 3);
    result.num_second_indirections = result.num_full_indirections == 0
                                         ? extra_blocks / pow(NUM_BLOCKS_IN_INDIRECTION_BLOCK, 2)
                                         : (extra_blocks % pow(NUM_BLOCKS_IN_INDIRECTION_BLOCK, 3)) / pow(
                                             NUM_BLOCKS_IN_INDIRECTION_BLOCK, 2);
    result.num_singular_indirections = result.num_second_indirections == 0
                                           ? extra_blocks / NUM_BLOCKS_IN_INDIRECTION_BLOCK
                                           : ((extra_blocks % pow(NUM_BLOCKS_IN_INDIRECTION_BLOCK, 3)) % pow(
                                               NUM_BLOCKS_IN_INDIRECTION_BLOCK, 2)) / NUM_BLOCKS_IN_INDIRECTION_BLOCK;
    result.num_block_pointers = extra_blocks % NUM_BLOCKS_IN_INDIRECTION_BLOCK;

    return result;
}

