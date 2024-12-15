//
// Created by dustyn on 12/15/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mkdiosfs.h"

#include <stdbool.h>

uint64_t strtoll_wrapper(const char* arg);
void write_block(const uint64_t block_numer,char *disk_buffer, const char * block_buffer);
static uint64_t diosfs_allocate_single_indirect_block(const , struct diosfs_inode* inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate,
                                                      const bool higher_order, const uint64_t block_number);
static uint64_t diosfs_allocate_double_indirect_block(, struct diosfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);
static uint64_t diosfs_allocate_triple_indirect_block(, struct diosfs_inode* inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate);
static void diosfs_write_inode(const , struct diosfs_inode* inode);
static struct diosfs_byte_offset_indices diosfs_indirection_indices_for_block_number(uint64_t block_number);
uint64_t diosfs_inode_allocate_new_blocks(, struct diosfs_inode* inode,
                                          uint32_t num_blocks_to_allocate);
static uint64_t diosfs_get_relative_block_number_from_file(const struct diosfs_inode* inode,
                                                           const uint64_t current_block,
                                                           );
static uint64_t diosfs_write_bytes_to_inode(, struct diosfs_inode* inode, char* buffer,
                                            const uint64_t buffer_size,
                                            const uint64_t offset,
                                            const uint64_t write_size_bytes);
static uint64_t diosfs_write_dirent(, struct diosfs_inode* inode,
                                    const struct diosfs_directory_entry* entry);
static uint64_t diosfs_get_free_block_and_mark_bitmap(const );
uint64_t diosfs_create(struct diosfs_inode* parent, char* name, const uint8_t vnode_type);
void read_block(const uint64_t block_numer, const char *disk_buffer, char * block_buffer);
static void diosfs_read_inode(const , struct diosfs_inode* inode, uint64_t inode_number);



void diosfs_get_size_info(struct diosfs_size_calculation* size_calculation, size_t gigabytes,size_t block_size) {
  if (gigabytes == 0) {
    return;
  }

  const uint64_t size_bytes = gigabytes << 30;

  size_calculation->total_blocks = size_bytes / block_size;

  uint64_t split = size_calculation->total_blocks / 32;

  size_calculation->total_inodes = split * (block_size / sizeof(struct diosfs_inode));
  size_calculation->total_data_blocks = size_calculation->total_blocks - split;
  size_calculation->total_block_bitmap_blocks = size_calculation->total_blocks / block_size / 8;
  size_calculation->total_inode_bitmap_blocks = size_calculation->total_inodes / 8 / block_size;

  size_calculation->total_blocks += 1; // superblock;
}

/*
 * mkdiosfs is the name of the executable
 *
 * Supports sizes 1-8 GB in GB increments, it will just load the entire disk in memory to make it a little simpler. Need to ensure
 * you have enough memory. My machine has more RAM than I ever use so this sort of thing is ok for me.
 *
 * It can take files as input, by default it will just have the following directories :
 *
 *  root
 *  bin
 *
 */
int main(const int argc, char **argv){

  if(argc < 4){
    printf("Usage: mkdiosfs name.img size-gigs\n");
    printf("You can add an archive via: mkdiosfs name.img size-gigs block-size --archive archive-name\n");
    exit(1);
  }

  uint64_t arg2 = strtoll_wrapper(argv[2]);

  if (arg2 > 8) {
    printf("This tool does not support images above 8GB, pick a size between 1 and 8 GB\n");
    exit(1);
  }

  FILE *f = fopen(argv[1],"w+");

  if(!f) {
    printf("Error creating file\n");
    exit(1);
  }

  char *disk_buffer = malloc(arg2);

  if(!disk_buffer) {
    printf("Error allocating disk buffer\n");
    perror("malloc");
    exit(1);
  }

  char* block = malloc(DIOSFS_BLOCKSIZE);

  if(!block) {
    printf("Error allocating block\n");
    perror("malloc");
    exit(1);
  }

  memset(block,0,*argv[3]);
  struct diosfs_superblock *superblock = (struct diosfs_superblock *) block;

  struct diosfs_size_calculation size_calculation = {0};

  diosfs_get_size_info(&size_calculation,arg2,DIOSFS_BLOCKSIZE);

  superblock->magic = DIOSFS_MAGIC;
  superblock->version = DIOSFS_VERSION;
  superblock->block_size = DIOSFS_BLOCKSIZE;
  superblock->num_blocks = size_calculation.total_blocks;
  superblock->num_inodes = size_calculation.total_inodes;
  superblock->inode_start_pointer = size_calculation.total_inode_bitmap_blocks + size_calculation.total_block_bitmap_blocks + 1 ; // 1 for superblock
  superblock->block_start_pointer = superblock->inode_start_pointer + (size_calculation.total_inodes / NUM_INODES_PER_BLOCK);
  superblock->inode_bitmap_pointers_start = 1;
  superblock->block_bitmap_pointers_start = DIOSFS_START_BLOCK_BITMAP;
  superblock->block_bitmap_size = size_calculation.total_block_bitmap_blocks;
  superblock->inode_bitmap_size = size_calculation.total_inode_bitmap_blocks;
  superblock->total_size = size_calculation.total_blocks * superblock->block_size;
  memset(superblock->reserved,0,sizeof(superblock->reserved));
  write_block(0,disk_buffer,block);

  memset(block,0,DIOSFS_BLOCKSIZE);
  //write zerod blocks for bitmap
  for (int i = 0; i < size_calculation.total_inode_bitmap_blocks + size_calculation.total_block_bitmap_blocks; i++) {

  }


  free(block);
  free(disk_buffer);

}

uint64_t strtoll_wrapper(const char* arg) {
  char *end_ptr;
  uint64_t result = strtoull(arg,&end_ptr,10);
  if (*end_ptr != '\0') {
    printf("Error converting string\n");
    exit(1);
  }

  return result;
}

void write_block(const uint64_t block_numer,char *disk_buffer, const char * block_buffer) {
  memcpy(disk_buffer + block_numer*DIOSFS_BLOCKSIZE,block_buffer,DIOSFS_BLOCKSIZE);
}

void read_block(const uint64_t block_numer, const char *disk_buffer, char * block_buffer) {
  memcpy(block_buffer,disk_buffer + block_numer*DIOSFS_BLOCKSIZE,DIOSFS_BLOCKSIZE);
}

void read_inode()


void write_inode(const , struct diosfs_inode* inode) {

}

uint64_t diosfs_create(struct diosfs_inode* parent, char* name, const uint8_t vnode_type) {

    struct diosfs_inode inode;
    diosfs_get_free_inode_and_mark_bitmap(parent, &inode);
    diosfs_read_inode(parent->filesystem_object, &parent_inode, parent->vnode_inode_number);
    parent->vnode_size++;
    parent_inode.size++;
    new_vnode->is_cached = false;
    new_vnode->vnode_type = vnode_type;
    new_vnode->vnode_inode_number = inode.inode_number;
    new_vnode->filesystem_object = parent->filesystem_object;
    new_vnode->vnode_refcount = 1;
    new_vnode->vnode_children = NULL;
    new_vnode->vnode_device_id = parent->vnode_device_id;
    new_vnode->vnode_ops = &diosfs_vnode_ops;
    new_vnode->vnode_parent = parent;
    new_vnode->vnode_filesystem_id = parent->vnode_filesystem_id;
    safe_strcpy(new_vnode->vnode_name, name, MAX_FILENAME_LENGTH);


    inode.type = vnode_type;
    inode.block_count = 0;
    inode.parent_inode_number = inode.inode_number;
    strcpy((char*)&inode.name, name);
    inode.uid = 0;
    diosfs_write_inode(parent->filesystem_object, &inode);

    struct diosfs_directory_entry entry = {0};
    entry.inode_number = inode.inode_number;
    entry.size = 0;
    strcpy(entry.name, name);
    entry.parent_inode_number = inode.parent_inode_number;
    entry.device_number = fs->device->device_id;
    entry.type = inode.type;
    uint64_t ret = diosfs_write_dirent(fs, &parent_inode, &entry);
    diosfs_write_inode(fs, parent);
    return new_vnode;
}


static uint64_t diosfs_get_free_block_and_mark_bitmap() {
    uint64_t buffer_size = DIOSFS_BLOCKSIZE * 128;
    char* buffer = malloc(buffer_size);
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
     *We do not use the diosfs_read_block function because it only works with single blocks and we are reading
     *many blocks here so we will work with ramdisk functions directly.
     *
     */

    uint64_t ret = ramdisk_read(buffer, fs->superblock->block_bitmap_pointers_start, 0,
                               DIOSFS_BLOCKSIZE * DIOSFS_NUM_BLOCK_POINTER_BLOCKS, buffer_size,
                                fs->device->device_id);


    if (ret != DIOSFS_SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "diosfs_get_free_inode_and_mark_bitmap");
        panic("diosfs_get_free_inode_and_mark_bitmap ramdisk read failed");
        /* For diagnostic purposes , shouldn't happen if it does I want to know right away */
    }

    while (1) {
        if (buffer[block *DIOSFS_BLOCKSIZE + byte] != 0xFF) {
            for (uint64_t i = 0; i <= 8; i++) {
                if (!(buffer[(block *DIOSFS_BLOCKSIZE) + byte] & (1 << i))) {
                    bit = i;
                    buffer[(block *DIOSFS_BLOCKSIZE) + byte] |= (1 << bit);
                    block_number = (block *DIOSFS_BLOCKSIZE * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte ==DIOSFS_BLOCKSIZE) {
                block++;
                byte = 0;
            }
            if (block > DIOSFS_NUM_BLOCK_POINTER_BLOCKS) {
                panic("diosfs_get_free_inode_and_mark_bitmap: No free inodes");
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        }
        else {
            byte++;
        }
    }


found_free:
    ret = ramdisk_write(buffer, fs->superblock->block_bitmap_pointers_start, 0,
                       DIOSFS_BLOCKSIZE * DIOSFS_NUM_BLOCK_POINTER_BLOCKS, buffer_size,
                        fs->device->device_id);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "diosfs_get_free_inode_and_mark_bitmap ramdisk_write call")
        panic("diosfs_get_free_inode_and_mark_bitmap"); /* Extreme but that is okay for diagnosing issues */
    }

    free(buffer);
    /* Free the buffer, all other control paths lead to panic so until that changes this is the only place it is required */

    /*
     * It will be very important that this return value not be wasted because it will leave a block marked and not used.
     */
    return block_number;
}

/*
 * Write a directory entry into a directory and handle block allocation, size changes accordingly
 */
static uint64_t diosfs_write_dirent(, struct diosfs_inode* inode,
                                    const struct diosfs_directory_entry* entry) {
    if (inode->size == DIOSFS_MAX_FILES_IN_DIRECTORY) {
        return DIOSFS_CANT_ALLOCATE_BLOCKS_FOR_DIR;
    }

    diosfs_read_inode(fs, inode, inode->inode_number);
    uint64_t block = inode->size / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK;
    uint64_t entry_in_block = (inode->size % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK);

    //allocate a new block when needed
    if ((entry_in_block == 0 && block > inode->block_count) || inode->block_count == 0) {
        inode->blocks[block] = diosfs_get_free_block_and_mark_bitmap(fs);
        inode->block_count++;
    }

    char* read_buffer = malloc(DIOSFS_BLOCKSIZE);
    struct diosfs_directory_entry* diosfs_directory_entries = (struct diosfs_directory_entry*)read_buffer;

    diosfs_read_block_by_number(inode->blocks[block], read_buffer, fs, 0,DIOSFS_BLOCKSIZE);

    memcpy(&diosfs_directory_entries[entry_in_block], entry, sizeof(struct diosfs_directory_entry));
    inode->size++;

    diosfs_write_block_by_number(inode->blocks[block], read_buffer, fs, 0,DIOSFS_BLOCKSIZE);
    diosfs_write_inode(fs, inode);

    free(read_buffer);
    return DIOSFS_SUCCESS;
}



static uint64_t diosfs_write_bytes_to_inode(, struct diosfs_inode* inode, char* buffer,
                                            const uint64_t buffer_size,
                                            const uint64_t offset,
                                            const uint64_t write_size_bytes) {
    if (inode->type != DIOSFS_REG_FILE) {
        panic("diosfs_write_bytes_to_inode bad type");
    }


    uint64_t num_blocks_to_write = write_size_bytes /DIOSFS_BLOCKSIZE;
    uint64_t start_block = offset /DIOSFS_BLOCKSIZE;
    uint64_t start_offset = offset %DIOSFS_BLOCKSIZE;
    uint64_t new_size = false;
    uint64_t new_size_bytes = 0;

    if ((start_offset + write_size_bytes) /DIOSFS_BLOCKSIZE && num_blocks_to_write == 0) {
        num_blocks_to_write++;
    }
    if (buffer_size < write_size_bytes) {
        return DIOSFS_BUFFER_TOO_SMALL;
    }

    if (offset > inode->size) {
        printf("offset %i write_size_bytes %i inode size %i\n", offset, write_size_bytes, inode->size);
        exit(1);
    }

    if (offset + write_size_bytes > inode->size) {
        new_size = true;
        new_size_bytes = (offset + write_size_bytes) - inode->size;
    }


    if (inode->size == 0) {
        inode->block_count += 1;
        inode->blocks[0] = diosfs_get_free_block_and_mark_bitmap(fs);
        diosfs_write_inode(fs, inode);
        diosfs_read_inode(fs, inode, inode->inode_number);
    }


    uint64_t current_block_number = 0;
    uint64_t end_block = start_block + num_blocks_to_write;

    if (end_block + 1 > inode->block_count) {
        diosfs_inode_allocate_new_blocks(fs, inode, (end_block + 1) - inode->block_count);
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
            byte_size =DIOSFS_BLOCKSIZE - start_offset;
        }
        else {
            byte_size = bytes_left;
        }

        current_block_number = diosfs_get_relative_block_number_from_file(inode, i, fs);

        diosfs_write_block_by_number(current_block_number, buffer, fs, start_offset, byte_size);


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

    diosfs_write_inode(fs, inode);
    return DIOSFS_SUCCESS;
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
static uint64_t diosfs_get_relative_block_number_from_file(const struct diosfs_inode* inode,
                                                           const uint64_t current_block,
                                                           ) {
    char* temp_buffer = malloc(DIOSFS_BLOCKSIZE);
    const struct diosfs_byte_offset_indices indices = diosfs_indirection_indices_for_block_number(current_block);
    uint64_t current_block_number = 0;
    uint64_t* indirection_block = (uint64_t*)temp_buffer;

    switch (indices.levels_indirection) {
    case 0:
        current_block_number = inode->blocks[indices.direct_block_number];
        goto done;

    case 1:

        diosfs_read_block_by_number(inode->single_indirect, temp_buffer, fs, 0,DIOSFS_BLOCKSIZE);
        current_block_number = indirection_block[indices.first_level_block_number];
        goto done;

    case 2:

        diosfs_read_block_by_number(inode->double_indirect, temp_buffer, fs, 0,DIOSFS_BLOCKSIZE);
        current_block_number = indirection_block[indices.second_level_block_number];
        diosfs_read_block_by_number(current_block_number, temp_buffer, fs, 0,DIOSFS_BLOCKSIZE);
        current_block_number = indirection_block[indices.first_level_block_number];
        goto done;

    case 3:

        diosfs_read_block_by_number(inode->triple_indirect, temp_buffer, fs, 0,DIOSFS_BLOCKSIZE);
        current_block_number = indirection_block[indices.third_level_block_number];
        diosfs_read_block_by_number(current_block_number, temp_buffer, fs, 0,DIOSFS_BLOCKSIZE);
        current_block_number = indirection_block[indices.second_level_block_number];
        diosfs_read_block_by_number(current_block_number, temp_buffer, fs, 0,DIOSFS_BLOCKSIZE);
        current_block_number = indirection_block[indices.first_level_block_number];
        goto done;

    default:
        printf("diosfs_get_next_logical_block_from_file: unknown indirection");
        exit(1);
    }

done:

    free(temp_buffer);
    return current_block_number;
}

uint64_t diosfs_inode_allocate_new_blocks(, struct diosfs_inode* inode,
                                          uint32_t num_blocks_to_allocate) {
    struct diosfs_byte_offset_indices result;
    char* buffer = malloc(DIOSFS_BLOCKSIZE);

    // Do not allocate blocks for a directory since they hold enough entries (90 or so at the time of writing)
    if (inode->type == DIOSFS_DIRECTORY && (num_blocks_to_allocate + (inode->size /DIOSFS_BLOCKSIZE)) >
        NUM_BLOCKS_DIRECT) {
        printf("diosfs_inode_allocate_new_block inode type not directory!\n");
        free(buffer);
        return DIOSFS_ERROR;
    }

    if (num_blocks_to_allocate + (inode->size /DIOSFS_BLOCKSIZE) > MAX_BLOCKS_IN_INODE) {
        printf("diosfs_inode_allocate_new_block too many blocks to request!\n");
        free(buffer);
        return DIOSFS_ERROR;
    }

    if (inode->size %DIOSFS_BLOCKSIZE == 0) {
        result = diosfs_indirection_indices_for_block_number(inode->size /DIOSFS_BLOCKSIZE);
    }
    else {
        result = diosfs_indirection_indices_for_block_number((inode->size /DIOSFS_BLOCKSIZE) + 1);
    }

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
        printf("diosfs_inode_allocate_new_block: unknown indirection");
        exit(1);
    }


level_zero:
    for (uint64_t i = inode->block_count; i < NUM_BLOCKS_DIRECT; i++) {
        inode->blocks[i] = diosfs_get_free_block_and_mark_bitmap(fs);
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
    diosfs_allocate_single_indirect_block(fs, inode, num_allocated, num_in_indirect,false, 0);
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
    diosfs_allocate_double_indirect_block(fs, inode, num_allocated, num_in_indirect,false, 0);
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
    diosfs_allocate_triple_indirect_block(fs, inode, num_allocated, num_in_indirect);
    inode->block_count += num_in_indirect;
    num_blocks_to_allocate -= num_in_indirect;

done:
    if (num_blocks_to_allocate != 0) {
        panic("diosfs_inode_allocate_new_block: num_blocks_to_allocate != 0!\n");
    }
    free(buffer);
    diosfs_write_inode(fs, inode);
    return DIOSFS_SUCCESS;
}


static struct diosfs_byte_offset_indices diosfs_indirection_indices_for_block_number(uint64_t block_number) {
    struct diosfs_byte_offset_indices byte_offset_indices = {0};

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


    printf("diosfs_indirection_indices_for_block_number invalid block number");
    exit(1);
}



/*
 * 4 functions beneath just take some of the ramdisk calls out in favor of local functions for read and writing blocks and inodes
 */
static void diosfs_write_inode(struct diosfs_inode* inode) {
    uint64_t inode_number_in_block = inode->inode_number % NUM_INODES_PER_BLOCK;
    uint64_t block_number = fs->superblock->inode_start_pointer + inode->inode_number / NUM_INODES_PER_BLOCK;

    uint64_t ret = ramdisk_write((char*)inode, block_number, sizeof(struct diosfs_inode) * inode_number_in_block,
                                 sizeof(struct diosfs_inode), sizeof(struct diosfs_inode), fs->device->device_id);

}

static void diosfs_read_inode(const , struct diosfs_inode* inode, uint64_t inode_number) {
    uint64_t inode_number_in_block = inode_number % NUM_INODES_PER_BLOCK;
    uint64_t block_number = fs->superblock->inode_start_pointer + (inode_number / NUM_INODES_PER_BLOCK);
    uint64_t ret = ramdisk_read((char*)inode, block_number, sizeof(struct diosfs_inode) * inode_number_in_block,
                                sizeof(struct diosfs_inode), sizeof(struct diosfs_inode), fs->device->device_id);
}

static void diosfs_write_block_by_number(const uint64_t block_number, const char* buffer,
                                         uint64_t offset, uint64_t write_size) {
    if (write_size >DIOSFS_BLOCKSIZE) {
        write_size =DIOSFS_BLOCKSIZE;
    }
    if (offset >=DIOSFS_BLOCKSIZE) {
        offset = 0;
    }

    uint64_t write_size_bytes = write_size;
    if (write_size_bytes + offset >DIOSFS_BLOCKSIZE) {
        write_size_bytes =DIOSFS_BLOCKSIZE - offset;
    }
    uint64_t ret = ramdisk_write(buffer, block_number + DIOSFS_START_BLOCKS, offset, write_size_bytes,
                                DIOSFS_BLOCKSIZE, fs->device->device_id);
}

static void diosfs_read_block_by_number(const uint64_t block_number, char* buffer,
                                        const ,
                                        uint64_t offset, uint64_t read_size) {
    if (read_size >DIOSFS_BLOCKSIZE) {
        read_size =DIOSFS_BLOCKSIZE - offset;
    }

    if (offset >=DIOSFS_BLOCKSIZE) {
        offset = 0;
    }

    uint64_t read_size_bytes = read_size;
    if (read_size_bytes + offset >DIOSFS_BLOCKSIZE) {
        read_size_bytes =DIOSFS_BLOCKSIZE - offset;
    }

    uint64_t ret = ramdisk_read(buffer, block_number + DIOSFS_START_BLOCKS, offset, read_size_bytes,
                               DIOSFS_BLOCKSIZE,
                                fs->device->device_id);

}



/*
 *  Functions beneath are self-explanatory
 *  Easy allocate functions for different levels of
 *  indirection. Num to allocate is how many in this indirect block to allocate,
 *  num_allocated is how many have already been allocated in this indirect block
 */

static uint64_t diosfs_allocate_triple_indirect_block(, struct diosfs_inode* inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate) {
    char* buffer = malloc(DIOSFS_BLOCKSIZE);
    uint64_t* block_array = (uint64_t*)buffer;
    uint64_t index;
    uint64_t allocated;
    uint64_t triple_indirect = inode->triple_indirect == 0
                                   ? diosfs_get_free_block_and_mark_bitmap(fs)
                                   : inode->triple_indirect;
    inode->triple_indirect = triple_indirect; // could be more elegant but this is fine

    diosfs_read_block_by_number(triple_indirect, buffer, fs, 0,DIOSFS_BLOCKSIZE);

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
        block_array[index++] = diosfs_allocate_double_indirect_block(fs, inode, allocated, amount,true, 0);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
    }

    diosfs_write_block_by_number(triple_indirect, buffer, fs, 0,DIOSFS_BLOCKSIZE);
    free(buffer);
    return triple_indirect;
}

static uint64_t diosfs_allocate_double_indirect_block(, struct diosfs_inode* inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number) {
    uint64_t index;
    uint64_t allocated;
    char* buffer = malloc(DIOSFS_BLOCKSIZE);
    uint64_t double_indirect;

    if (higher_order && block_number == 0) {
        double_indirect = diosfs_get_free_block_and_mark_bitmap(fs);
    }
    else if (!higher_order && block_number == 0) {
        double_indirect = inode->double_indirect == 0
                              ? diosfs_get_free_block_and_mark_bitmap(fs)
                              : inode->double_indirect;
        inode->double_indirect = double_indirect;
    }
    else {
        double_indirect = block_number;
    }


    diosfs_read_block_by_number(double_indirect, buffer, fs, 0,DIOSFS_BLOCKSIZE);
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
        block_array[index] = diosfs_allocate_single_indirect_block(fs, inode, allocated, amount,true, block);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
        index++;
        block = block_array[index];
    }
    diosfs_write_block_by_number(double_indirect, buffer, fs, 0,DIOSFS_BLOCKSIZE);
    diosfs_write_inode(fs, inode);
    free(buffer);
    return double_indirect;
}


static uint64_t diosfs_allocate_single_indirect_block(const , struct diosfs_inode* inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate,
                                                      const bool higher_order, const uint64_t block_number) {
    if (num_allocated + num_to_allocate >= NUM_BLOCKS_IN_INDIRECTION_BLOCK) {
        num_to_allocate = NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated;
    }

    char* buffer = malloc(DIOSFS_BLOCKSIZE);
    uint64_t single_indirect = block_number;

    if (higher_order && block_number == 0) {
        single_indirect = diosfs_get_free_block_and_mark_bitmap(fs);
    }
    else if (!higher_order && block_number == 0) {
        single_indirect = inode->single_indirect == 0
                              ? diosfs_get_free_block_and_mark_bitmap(fs)
                              : inode->single_indirect;
        inode->single_indirect = single_indirect;
    }
    else {
        single_indirect = block_number;
    }

    diosfs_read_block_by_number(single_indirect, buffer, fs, 0,DIOSFS_BLOCKSIZE);
    /* We can probably just write over the memory which makes these reads redundant, just a note for now */
    uint64_t* block_array = (uint64_t*)buffer;
    for (uint64_t i = num_allocated; i < num_to_allocate + num_allocated; i++) {
        block_array[i] = diosfs_get_free_block_and_mark_bitmap(fs);
    }
    diosfs_write_block_by_number(single_indirect, buffer, fs, 0,DIOSFS_BLOCKSIZE);
    diosfs_write_inode(fs, inode);
    free(buffer);
    return single_indirect;
}


