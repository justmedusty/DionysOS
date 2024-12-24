//
// Created by dustyn on 9/18/24.
//
#include "include/device/device.h"
#include "include/filesystem/diosfs.h"
#include "include/filesystem/vfs.h"
#include <include/definitions/string.h>
#include "include/data_structures/spinlock.h"
#include "include/definitions.h"
#include <include/drivers/serial/uart.h>
#include <include/memory/kalloc.h>
#include "include/device/block/ramdisk.h"
#include "include/memory/mem.h"
/*
 * We need to ensure that for each filesystem we have a separate lock
 * We'll only implement the one filesystem for now.
 */
#define NUM_FILESYSTEM_OBJECTS 10
struct spinlock diosfs_lock[10] = {0};
struct diosfs_superblock diosfs_superblock[10] = {0};

/*
 * Hardcode the main device in
 */
struct device block_dev[10] = {
    [0] = {
        .parent = NULL,
        .lock = &diosfs_lock[0],
        .device_major = DEVICE_MAJOR_RAMDISK,
        .device_minor = INITIAL_FILESYSTEM,
        .uses_dma = false,
        .device_type = DEVICE_TYPE_BLOCK,
        .pci_driver = NULL,
        .device_info = &ramdisk[0],
        .device_ops = &ramdisk_device_ops
    }
};


//Set up initial  filesystem object
struct diosfs_filesystem_context diosfs_filesystem_context[10] = {
    [0] = {
        .filesystem_id = INITIAL_FILESYSTEM,
        .lock = &diosfs_lock[0],
        .superblock = &diosfs_superblock[0],
        .device = &block_dev[0]
    }
};

/*
 * Internally linked function prototypes
 */
static void diosfs_clear_bitmap(const struct diosfs_filesystem_context *fs, uint8_t type, uint64_t number);

static void diosfs_get_free_inode_and_mark_bitmap(const struct diosfs_filesystem_context *fs,
                                                  struct diosfs_inode *inode_to_be_filled);

static uint64_t diosfs_get_free_block_and_mark_bitmap(const struct diosfs_filesystem_context *fs);

static uint64_t diosfs_free_block_and_mark_bitmap(struct diosfs_filesystem_context *fs, uint64_t block_number);

static uint64_t diosfs_free_inode_and_mark_bitmap(struct diosfs_filesystem_context *fs, uint64_t inode_number);

static uint64_t diosfs_get_directory_entries(struct diosfs_filesystem_context *fs,
                                             struct diosfs_directory_entry *children,
                                             uint64_t inode_number, uint64_t children_size);

static struct vnode *diosfs_directory_entry_to_vnode(struct vnode *parent, struct diosfs_directory_entry *entry,
                                                     struct diosfs_filesystem_context *fs);

static struct diosfs_byte_offset_indices diosfs_indirection_indices_for_block_number(uint64_t block_number);

static void diosfs_write_inode(const struct diosfs_filesystem_context *fs, struct diosfs_inode *inode);

static void diosfs_write_block_by_number(uint64_t block_number, const char *buffer,
                                         const struct diosfs_filesystem_context *fs,
                                         uint64_t offset, uint64_t write_size);

static void diosfs_read_block_by_number(uint64_t block_number, char *buffer, const struct diosfs_filesystem_context *fs,
                                        uint64_t offset, uint64_t read_size);

static uint64_t diosfs_allocate_single_indirect_block(const struct diosfs_filesystem_context *fs,
                                                      struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);

static uint64_t diosfs_allocate_double_indirect_block(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number);

static uint64_t diosfs_allocate_triple_indirect_block(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate);

static void diosfs_read_inode(const struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                              uint64_t inode_number);

static uint64_t diosfs_get_relative_block_number_from_file(const struct diosfs_inode *inode, uint64_t current_block,
                                                           struct diosfs_filesystem_context *fs);

static void free_blocks_from_inode(struct diosfs_filesystem_context *fs,
                                   uint64_t block_start, uint64_t num_blocks,
                                   struct diosfs_inode *inode);

static uint64_t diosfs_read_bytes_from_inode(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                             char *buffer,
                                             uint64_t buffer_size,
                                             uint64_t offset, uint64_t read_size_bytes);

static uint64_t diosfs_write_bytes_to_inode(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                            char *buffer,
                                            uint64_t buffer_size,
                                            uint64_t offset,
                                            uint64_t write_size_bytes);

static void diosfs_remove_file(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode);

static void diosfs_recursive_directory_entry_free(struct diosfs_filesystem_context *fs,
                                                  const struct diosfs_directory_entry *entry, uint64_t inode_number);

static uint64_t diosfs_directory_entry_free(struct diosfs_filesystem_context *fs, struct diosfs_directory_entry *entry,
                                            const struct diosfs_inode *inode);

uint64_t diosfs_inode_allocate_new_blocks(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                          uint32_t num_blocks_to_allocate);

static uint64_t diosfs_write_dirent(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                    const struct diosfs_directory_entry *entry);

static uint64_t diosfs_symlink_check(struct diosfs_filesystem_context *fs, struct vnode *vnode, int32_t expected_type);

static struct vnode *diosfs_follow_link(struct diosfs_filesystem_context *fs, const struct vnode *vnode);

static void shift_directory_entry(const struct diosfs_filesystem_context *fs, uint64_t entry_number,
                                  struct diosfs_inode *parent_inode);

static uint64_t diosfs_find_directory_entry_and_update(struct diosfs_filesystem_context *fs,
                                                       const uint64_t inode_number,
                                                       const uint64_t directory_inode_number);

/*
 * VFS pointer functions for vnodes
 */
struct vnode_operations diosfs_vnode_ops = {
    .lookup = diosfs_lookup,
    .create = diosfs_create,
    .read = diosfs_read,
    .write = diosfs_write,
    .open = diosfs_open,
    .close = diosfs_close,
    .remove = diosfs_remove,
    .rename = diosfs_rename,
    .link = diosfs_link,
    .unlink = diosfs_unlink,
};

/*
 *
 * DIOSFS is my hand-rolled simple filesystem, it does not have any journaling and has similarities to ext2 albeit being simpler.
 *
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
 * This function will do basic setup such as initing the diosfs lock, it will then setup the ramdisk driver and begin to fill ramdisk memory with a diosfs filesystem.
 * It will use the size DEFAULT_DIOSFS_SIZE and any other size, modifications need to be made to the superblock object.
 */
void diosfs_init(uint64_t filesystem_id) {
    if (filesystem_id >= NUM_FILESYSTEM_OBJECTS) {
        return;
    }
    initlock(diosfs_filesystem_context[filesystem_id].lock, DIOSFS_LOCK);
    ramdisk_init(DEFAULT_DIOSFS_SIZE, diosfs_filesystem_context[filesystem_id].device->device_minor, "initramfs",
                 DIOSFS_BLOCKSIZE);
    dios_mkfs(filesystem_id, diosfs_filesystem_context->device->device_type, &diosfs_filesystem_context[filesystem_id]);
};


void diosfs_get_size_info(struct diosfs_size_calculation *size_calculation, size_t gigabytes, size_t block_size) {
    if (gigabytes == 0) {
        return;
    }

    uint64_t size_bytes = gigabytes << 30;

    size_calculation->total_blocks = size_bytes / block_size;

    uint64_t split = size_calculation->total_blocks / 32;

    size_calculation->total_inodes = split * (block_size / sizeof(struct diosfs_inode));
    size_calculation->total_data_blocks = size_calculation->total_blocks - split;
    size_calculation->total_block_bitmap_blocks = size_calculation->total_blocks / block_size / 8;
    size_calculation->total_inode_bitmap_blocks = size_calculation->total_inodes / 8 / block_size;

    size_calculation->total_blocks += 1; // superblock;
}

/*
 * This will be the function that spins up an empty diosfs ram filesystem and then fill with a root directory and a few basic directories and files.
 *
 * Takes a ramdisk ID to specify which ramdisk to operate on
 */
void dios_mkfs(const uint64_t device_id, const uint64_t device_type, struct diosfs_filesystem_context *fs) {
    char *buffer = kmalloc(PAGE_SIZE);
    fs->superblock->magic = DIOSFS_MAGIC;
    fs->superblock->version = DIOSFS_VERSION;
    fs->superblock->block_size = DIOSFS_BLOCKSIZE;
    fs->superblock->num_blocks = DIOSFS_NUM_BLOCKS;
    fs->superblock->num_inodes = DIOSFS_NUM_INODES;
    fs->superblock->inode_start_pointer = DIOSFS_START_INODES;
    fs->superblock->block_start_pointer = DIOSFS_START_BLOCKS;
    fs->superblock->inode_bitmap_pointers_start = DIOSFS_START_INODE_BITMAP;
    fs->superblock->block_bitmap_pointers_start = DIOSFS_START_BLOCK_BITMAP;
    fs->superblock->block_bitmap_size = DIOSFS_NUM_BLOCK_POINTER_BLOCKS;
    fs->superblock->inode_bitmap_size = DIOSFS_NUM_INODE_POINTER_BLOCKS;

    fs->superblock->total_size = DEFAULT_DIOSFS_SIZE;
    fs->device->device_major = device_type;
    fs->device->device_minor = device_id;

    //copy the contents into our buffer
    memcpy(buffer, &diosfs_superblock, sizeof(struct diosfs_superblock));

    //Write the new superblock to the ramdisk
    fs->device->device_ops->block_device_ops->block_write(DIOSFS_SUPERBLOCK, fs->superblock->block_size, buffer,
                                                          fs->device);

    memset(buffer, 0, PAGE_SIZE);

    /*
     * Fill in inode bitmap with zeros blocks
     */
    for (size_t i = 0; i < fs->superblock->inode_bitmap_size; i++) {
        fs->device->device_ops->block_device_ops->block_write(
            (fs->superblock->inode_bitmap_pointers_start * fs->superblock->block_size) +
            (fs->superblock->block_size * i), fs->superblock->block_size, buffer, fs->device);
    }

    /*
     * Fill in block bitmap with zerod blocks
     */
    for (size_t i = 0; i < fs->superblock->block_bitmap_pointers_start; i++) {
        fs->device->device_ops->block_device_ops->block_write(
            (fs->superblock->block_bitmap_pointers_start * fs->superblock->block_size) +
            (fs->superblock->block_size * i), fs->superblock->block_size, buffer, fs->device);
    }

    /*
     *Write zerod blocks for the inode blocks
     */
    for (size_t i = 0;
         i < fs->superblock->num_inodes / (fs->superblock->block_size / sizeof(struct diosfs_inode)); i++) {
        fs->device->device_ops->block_device_ops->block_write(
            (fs->superblock->inode_start_pointer * fs->superblock->block_size) + (fs->superblock->block_size * i),
            fs->superblock->block_size, buffer, fs->device);
    }

    /*
     *Write zerod blocks for the blocks
     */
    for (size_t i = 0; i < fs->superblock->num_blocks; i++) {
        fs->device->device_ops->block_device_ops->block_write(
            (fs->superblock->block_start_pointer * fs->superblock->block_size) + (fs->superblock->block_size * i),
            fs->superblock->block_size, buffer, fs->device);
    }

    struct diosfs_inode root;
    diosfs_get_free_inode_and_mark_bitmap(fs, &root);


    root.type = DIOSFS_DIRECTORY;
    strcpy((char *) &root.name, "/");
    root.parent_inode_number = root.inode_number;
    diosfs_write_inode(fs, &root);

    memset(&vfs_root, 0, sizeof(struct vnode));
    strcpy((char *) vfs_root.vnode_name, "/");
    vfs_root.filesystem_object = fs;
    vfs_root.vnode_inode_number = root.inode_number;
    vfs_root.vnode_type = DIOSFS_DIRECTORY;
    vfs_root.vnode_ops = &diosfs_vnode_ops;
    vfs_root.vnode_refcount = 1;
    vfs_root.vnode_parent = NULL;
    vfs_root.vnode_children = NULL;
    vfs_root.vnode_filesystem_id = VNODE_FS_DIOSFS;

    struct vnode *vnode1 = vnode_create("/", "etc", VNODE_DIRECTORY);
    struct vnode *vnode2 = vnode_create("/", "dev", VNODE_DIRECTORY);
    struct vnode *vnode3 = vnode_create("/", "mnt", VNODE_DIRECTORY);
    struct vnode *vnode4 = vnode_create("/", "var", VNODE_DIRECTORY);
    struct vnode *vnode5 = vnode_create("/", "bin", VNODE_DIRECTORY);
    struct vnode *vnode6 = vnode_create("/", "root", VNODE_DIRECTORY);
    struct vnode *vnode7 = vnode_create("/", "home", VNODE_DIRECTORY);
    struct vnode *vnode8 = vnode_create("/", "proc", VNODE_DIRECTORY);
    struct vnode *vnode11 = vnode_create("/", "dev", VNODE_DIRECTORY);

    struct vnode *vnode12 = vnode_create("/dev", "rd0", VNODE_BLOCK_DEV);

    struct vnode *vnode9 = vnode_create("/etc", "passwd", VNODE_FILE);
    struct vnode *vnode10 = vnode_create("/etc", "config.txt", VNODE_FILE);

    vnode_write(vnode9, 0, sizeof("dustyn password"), "dustyn password");

    struct vnode *test = vnode_lookup("/etc/passwd");

    kfree(buffer);
    serial_printf("Diosfs filesystem initialized of size %i , %i byte blocks\n", DEFAULT_DIOSFS_SIZE / DIOSFS_BLOCKSIZE,
                  DIOSFS_BLOCKSIZE);
}

/*
 * The functions below encompass the vfs function pointer implementations utilizing all the vfs functions.
 */
void diosfs_remove(const struct vnode *vnode) {
    struct diosfs_filesystem_context *fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);
    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    diosfs_directory_entry_free(fs, NULL, &inode);
    release_spinlock(fs->lock);
}

void diosfs_rename(const struct vnode *vnode, char *new_name) {
    if (vnode->vnode_inode_number == ROOT_INODE) {
        return;
    }
    struct diosfs_filesystem_context *fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);
    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    safe_strcpy(inode.name, new_name, MAX_FILENAME_LENGTH);
    diosfs_write_inode(fs, &inode);
    diosfs_find_directory_entry_and_update(fs, vnode->vnode_inode_number, vnode->vnode_parent->vnode_inode_number);
    release_spinlock(fs->lock);
}

uint64_t diosfs_read(struct vnode *vnode, const uint64_t offset, char *buffer, const uint64_t bytes) {
    struct diosfs_filesystem_context *fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);

    if (vnode->vnode_type == VNODE_HARD_LINK) {
        if (diosfs_symlink_check(fs, vnode, DIOSFS_REG_FILE) != DIOSFS_SUCCESS) {
            return DIOSFS_UNEXPECTED_SYMLINK_TYPE;
        }
    }

    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    //TODO decide what I wish to do with this buffer size constraint when dealing with userspace. This works for now
    uint64_t ret = diosfs_read_bytes_from_inode(fs, &inode, buffer, PGROUNDUP(bytes),
                                                offset % fs->superblock->block_size, bytes);
    release_spinlock(fs->lock);
    return ret;
}

uint64_t diosfs_write(struct vnode *vnode, const uint64_t offset, char *buffer, const uint64_t bytes) {
    struct diosfs_filesystem_context *fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);

    if (vnode->vnode_type == DIOSFS_DIRECTORY) {
        return DIOSFS_CANNOT_WRITE_DIRECTORY;
    }

    if (vnode->vnode_type == DIOSFS_SYMLINK) {
        if (diosfs_symlink_check(fs, vnode, DIOSFS_REG_FILE) != DIOSFS_SUCCESS) {
            return DIOSFS_UNEXPECTED_SYMLINK_TYPE;
        }

        vnode = diosfs_follow_link(fs, vnode);
    }

    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    if (diosfs_write_bytes_to_inode(fs, &inode, buffer, bytes, offset, bytes) != DIOSFS_SUCCESS) {
        return DIOSFS_ERROR;
    }
    vnode->vnode_size += bytes;

    release_spinlock(fs->lock);
    return DIOSFS_SUCCESS;
}

/*
 * Unimplemented for now given there isn't really any extra data in the inode that isn't in the vnode I will sit on this
 */
uint64_t diosfs_stat(const struct vnode *vnode) {
    const struct diosfs_filesystem_context *fs = vnode->filesystem_object;
    acquire_spinlock(fs->lock);

    release_spinlock(fs->lock);
    return DIOSFS_SUCCESS;
}

/*
 * A lookup function to be invoked on a vnode via the diosfs pointer function struct.
 *
 * Ensure the filesystem is diosfs.
 *
 * Ensure the type is directory.
 *
 * If vnode is not cached, set fill_vnode so that we will fill it with children
 *
 * If the allocation flag for children pointer is not set, call vnode_directory_alloc_children to allocate memory and set the flag
 *
 * Since I am not allowing indirection with directory entries, we only need to allocate num blocks in inode * block size
 *
 * We invoke diosfs_get_directory_entries() which will fill up the buffer with directory entries
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
struct vnode *diosfs_lookup(struct vnode *parent, char *name) {
    struct diosfs_filesystem_context *fs = parent->filesystem_object;

    if (parent->vnode_filesystem_id != VNODE_FS_DIOSFS || parent->vnode_type != VNODE_DIRECTORY) {
        return NULL;
    }

    uint64_t buffer_size = fs->superblock->block_size * NUM_BLOCKS_DIRECT;

    char *buffer = kmalloc(buffer_size);
    struct diosfs_directory_entry *entries = (struct diosfs_directory_entry *) buffer;

    uint64_t ret = diosfs_get_directory_entries(fs, (struct diosfs_directory_entry *) buffer,
                                                parent->vnode_inode_number,
                                                buffer_size);

    if (ret != DIOSFS_SUCCESS) {
        kfree(buffer);
        return NULL;
    }

    uint64_t fill_vnode = false;

    if (!parent->is_cached) {
        fill_vnode = true;
    }

    struct vnode *child = NULL;

    uint8_t max_directories = parent->vnode_size / sizeof(struct diosfs_directory_entry) > VNODE_MAX_DIRECTORY_ENTRIES
                                  ? VNODE_MAX_DIRECTORY_ENTRIES
                                  : parent->vnode_size / sizeof(struct diosfs_directory_entry);

    for (uint64_t i = 0; i < max_directories; i++) {
        struct diosfs_directory_entry *entry = &entries[i];
        if (fill_vnode) {
            parent->vnode_children[i] = diosfs_directory_entry_to_vnode(parent, entry, fs);
        }
        /*
         * Check child so we don't strcmp every time after we find it in the case of filling the parent vnode with its children
         */
        if (child == NULL && safe_strcmp(name, entry->name, VFS_MAX_NAME_LENGTH)) {
            child = diosfs_directory_entry_to_vnode(parent, entry, fs);
            if (!fill_vnode) {
                goto done;
            }
        }
    }

done:
    kfree(buffer);
    return child;
}

/*
 * Create a new diosfs file/directory with name : name, type : vnode type, and parent : parent.
 * return new vnode.
 *
 * Assigning all the necessary vnode attributes and inode attributes
 */
struct vnode *diosfs_create(struct vnode *parent, char *name, const uint8_t vnode_type) {
    if (parent->vnode_filesystem_id != VNODE_FS_DIOSFS) {
        serial_printf("diosfs_create parent filesystem ID doesn't match\n");
        return NULL;
    }
    if (parent->vnode_type != VNODE_DIRECTORY) {
        serial_printf("diosfs_create parent not directory\n");
        return NULL;
    }

    struct diosfs_filesystem_context *fs = parent->filesystem_object;
    acquire_spinlock(fs->lock);
    struct vnode *new_vnode = vnode_alloc();
    struct diosfs_inode parent_inode;
    struct diosfs_inode inode;
    diosfs_get_free_inode_and_mark_bitmap(parent->filesystem_object, &inode);
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
    inode.parent_inode_number = parent->vnode_inode_number;
    safe_strcpy((char *) &inode.name, name, MAX_FILENAME_LENGTH);
    inode.uid = 0;
    diosfs_write_inode(parent->filesystem_object, &inode);

    struct diosfs_directory_entry entry = {0};
    entry.inode_number = inode.inode_number;
    entry.size = 0;
    safe_strcpy(entry.name, name, MAX_FILENAME_LENGTH);
    entry.parent_inode_number = inode.parent_inode_number;
    entry.device_number = fs->device->device_minor;
    entry.type = inode.type;
    uint64_t ret = diosfs_write_dirent(fs, &parent_inode, &entry);
    diosfs_write_inode(fs, &parent_inode);
    release_spinlock(fs->lock);

    if (ret != DIOSFS_SUCCESS) {
        panic("Could not write dirent");
        return NULL;
    }
    return new_vnode;
}

/*
 * Unimplemented for diosfs as I see no use for it. It may be useful for disk filesystems for block cache flushing and other kinds of
 * state teardown but for diosfs it is not required.
 */
void diosfs_close(struct vnode *vnode, uint64_t handle) {
    asm("nop");
}

uint64_t diosfs_open(struct vnode *vnode) {
    asm("nop");
    return DIOSFS_SUCCESS;
}

struct vnode *diosfs_link(struct vnode *parent, struct vnode *vnode_to_link, uint8_t type) {
    struct diosfs_filesystem_context *fs = parent->filesystem_object;
    acquire_spinlock(fs->lock);
    switch (type) {
        case VNODE_HARD_LINK:
            panic("unimplemented hard link diosfs");
            break;

        case VNODE_SYM_LINK:
            return diosfs_create(parent, vnode_get_canonical_path(vnode_to_link), VNODE_SYM_LINK);
        default: ;
    }

    release_spinlock(fs->lock);
}

void diosfs_unlink(struct vnode *vnode) {
    struct diosfs_inode inode;
    struct diosfs_filesystem_context *fs = vnode->filesystem_object;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    if (inode.type == DIOSFS_SYMLINK) {
        diosfs_remove_file(fs, &inode);
        vnode_free(vnode);
    }
    if (inode.refcount <= 1) {
        diosfs_remove(vnode);
    } else {
        inode.refcount--;
        diosfs_write_inode(fs, &inode);
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
 * Simple function for taking a diosfs directory entry and converting it a vnode,
 * and returning said vnode.
 */
static struct vnode *diosfs_directory_entry_to_vnode(struct vnode *parent, struct diosfs_directory_entry *entry,
                                                     struct diosfs_filesystem_context *fs) {
    struct vnode *vnode = vnode_alloc();
    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);
    vnode->vnode_type = entry->type;
    vnode->vnode_device_id = entry->device_number;
    vnode->vnode_size = entry->size;
    vnode->vnode_inode_number = entry->inode_number;
    vnode->vnode_filesystem_id = VNODE_FS_DIOSFS;\
    vnode->vnode_ops = &diosfs_vnode_ops;
    vnode->vnode_refcount = inode.refcount;
    vnode->is_mount_point = FALSE;
    vnode->mounted_vnode = NULL;
    vnode->is_cached = FALSE;
    safe_strcpy(vnode->vnode_name, entry->name, MAX_FILENAME_LENGTH);
    vnode->last_updated = 0;
    vnode->num_children = entry->type == DIOSFS_DIRECTORY
                              ? entry->size
                              : 0;
    vnode->filesystem_object = fs;
    vnode->vnode_parent = parent;

    return vnode;
}


static uint64_t diosfs_symlink_check(struct diosfs_filesystem_context *fs, struct vnode *vnode, int32_t expected_type) {
    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    uint8_t temp_buffer[DIOSFS_BLOCKSIZE] = {0};
    diosfs_read_block_by_number(inode.blocks[0], temp_buffer, fs, 0, DIOSFS_BLOCKSIZE);
    struct vnode *link = vnode_lookup(temp_buffer);

    if (link == NULL) {
        release_spinlock(fs->lock);
        return DIOSFS_BAD_SYMLINK;
    }

    if (link->vnode_type != expected_type) {
        release_spinlock(fs->lock);
        return DIOSFS_UNEXPECTED_SYMLINK_TYPE;
    }

    return DIOSFS_SUCCESS;
}


static struct vnode *diosfs_follow_link(struct diosfs_filesystem_context *fs, const struct vnode *vnode) {
    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, vnode->vnode_inode_number);

    uint8_t temp_buffer[DIOSFS_BLOCKSIZE] = {0};
    diosfs_read_block_by_number(inode.blocks[0], temp_buffer, fs, 0, DIOSFS_BLOCKSIZE);
    struct vnode *link = vnode_lookup(temp_buffer);
    return link;
}

/*
 * Can either pass a directory entry inside the target directory or an inode number of the directory
 *
 * This function must be used to remove a dirent , recursive deletion should not be used only called from this function.
 *
 */
static uint64_t diosfs_directory_entry_free(struct diosfs_filesystem_context *fs, struct diosfs_directory_entry *entry,
                                            const struct diosfs_inode *inode) {
    struct diosfs_inode parent_inode;

    if (entry != NULL) {
        diosfs_read_inode(fs, &parent_inode, entry->parent_inode_number);
    } else {
        diosfs_read_inode(fs, &parent_inode, inode->parent_inode_number);
    }

    uint64_t inode_to_remove = entry == NULL ? inode->inode_number : entry->inode_number;
    char *buffer = kmalloc(PAGE_SIZE);
    struct diosfs_directory_entry *entries = (struct diosfs_directory_entry *) buffer;

    /*
     * Iterate through each dirent , loading the next block when the modulus operation resets back to 0
     * If there is more than 1 dirent when the target is found, move the last dirent to the place of the removed one.
     */
    for (uint64_t i = 0; i < parent_inode.size; i++) {
        if (i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK == 0) {
            diosfs_read_block_by_number(parent_inode.blocks[i / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                        fs->superblock->block_size);
        }

        if (entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number == inode_to_remove) {
            if (entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK].type == DIOSFS_DIRECTORY) {
                diosfs_recursive_directory_entry_free(fs, &entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], 0);
            } else if (entries[i].type == DIOSFS_REG_FILE) {
                struct diosfs_inode temp_inode;
                diosfs_read_inode(fs, &temp_inode, entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number);

                /*
                 * If this is the only reference remove it , otherwise just drops to setting the entry memory only and decrementing refcount.
                 */

                if (temp_inode.refcount <= 1) {
                    diosfs_remove_file(fs, &temp_inode);
                } else {
                    temp_inode.refcount--;
                    diosfs_write_inode(fs, &temp_inode);
                }
            }
            char *shift_buffer = kmalloc(PAGE_SIZE);

            if (parent_inode.size > 1) {
                diosfs_read_block_by_number(parent_inode.blocks[parent_inode.size / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK],
                                            shift_buffer, fs, 0,
                                            fs->superblock->block_size);

                struct diosfs_directory_entry *entries2 = (struct diosfs_directory_entry *) shift_buffer;

                memmove(&entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK],
                        &entries2[parent_inode.size % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK],
                        sizeof(struct diosfs_directory_entry));

                memset(&entries2[parent_inode.size % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], 0,
                       sizeof(struct diosfs_directory_entry));

                /*
                 *  If the just shifted dirent was the only entry in a block, free that block
                 */
                if (parent_inode.size % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK == 0) {
                    diosfs_free_block_and_mark_bitmap(
                        fs, parent_inode.blocks[parent_inode.size / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK]);
                    parent_inode.block_count--;
                }

                //write the new block with the dirent taken out
                diosfs_write_block_by_number(parent_inode.blocks[i / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                             fs->superblock->block_size);
            } else {
                memset(&entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], 0, sizeof(struct diosfs_directory_entry));
            }
            //write the new block from which we just removed the dirent from
            diosfs_write_block_by_number(parent_inode.blocks[i / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                         fs->superblock->block_size);

            parent_inode.size -= 1;
            diosfs_write_inode(fs, &parent_inode);
            kfree(buffer);
            kfree(shift_buffer);

            return DIOSFS_SUCCESS;
        }
    }

    kfree(buffer);
    return DIOSFS_NOT_FOUND;
}

static void shift_directory_entry(const struct diosfs_filesystem_context *fs, const uint64_t entry_number,
                                  struct diosfs_inode *parent_inode) {
    if (entry_number == parent_inode->size - 1) {
        return;
    }

    char *buffer = kmalloc(PAGE_SIZE);
    char *shift_buffer = kmalloc(PAGE_SIZE);

    uint64_t target_block = parent_inode->blocks[entry_number / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK];
    uint64_t target_entry_in_block = entry_number % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK;
    uint64_t last_block = parent_inode->blocks[parent_inode->size / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK];
    uint64_t last_entry = parent_inode->size - 1 % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK;

    diosfs_read_block_by_number(target_block, buffer, fs, 0, DIOSFS_BLOCKSIZE);
    diosfs_read_block_by_number(last_block, shift_buffer, fs, 0, DIOSFS_BLOCKSIZE);

    struct diosfs_directory_entry *entries = (struct diosfs_directory_entry *) shift_buffer;
    struct diosfs_directory_entry *entries2 = (struct diosfs_directory_entry *) buffer;

    entries2[target_entry_in_block] = entries[last_entry];
    memset(&entries[last_block], 0, sizeof(struct diosfs_directory_entry));

    diosfs_write_block_by_number(target_block, buffer, fs, 0, DIOSFS_BLOCKSIZE);
    diosfs_write_block_by_number(last_block, shift_buffer, fs, 0, DIOSFS_BLOCKSIZE);
    diosfs_write_inode(fs, parent_inode);

    kfree(buffer);
    kfree(shift_buffer);
}

/*
 * This could pose an issue with stack space but we'll run with it. If it works it works, if it doesn't we'll either change it or allocate more stack space.
 */
static void diosfs_recursive_directory_entry_free(struct diosfs_filesystem_context *fs,
                                                  const struct diosfs_directory_entry *entry,
                                                  const uint64_t inode_number) {
    struct diosfs_inode inode;

    if (entry != NULL) {
        diosfs_read_inode(fs, &inode, entry->inode_number);
    } else {
        diosfs_read_inode(fs, &inode, inode_number);
    }

    char *buffer = kmalloc(PAGE_SIZE);
    struct diosfs_directory_entry *entries = (struct diosfs_directory_entry *) buffer;

    for (uint64_t i = 0; i < inode.size; i++) {
        if (i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK == 0) {
            diosfs_read_block_by_number(inode.blocks[i / DIOSFS_MAX_FILES_IN_DIRENT_BLOCK], buffer, fs, 0,
                                        fs->superblock->block_size);
        }

        if (entries[i].type == DIOSFS_REG_FILE) {
            struct diosfs_inode temp_inode;
            diosfs_read_inode(fs, &temp_inode, entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number);
            /*
                        * If this is the only reference remove it , otherwise just drops to setting the entry memory only and decrementing refcount.
                        */
            if (temp_inode.refcount <= 1) {
                diosfs_remove_file(fs, &temp_inode);
            } else {
                temp_inode.refcount--;
                diosfs_write_inode(fs, &temp_inode);
            }
        } else if (entries[i].type == DIOSFS_DIRECTORY) {
            diosfs_recursive_directory_entry_free(fs, &entries[i], 0);
        } else {
            struct diosfs_inode temp_inode;
            diosfs_read_inode(fs, &temp_inode, entries[i % DIOSFS_MAX_FILES_IN_DIRENT_BLOCK].inode_number);
            diosfs_remove_file(fs, &temp_inode);
        }
    }

    diosfs_remove_file(fs, &inode);
    diosfs_write_inode(fs, &inode);
    kfree(buffer);
}

/*
 *Remove regular file, free all blocks, clear the inode out, free the inode
 */
static void diosfs_remove_file(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode) {
    free_blocks_from_inode(fs, 0, inode->block_count, inode);
    memset(inode->name, 0, MAX_FILENAME_LENGTH);
    diosfs_write_inode(fs, inode);
    diosfs_free_inode_and_mark_bitmap(fs, inode->inode_number);
}

/*
 * Clear a block or inode bitmap entry , essentially the same thing reversed but I do not read the entire bitmap section in because we know which block we need
 * since we're given a number.
 */
static void diosfs_clear_bitmap(const struct diosfs_filesystem_context *fs, const uint8_t type, const uint64_t number) {
    if (type != BITMAP_TYPE_BLOCK && type != BITMAP_TYPE_INODE) {
        panic("Unknown type diosfs_clear_bitmap");
    }

    uint64_t block_to_read;
    uint64_t block;

    if (type == BITMAP_TYPE_BLOCK) {
        block_to_read = (number / (fs->superblock->block_size * 8));
        block = fs->superblock->block_bitmap_pointers_start + block_to_read;
    } else {
        block_to_read = number / (fs->superblock->block_size * 8);
        block = fs->superblock->inode_bitmap_pointers_start + block_to_read;
    }

    uint64_t bit = number % 8;

    char *buffer = kmalloc(PAGE_SIZE);
    memset(buffer, 0, PAGE_SIZE);
    fs->device->device_ops->block_device_ops->block_read(block * fs->superblock->block_size + 0,
                                                         fs->superblock->block_size, buffer, fs->device);
    /*
     * 0 the bit and write it back Noting that this doesnt work for a set but Im not sure that
     * I will use it for that.
     */
    buffer[BYTE(number)] &= ~BIT(bit);
    fs->device->device_ops->block_device_ops->block_write(block * fs->superblock->block_size + 0,
                                                          fs->superblock->block_size, buffer, fs->device);
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
static void diosfs_get_free_inode_and_mark_bitmap(const struct diosfs_filesystem_context *fs,
                                                  struct diosfs_inode *inode_to_be_filled) {
    uint64_t buffer_size = PAGE_SIZE * 16;
    char *buffer = kmalloc(buffer_size);
    uint64_t block = 0;
    uint64_t byte = 0;
    uint64_t bit = 0;
    uint64_t inode_number;

    uint64_t ret = fs->device->device_ops->block_device_ops->block_read(
        fs->superblock->inode_bitmap_pointers_start * fs->superblock->block_size,
        fs->superblock->block_size * (fs->superblock->inode_bitmap_size), buffer, fs->device);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_get_free_inode_and_mark_bitmap");
        panic("diosfs_get_free_inode_and_mark_bitmap ramdisk read failed");
    }

    while (1) {
        if (buffer[block * fs->superblock->block_size + byte] != 0xFF) {
            for (uint64_t i = 0; i <= 8; i++) {
                if (!(buffer[(block * fs->superblock->block_size) + byte] & (1 << i))) {
                    bit = i;
                    buffer[byte] |= BIT(bit);
                    inode_number = (block * fs->superblock->block_size * 8) + (byte * 8) + bit;
                    goto found_free;
                }
            }

            byte++;
            if (byte == fs->superblock->block_size) {
                block++;
                byte = 0;
            }
            if (block > DIOSFS_NUM_INODE_POINTER_BLOCKS) {
                panic("diosfs_get_free_inode_and_mark_bitmap: No free inodes");
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        } else {
            byte++;
        }

        if (byte == fs->superblock->block_size) {
            block++;
            byte = 0;
        }
    }


found_free:
    memset(inode_to_be_filled, 0, sizeof(struct diosfs_inode));
    inode_to_be_filled->inode_number = inode_number;

    ret = fs->device->device_ops->block_device_ops->block_write(
        fs->superblock->inode_bitmap_pointers_start * fs->superblock->block_size,
        fs->superblock->block_size * (fs->superblock->inode_bitmap_size), buffer, fs->device);


    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_get_free_inode_and_mark_bitmap");
        panic("diosfs_get_free_inode_and_mark_bitmap ramdisk read failed");
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

static uint64_t diosfs_get_free_block_and_mark_bitmap(const struct diosfs_filesystem_context *fs) {
    uint64_t buffer_size = fs->superblock->block_bitmap_size * fs->superblock->block_size;
    char *buffer = kmalloc(buffer_size);
    uint64_t block = 0;
    uint64_t byte = 0;
    uint64_t bit = 0;
    uint64_t offset = 0;
    uint64_t block_number = 0;

retry:

    /*
     *
     *We do not free the buffer we simply write into a smaller and smaller portion of the buffer.
     *It is only freed after a block is found and the new bitmap block is written.
     *
     *We do not use the diosfs_read_block function because it only works with single blocks and we are reading
     *many blocks here so we will work with ramdisk functions directly.
     *
     */
    uint64_t ret = fs->device->device_ops->block_device_ops->block_read(
        fs->superblock->block_bitmap_pointers_start * fs->superblock->block_size,
        fs->superblock->block_size * fs->superblock->block_bitmap_pointers_start, buffer, fs->device);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_get_free_inode_and_mark_bitmap");
        panic("diosfs_get_free_inode_and_mark_bitmap ramdisk read failed");
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
            if (block > DIOSFS_NUM_BLOCK_POINTER_BLOCKS) {
                panic("diosfs_get_free_inode_and_mark_bitmap: No free inodes");
                /* Panic for visibility so I can tweak sizes for this if it happens */
            }
        } else {
            byte++;
        }
    }


found_free:
    ret = fs->device->device_ops->block_device_ops->block_write(
        fs->superblock->block_bitmap_pointers_start * fs->superblock->block_size,
        fs->superblock->block_size * fs->superblock->block_bitmap_pointers_start, buffer, fs->device);


    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_get_free_inode_and_mark_bitmap write call")
        panic("diosfs_get_free_inode_and_mark_bitmap"); /* Extreme but that is okay for diagnosing issues */
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
static uint64_t diosfs_free_block_and_mark_bitmap(struct diosfs_filesystem_context *fs, const uint64_t block_number) {
    char *buffer = kmalloc(PAGE_SIZE);
    memset(buffer, 0, fs->superblock->block_size);

    diosfs_write_block_by_number(block_number, buffer, fs, 0, fs->superblock->block_size);

    diosfs_clear_bitmap(fs, BITMAP_TYPE_BLOCK, block_number);

    kfree(buffer);
    return DIOSFS_SUCCESS;
}

/*
 * I don't think I need locks on frees, I will find out one way or another if this is true
 */
static uint64_t diosfs_free_inode_and_mark_bitmap(struct diosfs_filesystem_context *fs, const uint64_t inode_number) {
    struct diosfs_inode inode;
    diosfs_read_inode(fs, &inode, inode_number);
    memset(&inode, 0, sizeof(struct diosfs_inode));
    inode.inode_number = inode_number;
    diosfs_write_inode(fs, &inode); /* zero the inode leaving just the inode number */
    uint64_t inode_number_block = inode_number / DIOSFS_INODES_PER_BLOCK;
    uint64_t offset = inode_number % DIOSFS_INODES_PER_BLOCK;

    char *buffer = kmalloc(PAGE_SIZE);

    diosfs_write_block_by_number(fs->superblock->inode_start_pointer + inode_number_block, buffer, fs, offset,
                                 DIOSFS_INODE_SIZE);

    diosfs_clear_bitmap(fs, BITMAP_TYPE_INODE, inode_number);

    kfree(buffer);
    return DIOSFS_SUCCESS;
}

/*
 * This function will fill the children array with directory entries found in the inode number passed to it. May make more sense to pass the inode directly or just a pointer to it but this is ok for now.
 *
 *
 * Checks children_size to not write out of bounds.
 *
 * Get the inode via diosfs_get_inode
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
static uint64_t diosfs_get_directory_entries(struct diosfs_filesystem_context *fs,
                                             struct diosfs_directory_entry *children, const uint64_t inode_number,
                                             const uint64_t children_size) {
    uint64_t buffer_size = PAGE_SIZE;
    char *buffer = kmalloc(buffer_size);
    struct diosfs_directory_entry *directory_entries = (struct diosfs_directory_entry *) buffer;
    struct diosfs_inode inode;
    uint64_t directory_entries_read = 0;
    uint64_t directory_block = 0;
    uint64_t block_number = 0;

    diosfs_read_inode(fs, &inode, inode_number);
    if (inode.type != DIOSFS_DIRECTORY) {
        kfree(buffer);
        return DIOSFS_NOT_A_DIRECTORY;
    }

    if (children_size < inode.block_count * fs->superblock->block_size) {
        return DIOSFS_BUFFER_TOO_SMALL;
    }


    while (1) {
        block_number = inode.blocks[directory_block++];
        /*Should be okay to leave this unrestrained since we check children size and inode size */

        diosfs_read_block_by_number(block_number, buffer, fs, 0, fs->superblock->block_size);

        for (uint64_t i = 0; i < (fs->superblock->block_size / sizeof(struct diosfs_directory_entry)); i++) {
            if (directory_entries_read == inode.size) {
                goto done;
            }
            children[directory_entries_read++] = directory_entries[i];
        }
    }


done:
    kfree(buffer);
    return DIOSFS_SUCCESS;
}

/*
 * This function exists to update the directory entry when a file name changes or size changes from a write or the like.
 */
static uint64_t diosfs_find_directory_entry_and_update(struct diosfs_filesystem_context *fs,
                                                       const uint64_t inode_number,
                                                       const uint64_t directory_inode_number) {
    uint64_t buffer_size = PAGE_SIZE;
    char *buffer = kmalloc(buffer_size);
    struct diosfs_directory_entry *directory_entries = (struct diosfs_directory_entry *) buffer;
    struct diosfs_directory_entry *directory_entry;
    struct diosfs_inode inode;
    struct diosfs_inode entry;
    uint64_t directory_entries_read = 0;
    uint64_t directory_block = 0;
    uint64_t block_number = 0;

    diosfs_read_inode(fs, &entry, inode_number);
    diosfs_read_inode(fs, &inode, directory_inode_number);
    if (inode.type != DIOSFS_DIRECTORY) {
        kfree(buffer);
        return DIOSFS_NOT_A_DIRECTORY;
    }


    while (1) {
        block_number = inode.blocks[directory_block++];
        /*Should be okay to leave this unrestrained since we check children size and inode size */

        diosfs_read_block_by_number(block_number, buffer, fs, 0, fs->superblock->block_size);

        for (uint64_t i = 0; i < (fs->superblock->block_size / sizeof(struct diosfs_directory_entry)); i++) {
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
    safe_strcpy(directory_entry->name, entry.name, MAX_FILENAME_LENGTH);
    directory_entry->size = entry.size;
    directory_entry->parent_inode_number = entry.parent_inode_number;

    if (entry.parent_inode_number != directory_inode_number) {
        panic("diosfs_find_directory_entry_and_update entry parent does not match expected parent");
    }

    diosfs_write_block_by_number(block_number, buffer, fs, 0, DIOSFS_BLOCKSIZE);
    return DIOSFS_SUCCESS;

not_found:
    panic("diosfs_find_directory_entry_and_update entry not found; this should not happen");
    kfree(buffer);
    return DIOSFS_SUCCESS;
}

/*
 * Fills a buffer with file data , following block pointers and appending bytes to the passed buffer.
 *
 * Took locks out since this will be called from a function that is locked.
 */
static uint64_t diosfs_read_bytes_from_inode(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                             char *buffer,
                                             const uint64_t buffer_size,
                                             const uint64_t offset, const uint64_t read_size_bytes) {
    uint64_t num_blocks_to_read = read_size_bytes / fs->superblock->block_size;

    if (inode->type != DIOSFS_REG_FILE) {
        panic("diosfs_write_bytes_to_inode bad type");
    }


    if (buffer_size < fs->superblock->block_size || buffer_size < fs->superblock->block_size * num_blocks_to_read) {
        /*
         * I'll set a sensible minimum buffer size
         */
        return DIOSFS_BUFFER_TOO_SMALL;
    }

    uint64_t start_block = offset / fs->superblock->block_size;

    if (offset > inode->size) {
        panic("diosfs_read_bytes_from_inode bad offset"); /* Should never happen, panic for visibility  */
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
        } else {
            byte_size = bytes_to_read;
        }
        current_block_number = diosfs_get_relative_block_number_from_file(inode, i, fs);

        diosfs_read_block_by_number(current_block_number, buffer + bytes_read, fs, start_offset, byte_size);

        bytes_read += byte_size;
        bytes_to_read -= byte_size;

        if (start_offset) {
            /*
             * Offset is only for first and last block so set to 0 after the first block
             */
            start_offset = 0;
        }
    }

    return DIOSFS_SUCCESS;
}

/*
 * Write a directory entry into a directory and handle block allocation, size changes accordingly
 */
static uint64_t diosfs_write_dirent(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                    const struct diosfs_directory_entry *entry) {
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

    char *read_buffer = kmalloc(PAGE_SIZE);
    struct diosfs_directory_entry *diosfs_directory_entries = (struct diosfs_directory_entry *) read_buffer;

    diosfs_read_block_by_number(inode->blocks[block], read_buffer, fs, 0, fs->superblock->block_size);

    memcpy(&diosfs_directory_entries[entry_in_block], entry, sizeof(struct diosfs_directory_entry));
    inode->size++;

    diosfs_write_block_by_number(inode->blocks[block], read_buffer, fs, 0, fs->superblock->block_size);
    diosfs_write_inode(fs, inode);

    kfree(read_buffer);
    return DIOSFS_SUCCESS;
}


static uint64_t diosfs_write_bytes_to_inode(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                            char *buffer,
                                            const uint64_t buffer_size,
                                            const uint64_t offset,
                                            const uint64_t write_size_bytes) {
    if (inode->type != DIOSFS_REG_FILE) {
        panic("diosfs_write_bytes_to_inode bad type");
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
        return DIOSFS_BUFFER_TOO_SMALL;
    }

    if (offset > inode->size) {
        serial_printf("offset %i write_size_bytes %i inode size %i\n", offset, write_size_bytes, inode->size);
        panic("diosfs_write_bytes_from_inode bad offset"); /* Should never happen, panic for visibility  */
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
            byte_size = fs->superblock->block_size - start_offset;
        } else {
            byte_size = bytes_left;
        }

        current_block_number = diosfs_get_relative_block_number_from_file(inode, i, fs);

        diosfs_write_block_by_number(current_block_number, buffer, fs, start_offset, byte_size);


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

    diosfs_write_inode(fs, inode);
    diosfs_find_directory_entry_and_update(fs, inode->inode_number, inode->parent_inode_number);
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
                                                           const uint64_t current_block,
                                                           struct diosfs_filesystem_context *fs) {
    char *temp_buffer = kmalloc(PAGE_SIZE);
    const struct diosfs_byte_offset_indices indices = diosfs_indirection_indices_for_block_number(current_block);
    uint64_t current_block_number = 0;
    uint64_t *indirection_block = (uint64_t *) temp_buffer;

    switch (indices.levels_indirection) {
        case 0:
            current_block_number = inode->blocks[indices.direct_block_number];
            goto done;

        case 1:

            diosfs_read_block_by_number(inode->single_indirect, temp_buffer, fs, 0, fs->superblock->block_size);
            current_block_number = indirection_block[indices.first_level_block_number];
            goto done;

        case 2:

            diosfs_read_block_by_number(inode->double_indirect, temp_buffer, fs, 0, fs->superblock->block_size);
            current_block_number = indirection_block[indices.second_level_block_number];
            diosfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, fs->superblock->block_size);
            current_block_number = indirection_block[indices.first_level_block_number];
            goto done;

        case 3:

            diosfs_read_block_by_number(inode->triple_indirect, temp_buffer, fs, 0, fs->superblock->block_size);
            current_block_number = indirection_block[indices.third_level_block_number];
            diosfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, fs->superblock->block_size);
            current_block_number = indirection_block[indices.second_level_block_number];
            diosfs_read_block_by_number(current_block_number, temp_buffer, fs, 0, fs->superblock->block_size);
            current_block_number = indirection_block[indices.first_level_block_number];
            goto done;

        default:
            panic("diosfs_get_next_logical_block_from_file: unknown indirection");
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
uint64_t diosfs_inode_allocate_new_blocks(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                          uint32_t num_blocks_to_allocate) {
    struct diosfs_byte_offset_indices result;
    char *buffer = kmalloc(PAGE_SIZE);

    // Do not allocate blocks for a directory since they hold enough entries (90 or so at the time of writing)
    if (inode->type == DIOSFS_DIRECTORY && (num_blocks_to_allocate + (inode->size / fs->superblock->block_size)) >
        NUM_BLOCKS_DIRECT) {
        serial_printf("diosfs_inode_allocate_new_block inode type not directory!\n");
        kfree(buffer);
        return DIOSFS_ERROR;
    }

    if (num_blocks_to_allocate + (inode->size / fs->superblock->block_size) > MAX_BLOCKS_IN_INODE) {
        serial_printf("diosfs_inode_allocate_new_block too many blocks to request!\n");
        kfree(buffer);
        return DIOSFS_ERROR;
    }

    if (inode->size % fs->superblock->block_size == 0) {
        result = diosfs_indirection_indices_for_block_number(inode->size / fs->superblock->block_size);
    } else {
        result = diosfs_indirection_indices_for_block_number((inode->size / fs->superblock->block_size) + 1);
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
            panic("diosfs_inode_allocate_new_block: unknown indirection");
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
    diosfs_allocate_single_indirect_block(fs, inode, num_allocated, num_in_indirect, false, 0);
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
    diosfs_allocate_double_indirect_block(fs, inode, num_allocated, num_in_indirect, false, 0);
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
    kfree(buffer);
    diosfs_write_inode(fs, inode);
    return DIOSFS_SUCCESS;
}


/*
 * A much simpler index derivation function for getting a diosfs index based off a block number given. Could be for reading a block at an arbitrary offset or finding the end
 * of the file to allocate or write.BL
 *
 * It is fairly simple.
 *
 * We avoid off-by-ones by using the greater than as opposed to greater than or equal operator.
 *
 * Panic if the block number is too high
 */

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


    panic("diosfs_indirection_indices_for_block_number invalid block number");
}

/*
 * 4 functions beneath just take some of the ramdisk calls out in favor of local functions for read and writing blocks and inodes
 */
static void diosfs_write_inode(const struct diosfs_filesystem_context *fs, struct diosfs_inode *inode) {
    uint64_t inode_number_in_block = inode->inode_number % NUM_INODES_PER_BLOCK;
    uint64_t block_number = fs->superblock->inode_start_pointer + inode->inode_number / NUM_INODES_PER_BLOCK;
    uint64_t ret = fs->device->device_ops->block_device_ops->block_write(
        (block_number * fs->superblock->block_size) + inode_number_in_block * sizeof(struct diosfs_inode),
        sizeof(struct diosfs_inode), (char *) inode, fs->device);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_write_inode");
        panic("diosfs_write_inode"); /* For diagnostic purposes */
    }
}

static void diosfs_read_inode(const struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                              uint64_t inode_number) {
    uint64_t inode_number_in_block = inode_number % NUM_INODES_PER_BLOCK;
    uint64_t block_number = fs->superblock->inode_start_pointer + (inode_number / NUM_INODES_PER_BLOCK);
    uint64_t ret = fs->device->device_ops->block_device_ops->block_read(
        (block_number * fs->superblock->block_size) + (inode_number_in_block * sizeof(struct diosfs_inode)),
        sizeof(struct diosfs_inode), (char *) inode, fs->device);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_write_inode");
        panic("diosfs_write_inode"); /* For diagnostic purposes */
    }
}

static void diosfs_write_block_by_number(const uint64_t block_number, const char *buffer,
                                         const struct diosfs_filesystem_context *fs,
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
    uint64_t ret = fs->device->device_ops->block_device_ops->block_write(
        (fs->superblock->block_start_pointer * fs->superblock->block_size) +
        (block_number * fs->superblock->block_size), fs->superblock->block_size, buffer, fs->device);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_write_block_by_number");
        panic("diosfs_write_block_by_number");
    }
}

static void diosfs_read_block_by_number(const uint64_t block_number, char *buffer,
                                        const struct diosfs_filesystem_context *fs,
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
    uint64_t ret = fs->device->device_ops->block_device_ops->block_read(
        (fs->superblock->block_start_pointer * fs->superblock->block_size) +
        (block_number * fs->superblock->block_size), fs->superblock->block_size, buffer, fs->device);

    if (ret != DIOSFS_SUCCESS) {
        HANDLE_DISK_ERROR(ret, "diosfs_read_block_by_number");
        panic("diosfs_read_block_by_number");
    }
}

/*
 * This function will start at block offset and go to num_blocks in order to free as many blocks as requested
 */
static void free_blocks_from_inode(struct diosfs_filesystem_context *fs,
                                   uint64_t block_start, uint64_t num_blocks,
                                   struct diosfs_inode *inode) {
    if (inode->block_count == 0) {
        return;
    }
    uint64_t block_number = block_start;
    uint64_t indirection_block;
    uint64_t double_indirection_block;
    uint64_t triple_indirection_block;
    char *buffer = kmalloc(PAGE_SIZE);
    char *buffer2 = kmalloc(PAGE_SIZE);
    char *buffer3 = kmalloc(PAGE_SIZE);
    uint64_t *blocks = (uint64_t *) buffer;
    uint64_t *blocks_2 = (uint64_t *) buffer2;
    uint64_t *blocks_3 = (uint64_t *) buffer3;

    struct diosfs_byte_offset_indices byte_offset_indices = diosfs_indirection_indices_for_block_number(block_number);

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
            panic("diosfs_indirection_indices_for_block_number invalid block number");
    }


level_zero:
    for (uint64_t i = byte_offset_indices.direct_block_number; i < NUM_BLOCKS_DIRECT; i++) {
        diosfs_free_block_and_mark_bitmap(fs, inode->blocks[i]);
        num_blocks--;
        if (num_blocks == 0) {
            goto done;
        }
    }

    indirection_block = 0;

level_one:
    diosfs_read_block_by_number(inode->single_indirect, buffer, fs, 0, fs->superblock->block_size);

    for (uint64_t i = indirection_block; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i++) {
        diosfs_free_block_and_mark_bitmap(fs, blocks[i]);
        num_blocks--;
        if (num_blocks == 0) {
            goto done;
        }
    }

    double_indirection_block = 0;
level_two:
    diosfs_read_block_by_number(inode->double_indirect, buffer, fs, 0, fs->superblock->block_size);
    for (uint64_t i = double_indirection_block / NUM_BLOCKS_IN_INDIRECTION_BLOCK; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i
         ++) {
        diosfs_read_block_by_number(blocks[i], buffer2, fs, 0, fs->superblock->block_size);

        for (uint64_t j = double_indirection_block % NUM_BLOCKS_IN_INDIRECTION_BLOCK; j <
                 NUM_BLOCKS_IN_INDIRECTION_BLOCK; j++) {
            diosfs_free_block_and_mark_bitmap(fs, blocks_2[j]);
            num_blocks--;
            if (num_blocks == 0) {
                goto done;
            }
        }
    }
    triple_indirection_block = 0;
level_three:
    diosfs_read_block_by_number(inode->triple_indirect, buffer, fs, 0, fs->superblock->block_size);
    for (uint64_t i = triple_indirection_block / NUM_BLOCKS_DOUBLE_INDIRECTION; i < NUM_BLOCKS_IN_INDIRECTION_BLOCK; i
         ++) {
        diosfs_read_block_by_number(blocks[i], buffer2, fs, 0, fs->superblock->block_size);

        for (uint64_t j = triple_indirection_block / NUM_BLOCKS_IN_INDIRECTION_BLOCK; j <
                 NUM_BLOCKS_IN_INDIRECTION_BLOCK; j++) {
            diosfs_read_block_by_number(blocks_2[j], buffer3, fs, 0, fs->superblock->block_size);

            for (uint64_t k = triple_indirection_block % NUM_BLOCKS_IN_INDIRECTION_BLOCK; k <
                     NUM_BLOCKS_IN_INDIRECTION_BLOCK; k++) {
                diosfs_free_block_and_mark_bitmap(fs, blocks_3[k]);
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

static uint64_t diosfs_allocate_triple_indirect_block(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate) {
    char *buffer = kmalloc(PAGE_SIZE);
    uint64_t *block_array = (uint64_t *) buffer;
    uint64_t index;
    uint64_t allocated;
    uint64_t triple_indirect = inode->triple_indirect == 0
                                   ? diosfs_get_free_block_and_mark_bitmap(fs)
                                   : inode->triple_indirect;
    inode->triple_indirect = triple_indirect; // could be more elegant but this is fine

    diosfs_read_block_by_number(triple_indirect, buffer, fs, 0, fs->superblock->block_size);

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
        block_array[index++] = diosfs_allocate_double_indirect_block(fs, inode, allocated, amount, true, 0);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
    }

    diosfs_write_block_by_number(triple_indirect, buffer, fs, 0, fs->superblock->block_size);
    kfree(buffer);
    return triple_indirect;
}

static uint64_t diosfs_allocate_double_indirect_block(struct diosfs_filesystem_context *fs, struct diosfs_inode *inode,
                                                      uint64_t num_allocated, uint64_t num_to_allocate,
                                                      bool higher_order, uint64_t block_number) {
    uint64_t index;
    uint64_t allocated;
    char *buffer = kmalloc(PAGE_SIZE);
    uint64_t double_indirect;

    if (higher_order && block_number == 0) {
        double_indirect = diosfs_get_free_block_and_mark_bitmap(fs);
    } else if (!higher_order && block_number == 0) {
        double_indirect = inode->double_indirect == 0
                              ? diosfs_get_free_block_and_mark_bitmap(fs)
                              : inode->double_indirect;
        inode->double_indirect = double_indirect;
    } else {
        double_indirect = block_number;
    }


    diosfs_read_block_by_number(double_indirect, buffer, fs, 0, fs->superblock->block_size);
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
        block_array[index] = diosfs_allocate_single_indirect_block(fs, inode, allocated, amount, true, block);

        allocated = 0; // reset after the first allocation round
        num_to_allocate -= amount;
        index++;
        block = block_array[index];
    }
    diosfs_write_block_by_number(double_indirect, buffer, fs, 0, fs->superblock->block_size);
    diosfs_write_inode(fs, inode);
    kfree(buffer);
    return double_indirect;
}


static uint64_t diosfs_allocate_single_indirect_block(const struct diosfs_filesystem_context *fs,
                                                      struct diosfs_inode *inode,
                                                      const uint64_t num_allocated, uint64_t num_to_allocate,
                                                      const bool higher_order, const uint64_t block_number) {
    if (num_allocated + num_to_allocate >= NUM_BLOCKS_IN_INDIRECTION_BLOCK) {
        num_to_allocate = NUM_BLOCKS_IN_INDIRECTION_BLOCK - num_allocated;
    }

    char *buffer = kmalloc(PAGE_SIZE);
    uint64_t single_indirect = block_number;

    if (higher_order && block_number == 0) {
        single_indirect = diosfs_get_free_block_and_mark_bitmap(fs);
    } else if (!higher_order && block_number == 0) {
        single_indirect = inode->single_indirect == 0
                              ? diosfs_get_free_block_and_mark_bitmap(fs)
                              : inode->single_indirect;
        inode->single_indirect = single_indirect;
    } else {
        single_indirect = block_number;
    }

    diosfs_read_block_by_number(single_indirect, buffer, fs, 0, fs->superblock->block_size);
    /* We can probably just write over the memory which makes these reads redundant, just a note for now */
    uint64_t *block_array = (uint64_t *) buffer;
    for (uint64_t i = num_allocated; i < num_to_allocate + num_allocated; i++) {
        block_array[i] = diosfs_get_free_block_and_mark_bitmap(fs);
    }
    diosfs_write_block_by_number(single_indirect, buffer, fs, 0, fs->superblock->block_size);
    diosfs_write_inode(fs, inode);
    kfree(buffer);
    return single_indirect;
}


