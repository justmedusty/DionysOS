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

static void tempfs_modify_bitmap(struct tempfs_filesystem *fs, uint8 type, uint64 number,uint8 action);
static uint64 tempfs_get_free_block_and_mark_bitmap(struct tempfs_filesystem *fs);
static uint64 tempfs_free_block_and_mark_bitmap(struct tempfs_filesystem *fs, uint64 block_number);
static uint64 tempfs_free_inode_and_mark_bitmap(struct tempfs_filesystem *fs, uint64 inode_number);
static uint64 tempfs_get_bytes_from_inode(struct tempfs_filesystem *fs, uint8* buffer,uint64 buffer_size, uint64 inode_number, uint64 byte_start,uint64 size_to_read);
static uint64 tempfs_get_directory_entries(struct tempfs_filesystem *fs,struct tempfs_directory_entry** children, uint64 inode_number,uint64 children_size);
static struct vnode* tempfs_directory_entry_to_vnode(struct vnode *parent,struct tempfs_directory_entry* entry,struct tempfs_filesystem *fs);
static struct tempfs_byte_offset_indices tempfs_indirection_indices_for_block_number(uint64 block_number);
static void tempfs_write_inode(struct tempfs_filesystem *fs, struct tempfs_inode* inode);
static void tempfs_write_block_by_number(uint64 block_number, uint8* buffer, struct tempfs_filesystem *fs, uint64 offset, uint64 write_size);
static void tempfs_read_block_by_number(uint64 block_number, uint8* buffer, struct tempfs_filesystem *fs, uint64 offset, uint64 read_size);
static uint64 tempfs_allocate_single_indirect_block(struct tempfs_filesystem *fs,uint64 num_allocated, uint64 num_to_allocate);
static uint64 tempfs_allocate_double_indirect_block(struct tempfs_filesystem *fs,uint64 num_allocated, uint64 num_to_allocate);
static uint64 tempfs_allocate_triple_indirect_block(struct tempfs_filesystem *fs);
static void tempfs_read_inode(struct tempfs_filesystem *fs, struct tempfs_inode* inode,uint64 inode_number);
static uint64 tempfs_get_logical_block_number_from_file(struct tempfs_inode *inode,uint64 current_block,struct tempfs_filesystem *fs );

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
void tempfs_init(uint64 filesystem_id) {
    if(filesystem_id >= NUM_FILESYSTEM_OBJECTS) {
        return;
    }

    initlock(tempfs_filesystem[filesystem_id].lock,TEMPFS_LOCK);
    ramdisk_init(DEFAULT_TEMPFS_SIZE,tempfs_filesystem[filesystem_id].ramdisk_id, "initramfs");
    tempfs_mkfs(filesystem_id,&tempfs_filesystem[filesystem_id]);
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

    struct tempfs_filesystem* fs = vnode->filesystem_object;

    uint64 ret = tempfs_get_directory_entries(fs, (struct tempfs_directory_entry**)&buffer, vnode->vnode_inode_number,
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
            vnode->vnode_children[i] = tempfs_directory_entry_to_vnode(vnode,entry, fs);
        }
        /*
         * Check child so we don't strcmp every time after we find it in the case of filling the parent vnode with its children
         */
        if (child == NULL && safe_strcmp(name, entry->name,VFS_MAX_NAME_LENGTH)) {
            child = tempfs_directory_entry_to_vnode(vnode,entry, fs);
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

    struct tempfs_inode inode;
    struct tempfs_filesystem *fs = vnode->filesystem_object;
    tempfs_read_inode(fs,&inode,vnode->vnode_inode_number);
    if(inode.refcount <= 1) {

    }

}

/*
 * This will be the function that spins up an empty tempfs filesystem and then fill with a root directory and a few basic directories and files.
 *
 * Takes a ramdisk ID to specify which ramdisk to operate on
 */
void tempfs_mkfs(uint64 ramdisk_id, struct tempfs_filesystem* fs) {
    uint8* buffer = kalloc(PAGE_SIZE);

    fs->superblock->magic = TEMPFS_MAGIC;
    fs->superblock->version = TEMPFS_VERSION;
    fs->superblock->block_size = TEMPFS_BLOCKSIZE;
    fs->superblock->num_blocks = ((sizeof(fs->superblock->block_bitmap_pointers) / 8) * TEMPFS_BLOCKSIZE) * 8;
    fs->superblock->num_inodes = ((sizeof(fs->superblock->inode_bitmap_pointers) / 8) * TEMPFS_BLOCKSIZE) * 8;
    fs->superblock->inode_start_pointer = TEMPFS_START_INODES;
    fs->superblock->block_start_pointer = TEMPFS_START_BLOCKS;

    for (uint64 i = 0; i < TEMPFS_NUM_INODE_POINTER_BLOCKS; i++) {
        fs->superblock->inode_bitmap_pointers[i] = TEMPFS_START_INODE_BITMAP + i;
    }

    for (uint64 i = 0; i < TEMPFS_NUM_BLOCK_POINTER_BLOCKS; i++) {
        fs->superblock->block_bitmap_pointers[i] = TEMPFS_START_BLOCK_BITMAP + i;
    }

    fs->superblock->total_size = DEFAULT_TEMPFS_SIZE;
    fs->ramdisk_id = ramdisk_id;

    //copy the contents into our buffer
    memcpy(buffer, &tempfs_superblock, sizeof(struct tempfs_superblock));

    //Write the new superblock to the ramdisk
    ramdisk_write(buffer,TEMPFS_SUPERBLOCK, 0,TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

    memset(buffer, 0,PAGE_SIZE);

    /*
     * Fill in inode bitmap with zeros blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODE_BITMAP, 0,TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE, ramdisk_id);

    /*
     * Fill in block bitmap with zerod blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCK_BITMAP, 0,TEMPFS_NUM_BLOCK_POINTER_BLOCKS * TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE, ramdisk_id);

    /*
     *Write zerod blocks for the inode blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODES, 0,TEMPFS_NUM_INODES * TEMPFS_BLOCKSIZE,TEMPFS_BLOCKSIZE, ramdisk_id);
    /*
     *Write zerod blocks for the blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCKS, 0, TEMPFS_NUM_BLOCKS * TEMPFS_BLOCKSIZE,TEMPFS_BLOCKSIZE, ramdisk_id);

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
static struct vnode* tempfs_directory_entry_to_vnode(struct vnode *parent,struct tempfs_directory_entry* entry,struct tempfs_filesystem *fs) {
    struct vnode* vnode = vnode_alloc();
    vnode->vnode_type = entry->type;
    vnode->vnode_device_id = entry->device_number;
    vnode->vnode_size = entry->size;
    vnode->vnode_inode_number = entry->inode_number;
    vnode->vnode_filesystem_id = VNODE_FS_TEMPFS;\
    vnode->vnode_ops = &tempfs_vnode_ops;
    vnode->vnode_refcount = entry->refcount;
    vnode->is_mount_point = FALSE;
    vnode->mounted_vnode = NULL;
    vnode->is_cached = FALSE;
    *vnode->vnode_name = *entry->name;
    vnode->last_updated = 0;
    vnode->num_children = entry->type == TEMPFS_DIRECTORY
                              ? entry->size / sizeof(struct tempfs_directory_entry)
                              : 0;
    vnode->filesystem_object = fs;
    vnode->vnode_parent = parent;

    return vnode;
}

static void tempfs_directory_entry_free(struct tempfs_filesystem *fs,struct tempfs_directory_entry* entry) {

    struct tempfs_inode inode;

    tempfs_read_inode(fs,&inode,entry->parent_inode_number);




}

static void tempfs_remove_file(struct tempfs_directory_entry* entry) {

}

static void tempfs_free_blocks(struct tempfs_directory_entry* entry) {

}

static void tempfs_shift_blocks(struct tempfs_inode *inode,uint64 start_block, uint64 shift_length,uint8 shift_direction) {

}
static void tempfs_modify_bitmap(struct tempfs_filesystem *fs, uint8 type, uint64 number,
                                 uint8 action) {

    if (type != BITMAP_TYPE_BLOCK && type != BITMAP_TYPE_INODE) {
        panic("Unknown type tempfs_modify_bitmap");
    }

    uint64 block_to_read;
    uint64 block;

    if (type == BITMAP_TYPE_BLOCK) {
        block_to_read = fs->superblock->block_bitmap_pointers[number / (TEMPFS_BLOCKSIZE * 8)];
        block = fs->superblock->block_bitmap_pointers[block_to_read];
    }
    if (type == BITMAP_TYPE_INODE) {
        block_to_read = fs->superblock->inode_bitmap_pointers[number / (TEMPFS_BLOCKSIZE * 8)];
        block = fs->superblock->inode_bitmap_pointers[block_to_read];
    }

    uint64 byte_in_block = number / 8;

    uint64 bit = number % 8;

    uint8* buffer = kalloc(PAGE_SIZE);

    tempfs_read_block_by_number(block, buffer, fs, 0,TEMPFS_BLOCKSIZE);

    /*
     * 0 the bit and write it back Noting that this doesnt work for a set but Im not sure that
     * I will use it for that.
     */
    buffer[byte_in_block] = buffer[byte_in_block] & (action << bit);
    tempfs_write_block_by_number(block, buffer, fs, 0,TEMPFS_BLOCKSIZE);
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
static uint64 tempfs_get_free_inode_and_mark_bitmap(struct tempfs_filesystem *fs,
                                                    struct tempfs_inode* inode_to_be_filled) {
    uint64 buffer_size = PAGE_SIZE * 2;
    uint8* buffer = kalloc(buffer_size);
    uint64 block = 0;
    uint64 byte = 0;
    uint64 bit = 0;
    uint64 inode_number;


    uint64 ret = ramdisk_read(buffer, fs->superblock->inode_bitmap_pointers[0], 0,
                              TEMPFS_BLOCKSIZE * TEMPFS_NUM_INODE_POINTER_BLOCKS, buffer_size, fs->ramdisk_id);

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
    tempfs_write_block_by_number(fs->superblock->inode_bitmap_pointers[block], &buffer[block], fs, 0,TEMPFS_BLOCKSIZE);
    kfree(buffer);

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
static uint64 tempfs_get_free_block_and_mark_bitmap(struct tempfs_filesystem *fs) {
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
     *
     *We do not use the tempfs_read_block function because it only works with single blocks and we are reading
     *many blocks here so we will work with ramdisk functions directly.
     *
     */
    uint64 ret = ramdisk_read(buffer, fs->superblock->block_bitmap_pointers[offset], 0, buffer_size, buffer_size, fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_get_free_inode_and_mark_bitmap");
        panic("tempfs_get_free_inode_and_mark_bitmap ramdisk read failed");
        /* For diagnostic purposes , shouldn't happen if it does I want to know right away */
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
    ret = ramdisk_write(&buffer[block * TEMPFS_BLOCKSIZE], fs->superblock->block_bitmap_pointers[block], 0, TEMPFS_BLOCKSIZE,
                        buffer_size - (block * TEMPFS_BLOCKSIZE), fs->ramdisk_id);

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
static uint64 tempfs_free_block_and_mark_bitmap(struct tempfs_filesystem *fs, uint64 block_number) {

    uint8* buffer = kalloc(PAGE_SIZE);
    memset(buffer, 0, TEMPFS_BLOCKSIZE);
    uint64 block = fs->superblock->block_start_pointer + block_number;

    tempfs_write_block_by_number(block, buffer, fs, 0,TEMPFS_BLOCKSIZE);

    tempfs_modify_bitmap(fs,BITMAP_TYPE_BLOCK, block_number,BITMAP_ACTION_CLEAR);

    kfree(buffer);
    return SUCCESS;
}

/*
 * I don't think I need locks on frees, I will find out one way or another if this is true
 */
static uint64 tempfs_free_inode_and_mark_bitmap(struct tempfs_filesystem *fs, uint64 inode_number) {
    uint64 inode_number_block = inode_number / TEMPFS_INODES_PER_BLOCK;
    uint64 offset = inode_number % TEMPFS_INODES_PER_BLOCK;

    uint8* buffer = kalloc(PAGE_SIZE);

    tempfs_write_block_by_number(fs->superblock->inode_start_pointer + inode_number_block, buffer, fs, offset,
                                 TEMPFS_INODE_SIZE);

    tempfs_modify_bitmap(fs,BITMAP_TYPE_INODE, inode_number,BITMAP_ACTION_CLEAR);

    kfree(buffer);
    return SUCCESS;
}

/*
 * Unimplemented until follow_block_pointers is complete
 */
static uint64 tempfs_get_bytes_from_inode(struct tempfs_filesystem *fs, uint8* buffer,
                                          uint64 buffer_size, uint64 inode_number, uint64 byte_start,
                                          uint64 size_to_read) {

     struct tempfs_inode inode;
     tempfs_read_inode(fs,&inode,inode_number);




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
static uint64 tempfs_get_directory_entries(struct tempfs_filesystem *fs,
                                           struct tempfs_directory_entry** children, uint64 inode_number,
                                           uint64 children_size) {
    uint64 buffer_size = PAGE_SIZE;
    uint8* buffer = kalloc(buffer_size);
    struct tempfs_inode inode;
    uint64 directory_entries_read = 0;
    uint64 directory_block = 0;
    uint64 block_number = 0;

    tempfs_read_inode(fs, &inode,inode_number);

    if (inode.type != TEMPFS_DIRECTORY) {
        kfree(buffer);
        return TEMPFS_NOT_A_DIRECTORY;
    }


    while (directory_entries_read <= (children_size / sizeof(struct tempfs_directory_entry)) && directory_entries_read <
        (inode.size / sizeof(struct tempfs_directory_entry))) {
        block_number = inode.blocks[directory_block++]; /*Should be okay to leave this unrestrained since we check children size and inode size */

        tempfs_read_block_by_number(block_number, buffer, fs, 0,TEMPFS_BLOCKSIZE);

        for(uint64 i = 0; i < (TEMPFS_BLOCKSIZE / sizeof(struct tempfs_directory_entry)) ; i++) {
            if(directory_entries_read >= inode.size / sizeof(struct tempfs_directory_entry) || directory_entries_read >= children_size / sizeof(struct tempfs_directory_entry)) {
                break;
            }
            *children[directory_entries_read++] = *(struct tempfs_directory_entry*)buffer[i * sizeof(struct tempfs_directory_entry)];
        }
    }

    kfree(buffer);
    return SUCCESS;
}

/*
 * Fills a buffer with file data , following block pointers and appending bytes to the passed buffer.
 *
 * Took locks out since this will be called from a function that is locked.
 *
 */
static uint64 follow_block_pointers(struct tempfs_filesystem *fs, struct tempfs_inode* inode,
                                    uint64 num_blocks_to_read, uint8* buffer, uint64 buffer_size, uint64 offset,uint64 start_block,
                                    uint64 read_size) {

    if (buffer_size < TEMPFS_BLOCKSIZE) {
        /*
         * I'll set a sensible minimum buffer size
         */
        return TEMPFS_BUFFER_TOO_SMALL;
    }

    if(offset > TEMPFS_BLOCKSIZE) {
        panic("follow_block_pointers bad offset"); /* Should never happen, panic for visibility  */
    }

    /*
 *  If 0 is passed, try to read everything
 */
    uint64 bytes_to_read = read_size == 0 ? inode->size -(start_block * TEMPFS_BLOCKSIZE) : read_size;
    if(read_size > inode->size) {
        read_size = inode->size - (start_block * TEMPFS_BLOCKSIZE);
    }

    uint64 current_block_number = 0;
    uint64 end_block = start_block + num_blocks_to_read;
    uint64 end_offset = (read_size  - offset) % TEMPFS_BLOCKSIZE;

    /*
     *  If 0 is passed, try to read everything
     */

    uint64 bytes_read = 0;

    for(uint64 i = start_block; i < end_block; i++) {

        current_block_number =  tempfs_get_logical_block_number_from_file(inode,i,fs);

        tempfs_read_block_by_number(current_block_number, buffer, fs, offset, TEMPFS_BLOCKSIZE - offset);

        bytes_read += (TEMPFS_BLOCKSIZE - offset);

        if(offset) {
            offset = 0;
        }

        if(i+1 == end_block) {
            offset = end_offset;
        }
    }



    /*
     * Calculate block
     */

    kfree(buffer);
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
static uint64 tempfs_get_logical_block_number_from_file(struct tempfs_inode *inode,uint64 current_block,struct tempfs_filesystem *fs ) {

    uint8* temp_buffer = kalloc(PAGE_SIZE);
    const struct tempfs_byte_offset_indices indices = tempfs_indirection_indices_for_block_number(current_block);
    uint64 current_block_number = 0;
    uint64 **indirection_block = (uint64 **) temp_buffer;
    switch (indices.levels_indirection) {

    case 0:

        current_block_number = inode->blocks[indices.top_level_block_number];
        kfree(temp_buffer);
        return current_block_number;

    case 1:

        tempfs_read_block_by_number(inode->single_indirect, temp_buffer, fs, 0, TEMPFS_BLOCKSIZE);
        current_block_number = *indirection_block[indices.first_level_block_number];
        kfree(temp_buffer);
        return current_block_number;

    case 2:

        tempfs_read_block_by_number(inode->double_indirect, temp_buffer, fs, 0, TEMPFS_BLOCKSIZE);
        current_block_number = *indirection_block[indices.second_level_block_number];
        kfree(temp_buffer);
        return current_block_number;

    case 3:

        tempfs_read_block_by_number(inode->triple_indirect, temp_buffer, fs, 0, TEMPFS_BLOCKSIZE);
        current_block_number = *indirection_block[indices.third_level_block_number];
        tempfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, TEMPFS_BLOCKSIZE);
        current_block_number = *indirection_block[indices.second_level_block_number];
        kfree(temp_buffer);
        return current_block_number;

    default:
        panic("tempfs_get_next_logical_block_from_file: unknown indirection");
        return 0; /* Just to shut up the linter, this is unreachable */
    }


}

/*
 * This function will allocate new blocks to an inode. If needed it will also slide pointers down if there is a write to the beginning or
 * to the end of the file
 *
 * IMPORTANT: LOCK MUST BE HELD WHILE THIS FUNCTION IS EXECUTING.
 * ENSURE THAT THE CALLING FUNCTION HANDLES LOCKING, OR AT LEAST THE FUNCTION AT
 * THE TOP OF THE STACK FRAMES ABOVE THIS FUNCTION
 *
 */
uint64 tempfs_inode_allocate_new_blocks(struct tempfs_filesystem *fs, struct tempfs_inode* inode,
                                        uint32 num_blocks_to_allocate) {
    struct tempfs_byte_offset_indices result = {};
    uint8* buffer = kalloc(PAGE_SIZE);
    uint64 ret;
    uint64 index = 0;

    // Do not allocate blocks for a directory since they hold enough entries (90 or so at the time of writing)
    if (inode->type == TEMPFS_DIRECTORY && (num_blocks_to_allocate + (inode->size / TEMPFS_BLOCKSIZE)) >
        TEMPFS_NUM_BLOCK_POINTERS_PER_INODE) {
        serial_printf("tempfs_inode_allocate_new_block inode type not directory!\n");
        kfree(buffer);
        return TEMPFS_ERROR;
    }

    if (num_blocks_to_allocate + (inode->size / TEMPFS_BLOCKSIZE) > MAX_BLOCKS_IN_INODE) {
        serial_printf("tempfs_inode_allocate_new_block too many blocks to request!\n");
        kfree(buffer);
        return TEMPFS_ERROR;
    }

    if (inode->size % TEMPFS_BLOCKSIZE == 0) {
        result = tempfs_indirection_indices_for_block_number(inode->size / TEMPFS_BLOCKSIZE);
    }
    else {
        result = tempfs_indirection_indices_for_block_number((inode->size / TEMPFS_BLOCKSIZE) + 1);
    }

    switch (result.levels_indirection) {
        case 0:

            uint64 index = result.top_level_block_number + 1;
            while (num_blocks_to_allocate > 0) {
                inode->blocks[index] = tempfs_get_free_block_and_mark_bitmap(fs);
                index++;
                if(index == TEMPFS_NUM_BLOCK_POINTERS_PER_INODE && num_blocks_to_allocate > 1) {
                    uint64 num_to_allocate = num_blocks_to_allocate > NUM_BLOCKS_IN_INDIRECTION_BLOCK ? NUM_BLOCKS_IN_INDIRECTION_BLOCK : num_blocks_to_allocate;
                    inode->single_indirect = tempfs_allocate_single_indirect_block(fs,0,num_to_allocate);
                    num_blocks_to_allocate -= num_to_allocate;
                }

                num_blocks_to_allocate--;
            }

            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        default:
            panic("tempfs_inode_allocate_new_block: unknown indirection");
    }


}

/*
 * A much simpler index derivation function for getting a tempfs index based off a block number given. Could be for reading a block at an arbitrary offset or finding the end
 * of the file to allocate or write.
 *
 * It is fairly simple.
 *
 * We avoid off-by-ones by using the greater than as opposed to greater than or equal operator.
 *
 * Panic if the block number is too high
 */

static struct tempfs_byte_offset_indices tempfs_indirection_indices_for_block_number(uint64 block_number) {
    struct tempfs_byte_offset_indices byte_offset_indices = {};

    if(block_number < NUM_BLOCKS_DIRECT) {
        byte_offset_indices.top_level_block_number = block_number;
        byte_offset_indices.levels_indirection = 0;
        return byte_offset_indices;
    }

    if(block_number < (NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT)) {
        block_number -= (NUM_BLOCKS_DIRECT);
        byte_offset_indices.levels_indirection = 1;
        byte_offset_indices.top_level_block_number = 0;
        byte_offset_indices.first_level_block_number = block_number / NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        return byte_offset_indices;
    }

    if(block_number < (NUM_BLOCKS_DOUBLE_INDIRECTION + NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT)) {
        block_number -= (NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT);
        byte_offset_indices.levels_indirection = 2;
        byte_offset_indices.top_level_block_number = 0;
        byte_offset_indices.second_level_block_number = (block_number / NUM_BLOCKS_IN_INDIRECTION_BLOCK);
        byte_offset_indices.first_level_block_number = block_number % NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        return byte_offset_indices;
    }

    if(block_number < (NUM_BLOCKS_TRIPLE_INDIRECTION + NUM_BLOCKS_DOUBLE_INDIRECTION + NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT)) {
        block_number -= (NUM_BLOCKS_DOUBLE_INDIRECTION + NUM_BLOCKS_IN_INDIRECTION_BLOCK + NUM_BLOCKS_DIRECT);
        byte_offset_indices.levels_indirection = 3;
        byte_offset_indices.top_level_block_number = 0;
        byte_offset_indices.third_level_block_number = block_number  / NUM_BLOCKS_DOUBLE_INDIRECTION;
        byte_offset_indices.second_level_block_number = (block_number - ((byte_offset_indices.third_level_block_number * NUM_BLOCKS_DOUBLE_INDIRECTION))) / NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        byte_offset_indices.first_level_block_number = block_number % (NUM_BLOCKS_IN_INDIRECTION_BLOCK);
        return byte_offset_indices;
    }


    panic("tempfs_indirection_indices_for_block_number invalid block number");
}

/*
 * 4 functions beneath just take some of the ramdisk calls out in favor of local functions for read and writing blocks and inodes
 */
static void tempfs_write_inode(struct tempfs_filesystem *fs, struct tempfs_inode* inode) {
    uint64 inode_number_in_block = inode->inode_number % NUM_INODES_PER_BLOCK;
    uint64 block_number = fs->superblock->inode_start_pointer + inode->inode_number / NUM_INODES_PER_BLOCK;
    uint64 ret = ramdisk_write((uint8*)inode, block_number, sizeof(struct tempfs_inode) * inode_number_in_block,
                               sizeof(struct tempfs_inode), sizeof(struct tempfs_inode), fs->ramdisk_id);
    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_inode");
        panic("tempfs_write_inode"); /* For diagnostic purposes */
    }
}

static void tempfs_read_inode(struct tempfs_filesystem *fs, struct tempfs_inode* inode,uint64 inode_number) {
    uint64 inode_number_in_block = inode_number % NUM_INODES_PER_BLOCK;
    uint64 block_number = fs->superblock->inode_start_pointer + (inode_number / NUM_INODES_PER_BLOCK);
    uint64 ret = ramdisk_read((uint8*)inode, block_number, sizeof(struct tempfs_inode) * inode_number_in_block,
                               sizeof(struct tempfs_inode), sizeof(struct tempfs_inode), fs->ramdisk_id);
    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_inode");
        panic("tempfs_write_inode"); /* For diagnostic purposes */
    }
}

static void tempfs_write_block_by_number(uint64 block_number, uint8* buffer, struct tempfs_filesystem *fs,uint64 offset, uint64 write_size) {
    if (write_size > TEMPFS_BLOCKSIZE) {
        write_size = TEMPFS_BLOCKSIZE;
    }
    if (offset >= TEMPFS_BLOCKSIZE) {
        offset = 0;
    }

    uint64 ret = ramdisk_write(buffer, block_number, offset, write_size - offset, write_size, fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_block_by_number");
        panic("tempfs_write_block_by_number");
    }
}

static void tempfs_read_block_by_number(uint64 block_number, uint8* buffer, struct tempfs_filesystem *fs,uint64 offset, uint64 read_size) {
    if (read_size > TEMPFS_BLOCKSIZE) {
        read_size = TEMPFS_BLOCKSIZE - offset;
    }

    if (offset >= TEMPFS_BLOCKSIZE) {
        offset = 0;
    }

    uint64 ret = ramdisk_read(buffer, block_number, offset, read_size - offset, read_size, fs->ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_write_block_by_number");
        panic("tempfs_write_block_by_number");
    }
}

/*
 *  Functions beneath are self-explanatory
 *  Easy allocate functions
 *
 *
 *  TODO check for off-by-ones here my off-by-one senses are tingling
 */
static uint64 tempfs_allocate_triple_indirect_block(struct tempfs_filesystem *fs) {
    uint8* buffer = kalloc(PAGE_SIZE);
    uint64 triple_indirect = tempfs_get_free_block_and_mark_bitmap(fs);
    tempfs_read_block_by_number(triple_indirect, buffer, fs, 0,TEMPFS_BLOCKSIZE);
    uint64** block_array = (uint64**)buffer;
    for (uint64 i = 0; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i++) {
        *block_array[i] = tempfs_allocate_double_indirect_block(fs, 0,NUM_BLOCKS_IN_INDIRECTION_BLOCK);
    }
    tempfs_write_block_by_number(triple_indirect, buffer, fs, 0,TEMPFS_BLOCKSIZE);
    kfree(buffer);
    return triple_indirect;
}

static uint64 tempfs_allocate_double_indirect_block(struct tempfs_filesystem *fs,
                                                    uint64 num_allocated, uint64 num_to_allocate) {
    if (num_allocated + num_to_allocate > NUM_BLOCKS_IN_INDIRECTION_BLOCK) {
        num_to_allocate = NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated;
    }

    uint64 index;
    uint64 allocated;
    uint8* buffer = kalloc(PAGE_SIZE);
    uint64 double_indirect = tempfs_get_free_block_and_mark_bitmap(fs);
    tempfs_read_block_by_number(double_indirect, buffer, fs, 0,TEMPFS_BLOCKSIZE);
    uint64** block_array = (uint64**)buffer;

    if(num_allocated == 0) {
        index = 0;
        allocated = 0;
    }else {
        index = num_allocated % NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        allocated = num_allocated % NUM_BLOCKS_IN_INDIRECTION_BLOCK; // we're using a second variable here so we can easily 0 it after it's not needed. if we use index we can't zero it after.
    }
    while(num_to_allocate > 0){
        uint64 amount = num_to_allocate < NUM_BLOCKS_IN_INDIRECTION_BLOCK ? num_to_allocate : NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        *block_array[index] = tempfs_allocate_single_indirect_block(fs, allocated,amount);
        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
    }
    tempfs_write_block_by_number(double_indirect, buffer, fs, 0,TEMPFS_BLOCKSIZE);
    kfree(buffer);
    return double_indirect;
}


static uint64 tempfs_allocate_single_indirect_block(struct tempfs_filesystem *fs,uint64 num_allocated, uint64 num_to_allocate) {
    if (num_allocated + num_to_allocate > NUM_BLOCKS_IN_INDIRECTION_BLOCK) {
        num_to_allocate = NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated - 1;
    }

    uint8* buffer = kalloc(PAGE_SIZE);
    uint64 single_indirect = tempfs_get_free_block_and_mark_bitmap(fs);
    tempfs_read_block_by_number(single_indirect, buffer,fs,0,TEMPFS_BLOCKSIZE);
    /* We can probably just write over the memory which makes these reads redundant, just a note for now */
    uint64** block_array = (uint64**)buffer;
    for (uint64 i = num_allocated; i < num_to_allocate; i++) {
        *block_array[i] = tempfs_get_free_block_and_mark_bitmap(fs);
    }
    tempfs_write_block_by_number(single_indirect, buffer, fs, 0,TEMPFS_BLOCKSIZE);
    kfree(buffer);
    return single_indirect;
}


