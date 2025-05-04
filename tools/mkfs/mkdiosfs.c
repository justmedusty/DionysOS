//
// Created by dustyn on 12/15/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mkdiosfs.h"
#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>

uint64_t strtoll_wrapper(const char *arg);

void write_block(const uint64_t block_number, char *block_buffer, const uint64_t offset, uint64_t write_size);

static uint64_t diosfs_allocate_single_indirect_block(struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);

static uint64_t diosfs_allocate_double_indirect_block(struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);

static uint64_t diosfs_allocate_triple_indirect_block(struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate);

static void diosfs_write_inode(struct diosfs_inode *inode);

static struct diosfs_byte_offset_indices diosfs_indirection_indices_for_block_number(uint64_t block_number);

uint64_t diosfs_inode_allocate_new_blocks(struct diosfs_inode *inode,
                                          uint32_t num_blocks_to_allocate);

static uint64_t diosfs_get_relative_block_number_from_file(const struct diosfs_inode *inode,
                                                           uint64_t current_block
);

static uint64_t diosfs_write_bytes_to_inode(struct diosfs_inode *inode, const char *buffer,
                                            uint64_t buffer_size,
                                            uint64_t offset,
                                            uint64_t write_size_bytes);

static uint64_t diosfs_write_dirent(struct diosfs_inode *inode,
                                    const struct diosfs_directory_entry *entry);

static uint64_t diosfs_get_free_block_and_mark_bitmap();

uint64_t diosfs_create(struct diosfs_inode *parent, const char *name, uint8_t inode_type);

void read_block(const uint64_t block_number, char *block_buffer, uint64_t offset, uint64_t read_size);

static void diosfs_read_inode(struct diosfs_inode *inode, uint64_t inode_number);

static void diosfs_read_block_by_number(uint64_t block_number, char *buffer,
                                        uint64_t offset, uint64_t read_size);

static void diosfs_write_block_by_number(uint64_t block_number, const char *buffer,
                                         uint64_t offset, uint64_t write_size);

static void diosfs_get_free_inode_and_mark_bitmap(struct diosfs_inode *inode_to_be_filled);

void read_inode(uint64_t inode_number, struct diosfs_inode *inode);

void write_inode(struct diosfs_inode *inode);

void diosfs_get_size_info(struct diosfs_size_calculation *size_calculation, size_t gigabytes, size_t block_size);

static uint64_t diosfs_find_directory_entry_and_update(const uint64_t inode_number,
                                                       const uint64_t directory_inode_number);
static void fill_directory(uint64_t inode_number, char* directory_path);

#define FILE_LIST_OFFSET 4

char *disk_buffer = NULL;

uint64_t block_start = 0;
uint64_t inode_start = 0;
uint64_t block_bitmap_start = 0;
uint64_t inode_bitmap_start = 0;

/*
 * mkdiosfs is the name of the executable
 *
 * Supports sizes 250 mb to 8000mb in mb increments, it will just load the entire disk in memory to make it a little simpler. Need to ensure
 * you have enough memory. My machine has more RAM than I ever use so this sort of thing is ok for me.
 *
 * It can take files as input, by default it will just have the following directories :
 *
 *  root
 *  bin
 *  etc
 *  home
 *  mnt
 *  var
 *
 *  You can add files by adding arguments in this fashion :
 *  mkdiosfs name.img size-gigs -f file_to_add other_file_to_add etc
 */
int main(const int argc, char **argv) {
    bool files = false;
    size_t num_files = 0;
    if (argc < 3) {
        printf("Usage: mkdiosfs name.img size-megs\n");
        printf("You can add files via: mkdiosfs name.img size-megs -f file_to_add other_file_to_add etc\n");
        exit(1);
    }


    if (argc > 3 && strcmp(argv[3], "--f") != 0) {
        printf("Invalid optional argument format, to add files append -f path/to/file/one path/to/file/two etc\n");
        exit(1);
    }

    if (argc > 4) {
        files = true;
        num_files = argc - FILE_LIST_OFFSET;
    }

    const uint64_t arg2 = strtoll_wrapper(argv[2]);

    if (arg2 > 8000) {
        printf("This tool does not support images above 8GB, pick a size between 250 and 8000MiB\n");
        exit(1);
    }


    FILE *f = fopen(argv[1], "w+");

    if (!f) {
        printf("Error creating file\n");
        exit(1);
    }

    uint64_t size_bytes = arg2 << 20;
    //giving a bit of extra space in the buffer, size + size / 20
    disk_buffer = malloc(size_bytes + (size_bytes / 20));

    if (!disk_buffer) {
        printf("Error allocating disk buffer\n");
        perror("malloc");
        exit(1);
    }

    char *block = malloc(DIOSFS_BLOCKSIZE);

    if (!block) {
        printf("Error allocating block\n");
        perror("malloc");
        exit(1);
    }

    memset(block, 0, DIOSFS_BLOCKSIZE);
    struct diosfs_superblock *superblock = (struct diosfs_superblock *) block;

    struct diosfs_size_calculation size_calculation = {0};

    diosfs_get_size_info(&size_calculation, arg2, DIOSFS_BLOCKSIZE);

    superblock->magic = DIOSFS_MAGIC;
    superblock->version = DIOSFS_VERSION;
    superblock->block_size = DIOSFS_BLOCKSIZE;
    superblock->num_blocks = size_calculation.total_blocks;
    superblock->num_inodes = size_calculation.total_inodes;
    superblock->inode_start_pointer = size_calculation.total_inode_bitmap_blocks + size_calculation.
            total_block_bitmap_blocks + 1; // 1 for superblock
    superblock->block_start_pointer = superblock->inode_start_pointer + (size_calculation.total_inodes /
                                                                         NUM_INODES_PER_BLOCK);
    superblock->inode_bitmap_pointers_start = 1;
    superblock->block_bitmap_pointers_start = inode_bitmap_start + size_calculation.total_inode_bitmap_blocks;
    superblock->block_bitmap_size = size_calculation.total_block_bitmap_blocks;
    superblock->inode_bitmap_size = size_calculation.total_inode_bitmap_blocks;
    superblock->total_size = size_calculation.total_blocks * superblock->block_size;

    /*
     * Set global variables so we don't need to pass the superblock around
     */
    block_bitmap_start = superblock->block_bitmap_pointers_start;
    block_start = superblock->block_start_pointer;
    inode_start = superblock->inode_start_pointer;
    inode_bitmap_start = superblock->inode_bitmap_pointers_start;

    memset(superblock->reserved, 0, sizeof(superblock->reserved));
    write_block(0, block, 0, DIOSFS_BLOCKSIZE);

    memset(block, 0, DIOSFS_BLOCKSIZE);

    //write zerod blocks across the filesystem
    for (size_t i = 0; i < size_calculation.total_inode_bitmap_blocks; i++) {
        write_block(i + inode_bitmap_start, block, 0, DIOSFS_BLOCKSIZE);
    }

    for (size_t i = 0; i < size_calculation.total_block_bitmap_blocks; i++) {
        write_block(i + block_bitmap_start, block, 0, DIOSFS_BLOCKSIZE);
    }

    for (size_t i = 0; i < ((size_calculation.total_inodes / NUM_INODES_PER_BLOCK)); i++) {
        write_block((i + inode_start), block, 0, DIOSFS_BLOCKSIZE);
    }

    for (size_t i = 0; i < (size_calculation.total_blocks + block_start); i++) {
        write_block(i + block_start, block, 0, DIOSFS_BLOCKSIZE);
    }

    /*
     * Manually create the root inode
     */
    struct diosfs_inode inode = {0};
    diosfs_get_free_inode_and_mark_bitmap(&inode);
    strcpy(inode.name, "/");
    inode.type = DIOSFS_DIRECTORY;
    inode.uid = 0;
    inode.parent_inode_number = 0;
    inode.refcount = 1;
    write_inode(&inode);
    read_inode(inode.inode_number, &inode);
    printf("Created root directory\n");

    /*
     * create default directories
     */
#define BIN_INO 1

    diosfs_create(&inode, "bin", DIOSFS_DIRECTORY);
    printf("Created bin directory\n");

#define ETC_INO 2

    diosfs_create(&inode, "etc", DIOSFS_DIRECTORY);
    printf("Created etc directory\n");

#define HOME_INO 3

    diosfs_create(&inode, "home", DIOSFS_DIRECTORY);
    printf("Created home directory\n");

#define ROOT_INO 4

    diosfs_create(&inode, "root", DIOSFS_DIRECTORY);
    printf("Created root directory\n");

#define MNT_INO 5

    diosfs_create(&inode, "mnt", DIOSFS_DIRECTORY);
    printf("Created mnt directory\n");

#define VAR_INO 6

    diosfs_create(&inode, "var", DIOSFS_DIRECTORY);
    printf("Created var directory\n");

#define TEMP_INO 7

    diosfs_create(&inode, "temp", DIOSFS_DIRECTORY);
    printf("Created temp directory\n");

    //include any passed files in, they automatically end up in the home directory
    if (files) {
        struct diosfs_inode home_inode;
        read_inode(HOME_INO, &home_inode);
        printf("Going into %s directory\n", home_inode.name);

        for (size_t i = 0; i < num_files; i++) {
            struct diosfs_inode new_inode;
            FILE *file1 = fopen(argv[i + FILE_LIST_OFFSET], "r");

            if (file1 == NULL) {
                printf("Error opening file %s. Image on disk will not include any of the extra files.\n",
                       argv[i + FILE_LIST_OFFSET]);
                perror("fopen");
                exit(1);
            }

            const char *filename = strchr(argv[i + FILE_LIST_OFFSET], '/');

            if (filename) {
                filename++;
            } else {
                filename = argv[i + FILE_LIST_OFFSET];
            }

            fseek(file1, 0L, SEEK_END);
            uint64_t size = ftell(file1);
            rewind(file1);
            size_t bytes_read = 0;
            char *file_buffer = malloc(size + 1);
            bytes_read = fread(file_buffer, 1, size, file1);
            if (bytes_read != size) {
            }
            printf("Creating file %s in the home directory...\n", filename);
            uint64_t inode_number = diosfs_create(&home_inode, filename, DIOSFS_REG_FILE);
            read_inode(inode_number, &new_inode);
            diosfs_write_bytes_to_inode(&new_inode, file_buffer, size, 0, size);
            printf("Created file %s\n", filename);
        }
    }

    fill_directory(BIN_INO,"./default_files/bin");
    fill_directory(ETC_INO,"./default_files/etc");
    fill_directory(HOME_INO,"./default_files/home");
    fill_directory(VAR_INO,"./default_files/var");


    fwrite(disk_buffer, size_calculation.total_blocks * DIOSFS_BLOCKSIZE, 1, f);
    free(block);
    free(disk_buffer);
    printf("Successfully created diosfs image %s\n", argv[1]);
    exit(0);
}

void diosfs_get_size_info(struct diosfs_size_calculation *size_calculation, const size_t megabytes,
                          const size_t block_size) {
    if (megabytes == 0) {
        exit(1);
    }

    const uint64_t size_bytes = megabytes << 20;

    size_calculation->total_blocks = (size_bytes / block_size);
    uint64_t padding = size_calculation->total_blocks / 150;
    const uint64_t split = (size_calculation->total_blocks / 32) - padding;


    size_calculation->total_inodes = split * (block_size / sizeof(struct diosfs_inode));
    size_calculation->total_data_blocks = (size_calculation->total_blocks - split) - padding;
    size_calculation->total_block_bitmap_blocks = (size_calculation->total_data_blocks / block_size / 8);
    size_calculation->total_inode_bitmap_blocks = size_calculation->total_inodes / block_size / 8;
    printf("Total_inodes :%lu, Total data blocks :%lu, Total block bitmap blocks :%lu, Total inode bitmap blocks :%lu\n",
           size_calculation->total_inodes, size_calculation->total_data_blocks,
           size_calculation->total_block_bitmap_blocks, size_calculation->total_inode_bitmap_blocks);
}

uint64_t strtoll_wrapper(const char *arg) {
    char *end_ptr;
    uint64_t result = strtoull(arg, &end_ptr, 10);
    if (*end_ptr != '\0') {
        printf("Error converting string\n");
        exit(1);
    }

    return result;
}
static void fill_directory(uint64_t inode_number, char* directory_path) {
    struct diosfs_inode inode;
    read_inode(inode_number, &inode);
    printf("Going into %s directory\n", inode.name);

    struct dirent **namelist;
    int n = scandir(directory_path, &namelist, NULL, alphasort);
    if (n < 0) {
        perror("scandir");
        return;
    }

    for (int i = 0; i < n; i++) {
        struct dirent *entry = namelist[i];

        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        // Build full path to the file
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory_path, entry->d_name);

        FILE *file = fopen(filepath, "r");
        if (file == NULL) {
            fprintf(stderr, "Error opening file %s. Skipping.\n", filepath);
            perror("fopen");
            free(entry);
            continue;
        }

        // Read file contents
        fseek(file, 0L, SEEK_END);
        long size = ftell(file);
        rewind(file);

        char *file_buffer = malloc(size + 1);
        if (!file_buffer) {
            fprintf(stderr, "Memory allocation failed for file %s\n", filepath);
            fclose(file);
            free(entry);
            continue;
        }

        size_t bytes_read = fread(file_buffer, 1, size, file);
        fclose(file);

        if (bytes_read != size) {
            fprintf(stderr, "Warning: expected %ld bytes but read %zu for file %s\n", size, bytes_read, filepath);
        }

        // Create file in filesystem
        printf("Creating file %s in the %s directory...\n", entry->d_name,inode.name);
        uint64_t new_inode_num = diosfs_create(&inode, entry->d_name, DIOSFS_REG_FILE);

        struct diosfs_inode new_inode;
        read_inode(new_inode_num, &new_inode);
        printf("%s BYTES READ %lu\n",file_buffer,bytes_read);
        diosfs_write_bytes_to_inode(&new_inode, file_buffer, bytes_read, 0, bytes_read);
        printf("Created file %s\n", entry->d_name);

        free(file_buffer);
        free(entry);
    }

    free(namelist);
}

void write_block(const uint64_t block_number, char *block_buffer, const uint64_t offset, uint64_t write_size) {
    memcpy(disk_buffer + (block_number * DIOSFS_BLOCKSIZE) + offset, block_buffer, write_size);

}

void read_block(const uint64_t block_number, char *block_buffer, uint64_t offset, uint64_t read_size) {
    memcpy(block_buffer, disk_buffer + (block_number * DIOSFS_BLOCKSIZE) + offset, read_size);
}

void read_inode(uint64_t inode_number, struct diosfs_inode *inode) {
    memcpy(inode, &disk_buffer[INODE_TO_BYTE_OFFSET(inode_number)], sizeof(struct diosfs_inode));
}


void write_inode(struct diosfs_inode *inode) {
    memcpy(&disk_buffer[INODE_TO_BYTE_OFFSET(inode->inode_number)], inode, sizeof(struct diosfs_inode));
}

uint64_t diosfs_create(struct diosfs_inode *parent, const char *name, const uint8_t inode_type) {
    struct diosfs_inode inode;

    diosfs_get_free_inode_and_mark_bitmap(&inode);
    diosfs_read_inode(parent, parent->inode_number);
    parent->size++;
    inode.type = inode_type;
    inode.block_count = 0;
    inode.parent_inode_number = parent->inode_number;
    strcpy((char *) &inode.name, name);
    inode.uid = 0;
    diosfs_write_inode(&inode);

    struct diosfs_directory_entry entry = {0};
    entry.inode_number = inode.inode_number;
    entry.size = 0;
    strcpy(entry.name, name);
    entry.parent_inode_number = inode.parent_inode_number;
    entry.type = inode.type;
    uint64_t ret = diosfs_write_dirent(parent, &entry);
    diosfs_write_inode(parent);
    return inode.inode_number;

}


static uint64_t diosfs_get_free_block_and_mark_bitmap() {
    uint64_t buffer_size = DIOSFS_BLOCKSIZE * 128;
    char *buffer = disk_buffer + (DIOSFS_BLOCKSIZE * block_bitmap_start);
    uint64_t block = 0;
    uint64_t byte = 0;
    uint64_t bit = 0;
    uint64_t offset = 0;
    uint64_t block_number = 0;

    retry:

    while (1) {
        if (buffer[block * DIOSFS_BLOCKSIZE + byte] != 0xFF) {
            for (uint64_t i = 0; i <= 8; i++) {
                if (!(buffer[(block * DIOSFS_BLOCKSIZE) + byte] & (1 << i))) {
                    bit = i;
                    buffer[(block * DIOSFS_BLOCKSIZE) + byte] |= (1 << bit);
                    block_number = (block * DIOSFS_BLOCKSIZE * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte == DIOSFS_BLOCKSIZE) {
                block++;
                byte = 0;
            }
            if (block > DIOSFS_NUM_BLOCK_POINTER_BLOCKS) {
                printf("diosfs_get_free_inode_and_mark_bitmap: No free inodes");
                exit(1);
            }
        } else {
            byte++;
        }
    }


    found_free:

    return block_number;
}

static void diosfs_get_free_inode_and_mark_bitmap(struct diosfs_inode *inode_to_be_filled) {
    uint64_t buffer_size = DIOSFS_BLOCKSIZE * 16;
    char *buffer = disk_buffer + (DIOSFS_BLOCKSIZE * inode_bitmap_start);
    uint64_t block = 0;
    uint64_t byte = 0;
    uint64_t bit = 0;
    uint64_t inode_number;

    while (1) {
        if (buffer[block * DIOSFS_BLOCKSIZE + byte] != 0xFF) {
            for (uint64_t i = 0; i <= 8; i++) {
                if (!(buffer[(block * DIOSFS_BLOCKSIZE) + byte] & (1 << i))) {
                    bit = i;
                    buffer[byte] |= (1 << bit);
                    inode_number = (block * DIOSFS_BLOCKSIZE * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte == DIOSFS_BLOCKSIZE) {
                block++;
                byte = 0;
            }
            if (block > DIOSFS_NUM_INODE_POINTER_BLOCKS) {
                printf("diosfs_get_free_inode_and_mark_bitmap: No free inodes");
                exit(1);
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        } else {
            byte++;
        }

        if (byte == DIOSFS_BLOCKSIZE) {
            block++;
            byte = 0;
        }
    }


    found_free:
    memset(inode_to_be_filled, 0, sizeof(struct diosfs_inode));
    inode_to_be_filled->inode_number = inode_number;
}

/*
 * Write a directory entry into a directory and handle block allocation, size changes accordingly
 */
static uint64_t diosfs_write_dirent(struct diosfs_inode *inode,
                                    const struct diosfs_directory_entry *entry) {
    if (inode->size == DIOSFS_MAX_FILES_IN_DIRECTORY) {
        return DIOSFS_CANT_ALLOCATE_BLOCKS_FOR_DIR;
    }

    diosfs_read_inode(inode, inode->inode_number);
    uint64_t block = inode->size / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK;
    uint64_t entry_in_block = (inode->size % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK);

    //allocate a new block when needed
    if ((entry_in_block == 0 && block > inode->block_count) || inode->block_count == 0) {
        inode->blocks[block] = diosfs_get_free_block_and_mark_bitmap();
        inode->block_count++;
    }

    char *read_buffer = malloc(DIOSFS_BLOCKSIZE);
    struct diosfs_directory_entry *diosfs_directory_entries = (struct diosfs_directory_entry *) read_buffer;

    diosfs_read_block_by_number(inode->blocks[block], read_buffer, 0, DIOSFS_BLOCKSIZE);

    memcpy(&diosfs_directory_entries[entry_in_block], entry, sizeof(struct diosfs_directory_entry));
    inode->size++;

    diosfs_write_block_by_number(inode->blocks[block], read_buffer, 0, DIOSFS_BLOCKSIZE);
    diosfs_write_inode(inode);

    free(read_buffer);
    return DIOSFS_SUCCESS;
}


static uint64_t diosfs_write_bytes_to_inode(struct diosfs_inode *inode, const char *buffer,
                                            const uint64_t buffer_size,
                                            const uint64_t offset,
                                            const uint64_t write_size_bytes) {
    if (inode->type != DIOSFS_REG_FILE) {
        printf("diosfs_write_bytes_to_inode bad type");
        exit(1);
    }


    uint64_t num_blocks_to_write = write_size_bytes / DIOSFS_BLOCKSIZE;
    uint64_t start_block = offset / DIOSFS_BLOCKSIZE;
    uint64_t start_offset = offset % DIOSFS_BLOCKSIZE;
    uint64_t new_size = false;
    uint64_t new_size_bytes = 0;

    if ((start_offset + write_size_bytes) / DIOSFS_BLOCKSIZE && num_blocks_to_write == 0) {
        num_blocks_to_write++;
    }
    if (buffer_size < write_size_bytes) {
        return DIOSFS_BUFFER_TOO_SMALL;
    }

    if (offset > inode->size) {
        printf("offset %lu write_size_bytes %lu inode size %i\n", offset, write_size_bytes, inode->size);
        exit(1);
    }

    if (offset + write_size_bytes > inode->size) {
        new_size = true;
        new_size_bytes = (offset + write_size_bytes) - inode->size;
    }


    if (inode->size == 0) {
        inode->block_count += 1;
        inode->blocks[0] = diosfs_get_free_block_and_mark_bitmap();
        diosfs_write_inode(inode);
        diosfs_read_inode(inode, inode->inode_number);
    }


    uint64_t current_block_number = 0;
    uint64_t end_block = start_block + num_blocks_to_write;

    if (end_block + 1 > inode->block_count) {
        diosfs_inode_allocate_new_blocks(inode, (end_block + 1) - inode->block_count);
        inode->block_count += ((end_block + 1) - inode->block_count);
    }

    /*
     *  If 0 is passed, try to write everything
     */

    uint64_t bytes_written = 0;
    uint64_t bytes_left = write_size_bytes;

    for (uint64_t i = start_block; i < end_block; i++) {
        uint64_t byte_size;

        if (DIOSFS_BLOCKSIZE - start_offset < bytes_left) {
            byte_size = DIOSFS_BLOCKSIZE - start_offset;
        } else {
            byte_size = bytes_left;
        }
        current_block_number = diosfs_get_relative_block_number_from_file(inode, i);
        diosfs_write_block_by_number(current_block_number, buffer, start_offset, byte_size);
        bytes_written += byte_size;
        bytes_left -= byte_size;
        buffer += byte_size;

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

    diosfs_write_inode(inode);
    diosfs_find_directory_entry_and_update(inode->inode_number,inode->parent_inode_number);
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
static uint64_t diosfs_get_relative_block_number_from_file(const struct diosfs_inode *inode,
                                                           const uint64_t current_block
) {
    char *temp_buffer = malloc(DIOSFS_BLOCKSIZE);
    const struct diosfs_byte_offset_indices indices = diosfs_indirection_indices_for_block_number(current_block);
    uint64_t current_block_number = 0;
    uint64_t *indirection_block = (uint64_t *) temp_buffer;

    switch (indices.levels_indirection) {
        case 0:
            current_block_number = inode->blocks[indices.direct_block_number];
            goto done;

        case 1:

            diosfs_read_block_by_number(inode->single_indirect, temp_buffer, 0, DIOSFS_BLOCKSIZE);
            current_block_number = indirection_block[indices.first_level_block_number];
            goto done;

        case 2:

            diosfs_read_block_by_number(inode->double_indirect, temp_buffer, 0, DIOSFS_BLOCKSIZE);
            current_block_number = indirection_block[indices.second_level_block_number];
            diosfs_read_block_by_number(current_block_number, temp_buffer, 0, DIOSFS_BLOCKSIZE);
            current_block_number = indirection_block[indices.first_level_block_number];
            goto done;

        case 3:

            diosfs_read_block_by_number(inode->triple_indirect, temp_buffer, 0, DIOSFS_BLOCKSIZE);
            current_block_number = indirection_block[indices.third_level_block_number];
            diosfs_read_block_by_number(current_block_number, temp_buffer, 0, DIOSFS_BLOCKSIZE);
            current_block_number = indirection_block[indices.second_level_block_number];
            diosfs_read_block_by_number(current_block_number, temp_buffer, 0, DIOSFS_BLOCKSIZE);
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

uint64_t diosfs_inode_allocate_new_blocks(struct diosfs_inode *inode,
                                          uint32_t num_blocks_to_allocate) {
    struct diosfs_byte_offset_indices result;
    char *buffer = malloc(DIOSFS_BLOCKSIZE);

    // Do not allocate blocks for a directory since they hold enough entries (90 or so at the time of writing)
    if (inode->type == DIOSFS_DIRECTORY && (num_blocks_to_allocate + (inode->size / DIOSFS_BLOCKSIZE)) >
                                           NUM_BLOCKS_DIRECT) {
        printf("diosfs_inode_allocate_new_block inode type not directory!\n");
        free(buffer);
        return DIOSFS_ERROR;
    }

    if (num_blocks_to_allocate + (inode->size / DIOSFS_BLOCKSIZE) > MAX_BLOCKS_IN_INODE) {
        printf("diosfs_inode_allocate_new_block too many blocks to request!\n");
        free(buffer);
        return DIOSFS_ERROR;
    }

    if (inode->size % DIOSFS_BLOCKSIZE == 0) {
        result = diosfs_indirection_indices_for_block_number(inode->size / DIOSFS_BLOCKSIZE);
    } else {
        result = diosfs_indirection_indices_for_block_number((inode->size / DIOSFS_BLOCKSIZE) + 1);
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
        inode->blocks[i] = diosfs_get_free_block_and_mark_bitmap();
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
    diosfs_allocate_single_indirect_block(inode, num_allocated, num_in_indirect, false, 0);
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
    diosfs_allocate_double_indirect_block(inode, num_allocated, num_in_indirect, false, 0);
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
    diosfs_allocate_triple_indirect_block(inode, num_allocated, num_in_indirect);
    inode->block_count += num_in_indirect;
    num_blocks_to_allocate -= num_in_indirect;

    done:
    if (num_blocks_to_allocate != 0) {
        printf("diosfs_inode_allocate_new_block: num_blocks_to_allocate != 0!\n");
        exit(1);
    }
    free(buffer);
    diosfs_write_inode(inode);
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
                                                                          NUM_BLOCKS_DOUBLE_INDIRECTION))) /
                                                        NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        byte_offset_indices.first_level_block_number = block_number % (NUM_BLOCKS_IN_INDIRECTION_BLOCK);
        return byte_offset_indices;
    }


    printf("diosfs_indirection_indices_for_block_number invalid block number");
    exit(1);
}


/*
 * 4 functions beneath just take some of the ramdisk calls out in favor of local functions for read and writing blocks and inodes
 */
static void diosfs_write_inode(struct diosfs_inode *inode) {
    write_inode(inode);
}

static void diosfs_read_inode(struct diosfs_inode *inode, uint64_t inode_number) {
    read_inode(inode_number, inode);
}

static void diosfs_write_block_by_number(const uint64_t block_number, const char *buffer,
                                         uint64_t offset, uint64_t write_size) {
    write_block(block_number + block_start, buffer, offset, write_size);
}

static void diosfs_read_block_by_number(const uint64_t block_number, char *buffer,
                                        uint64_t offset, uint64_t read_size) {
    read_block(block_number + block_start, buffer, offset, DIOSFS_BLOCKSIZE - offset);
}

static uint64_t diosfs_find_directory_entry_and_update(const uint64_t inode_number,
                                                       const uint64_t directory_inode_number) {
    uint64_t buffer_size = DIOSFS_BLOCKSIZE;
    char *buffer = malloc(buffer_size);
    struct diosfs_directory_entry *directory_entries = (struct diosfs_directory_entry *) buffer;
    struct diosfs_directory_entry *directory_entry;
    struct diosfs_inode inode;
    struct diosfs_inode entry;
    uint64_t directory_entries_read = 0;
    uint64_t directory_block = 0;
    uint64_t block_number = 0;

    diosfs_read_inode(&entry, inode_number);
    diosfs_read_inode(&inode, directory_inode_number);
    if (inode.type != DIOSFS_DIRECTORY) {
        free(buffer);
        return DIOSFS_NOT_A_DIRECTORY;
    }


    while (1) {
        block_number = inode.blocks[directory_block++];
        /*Should be okay to leave this unrestrained since we check children size and inode size */

        diosfs_read_block_by_number(block_number, buffer, 0, DIOSFS_BLOCKSIZE);

        for (uint64_t i = 0; i < (DIOSFS_BLOCKSIZE / sizeof(struct diosfs_directory_entry)); i++) {
            if (directory_entries_read == inode.size) {
                goto not_found;
            }
            directory_entry = &directory_entries[i];
            if (directory_entry->inode_number == inode_number) {
                goto done;
            }
        }
    }
    done:
    strcpy(directory_entry->name, entry.name);
    directory_entry->size = entry.size;
    directory_entry->parent_inode_number = entry.parent_inode_number;
    diosfs_write_block_by_number(block_number, buffer, 0, DIOSFS_BLOCKSIZE);
    return DIOSFS_SUCCESS;
    not_found:
    free(buffer);
    return DIOSFS_SUCCESS;
}

/*
 *  Functions beneath are self-explanatory
 *  Easy allocate functions for different levels of
 *  indirection. Num to allocate is how many in this indirect block to allocate,
 *  num_allocated is how many have already been allocated in this indirect block
 */

static uint64_t diosfs_allocate_triple_indirect_block(struct diosfs_inode *inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate) {
    char *buffer = malloc(DIOSFS_BLOCKSIZE);
    uint64_t *block_array = (uint64_t *) buffer;
    uint64_t index;
    uint64_t allocated;
    uint64_t triple_indirect = inode->triple_indirect == 0
                               ? diosfs_get_free_block_and_mark_bitmap()
                               : inode->triple_indirect;
    inode->triple_indirect = triple_indirect; // could be more elegant but this is fine

    diosfs_read_block_by_number(triple_indirect, buffer, 0, DIOSFS_BLOCKSIZE);

    if (num_allocated == 0) {
        index = 0;
        allocated = 0;
    } else {
        index = num_allocated / NUM_BLOCKS_DOUBLE_INDIRECTION;
        allocated = num_allocated % NUM_BLOCKS_DOUBLE_INDIRECTION;
    }


    while (num_to_allocate > 0) {
        uint64_t amount = num_to_allocate < NUM_BLOCKS_DOUBLE_INDIRECTION
                          ? num_to_allocate
                          : NUM_BLOCKS_DOUBLE_INDIRECTION;
        block_array[index++] = diosfs_allocate_double_indirect_block(inode, allocated, amount, true, 0);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
    }

    diosfs_write_block_by_number(triple_indirect, buffer, 0, DIOSFS_BLOCKSIZE);
    free(buffer);
    return triple_indirect;
}

static uint64_t diosfs_allocate_double_indirect_block(struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number) {
    uint64_t index;
    uint64_t allocated;
    char *buffer = malloc(DIOSFS_BLOCKSIZE);

    uint64_t double_indirect = diosfs_get_free_block_and_mark_bitmap();

    diosfs_read_block_by_number(double_indirect, buffer, 0, DIOSFS_BLOCKSIZE);
    uint64_t *block_array = (uint64_t *) buffer;

    if (num_allocated == 0) {
        index = 0;
        allocated = 0;
    } else {
        index = num_allocated / NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        allocated = num_allocated % NUM_BLOCKS_IN_INDIRECTION_BLOCK;
    }


    uint64_t block;

    if (num_allocated > 0) {
        block = block_array[index];
    } else {
        block = 0;
    }

    while (num_to_allocate > 0) {
        uint64_t amount = num_to_allocate < NUM_BLOCKS_IN_INDIRECTION_BLOCK
                          ? num_to_allocate
                          : NUM_BLOCKS_IN_INDIRECTION_BLOCK;
        block_array[index] = diosfs_allocate_single_indirect_block(inode, allocated, amount, true, block);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
        index++;
        block = block_array[index];
    }
    diosfs_write_block_by_number(double_indirect, buffer, 0, DIOSFS_BLOCKSIZE);
    diosfs_write_inode(inode);
    free(buffer);
    return double_indirect;
}


static uint64_t diosfs_allocate_single_indirect_block(struct diosfs_inode *inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate,
                                                      const bool higher_order, const uint64_t block_number) {
    if (num_allocated + num_to_allocate >= NUM_BLOCKS_IN_INDIRECTION_BLOCK) {
        num_to_allocate = NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated;
    }

    char *buffer = malloc(DIOSFS_BLOCKSIZE);
    uint64_t single_indirect = block_number;

    if (inode->single_indirect == 0) {
        inode->single_indirect = diosfs_get_free_block_and_mark_bitmap();
        single_indirect = inode->single_indirect;
    }

    diosfs_read_block_by_number(single_indirect, buffer, 0, DIOSFS_BLOCKSIZE);
    /* We can probably just write over the memory which makes these reads redundant, just a note for now */
    uint64_t *block_array = (uint64_t *) buffer;
    for (uint64_t i = num_allocated; i < num_to_allocate + num_allocated; i++) {
        block_array[i] = diosfs_get_free_block_and_mark_bitmap();
    }
    diosfs_write_block_by_number(single_indirect, buffer, 0, DIOSFS_BLOCKSIZE);
    diosfs_write_inode(inode);
    free(buffer);
    return single_indirect;
}


