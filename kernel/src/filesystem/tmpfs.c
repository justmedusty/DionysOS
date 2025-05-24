
//
// Created by dustyn on 12/22/24.
//

#include "include/filesystem/tmpfs.h"
#include <include/architecture/arch_asm_functions.h>
#include <include/data_structures/binary_tree.h>
#include <include/definitions/string.h>
#include <include/drivers/display/framebuffer.h>

static struct vnode *tmpfs_node_to_vnode(struct tmpfs_node *node);

static struct tmpfs_node *tmpfs_find_child(struct tmpfs_node *node, char *name);

static struct vnode *insert_tmpfs_children_nodes_into_vnode_children(struct vnode *vnode,
                                                                     const struct tmpfs_directory_entries *entries,
                                                                     size_t num_entries, char *target_name);

static void tmpfs_remove_all_data_pages(const struct tmpfs_node *node);

static struct tmpfs_node *find_tmpfs_node_from_vnode(const struct vnode *vnode);

static void insert_tmpfs_node_into_parent_directory_entries(struct tmpfs_node *node);

static struct tmpfs_node *conjure_new_tmpfs_node(char *name, uint8_t type);

static void tmpfs_node_bitmap_free(uint64_t node_number);

static uint64_t tmpfs_node_bitmap_get();

static void free_page_list_entry(struct tmpfs_page_list_entry *entry);

static struct tmpfs_page_list_entry *tmpfs_find_page_list_entry(const struct tmpfs_node *node,
                                                                uint64_t page_list_entry_number);

static void tmpfs_delete_directory_recursively(struct tmpfs_node *node);

static void tmpfs_delete_reg_file(struct tmpfs_node *node);

static void tmpfs_remove_dirent_in_parent_directory(const struct tmpfs_node *node_to_remove);

uint8_t tmpfs_node_number_bitmap[PAGE_SIZE] = {0}; // only 1 since we won't support multiple distinct tmpfs filesystems
/*
 * Store the procfs root so when I implement it's usage later we can access it directly without worrying where it is
 */
struct vnode *procfs_root = NULL;
struct vnode *kernel_message = NULL;
bool procfs_online = false;

#define TMPFS_NUM_SUPERBLOCKS 10
struct tmpfs_superblock superblock[TMPFS_NUM_SUPERBLOCKS] = {0};

struct vnode_operations tmpfs_ops = {
        .close = tmpfs_close,
        .create = tmpfs_create,
        .link = tmpfs_link,
        .lookup = tmpfs_lookup,
        .open = tmpfs_open,
        .read = tmpfs_read,
        .remove = tmpfs_remove,
        .rename = tmpfs_rename,
        .unlink = tmpfs_unlink,
        .write = tmpfs_write
};

/*
 * lookup a node inside the directory passed as the vnode argument
 */
struct vnode *tmpfs_lookup(struct vnode *vnode, char *name) {
    struct tmpfs_filesystem_context *context = vnode->filesystem_object;
    acquire_spinlock(&context->fs_lock);

    if (vnode->is_cached && vnode->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED) {
        panic("tmpfs_lookup: vnode is cached already");
        // not a huge deal but a logic bug nonetheless so panic for visibility
    }
    const struct tmpfs_filesystem_context *tmpfs = vnode->filesystem_object;
    struct tmpfs_node *target_node = lookup_tree(&tmpfs->superblock->node_tree, vnode->vnode_inode_number, false);

    if (target_node == NULL) {
        panic("tmpfs_lookup vnode exists but not corresponding tmpfs node found in tree");
        return NULL; // linter purposes
    }
    struct tmpfs_node *child_node = tmpfs_find_child(target_node, name);


    struct vnode *child = insert_tmpfs_children_nodes_into_vnode_children(
            vnode, &target_node->directory_entries, target_node->tmpfs_node_size, name);
    vnode->num_children = target_node->tmpfs_node_size;
    release_spinlock(&context->fs_lock);
    return child;
}

/*
 * Create a new tmpfs node , the parent parameter is the parent directory.
 */
struct vnode *tmpfs_create(struct vnode *parent, char *name, uint8_t type) {

    struct tmpfs_filesystem_context *context = parent->filesystem_object;
    acquire_spinlock(&context->fs_lock);

    struct tmpfs_node *parent_tmpfs_node = find_tmpfs_node_from_vnode(parent);
    struct tmpfs_node *child = conjure_new_tmpfs_node(name, type);

    child->tmpfs_node_number = tmpfs_node_bitmap_get();
    child->parent_tmpfs_node = parent_tmpfs_node;
    child->superblock = parent_tmpfs_node->superblock;
    child->superblock->tmpfs_node_count++;
    insert_tree_node(&child->superblock->node_tree, child, child->tmpfs_node_number);
    insert_tmpfs_node_into_parent_directory_entries(child);

    release_spinlock(&context->fs_lock);
    return tmpfs_node_to_vnode(child);
}

/*
 * Rename a given tmpfs_node
 */
void tmpfs_rename(const struct vnode *vnode, char *name) {
    struct tmpfs_filesystem_context *context = vnode->filesystem_object;
    acquire_spinlock(&context->fs_lock);
    struct tmpfs_node *tmpfs_node = find_tmpfs_node_from_vnode(vnode);
    safe_strcpy(tmpfs_node->node_name, name, VFS_MAX_NAME_LENGTH);
    release_spinlock(&context->fs_lock);
}

void tmpfs_remove(const struct vnode *vnode) {
    struct tmpfs_filesystem_context *context = vnode->filesystem_object;
    acquire_spinlock(&context->fs_lock);
    struct tmpfs_node *node = find_tmpfs_node_from_vnode(vnode);


    switch (vnode->vnode_type) {
        case VNODE_FILE:
            tmpfs_delete_reg_file(node);
            tmpfs_remove_dirent_in_parent_directory(node);
            kfree(node);
            release_spinlock(&context->fs_lock);
            return;

        case VNODE_DIRECTORY :
            tmpfs_delete_directory_recursively(node);
            tmpfs_remove_dirent_in_parent_directory(node);
            kfree(node);
            release_spinlock(&context->fs_lock);
            return;

        case VNODE_SYM_LINK:
            kfree(node->sym_link_path.path);
            tmpfs_remove_dirent_in_parent_directory(node);
            kfree(node);
            release_spinlock(&context->fs_lock);
            return;
    }


    panic("tmpfs_remove: unknown vnode type");
}


/*
 * Write bytes to a tmpfs regular file , update size if necessary and allocate new page_list_entries when
 * it is required to do so.
 */
int64_t tmpfs_write(struct vnode *vnode, uint64_t offset, const char *buffer, uint64_t bytes) {
    DEBUG_PRINT("VNODE %s OFFSET %i BYTES %i SIZE %i\n",vnode->vnode_name,offset,bytes,vnode->vnode_size);
    struct tmpfs_filesystem_context *context = vnode->filesystem_object;
    acquire_spinlock(&context->fs_lock);
    struct tmpfs_node *node = find_tmpfs_node_from_vnode(vnode);
    uint64_t page = offset > PAGE_SIZE ? offset / PAGE_SIZE : 0;
    offset = offset % PAGE_SIZE;
    const uint64_t total_bytes = bytes;
    uint64_t copy_index = 0;
    struct tmpfs_page_list_entry *target = tmpfs_find_page_list_entry(node, page / PAGES_PER_TMPFS_ENTRY);

    while (bytes > 0) {
        bool new_size = false;
        char *data_page = (char *) target->page_list[page / PAGES_PER_TMPFS_ENTRY];
        if( buffer[copy_index] == '\0'){
            panic("null term");
        }
        data_page[offset] = buffer[copy_index];

        copy_index++;
        offset = offset + 1 == PAGE_SIZE ? 0 : offset + 1;
        bytes--;

        if (((page * PAGE_SIZE) + offset) > node->tmpfs_node_size) {
            node->tmpfs_node_size += ((page * PAGE_SIZE) + offset) - node->tmpfs_node_size;
            new_size = true;
        }

        if (offset == 0) {
            if (new_size) {
                target->number_of_pages++;
            }
            page++;
            if (page / PAGES_PER_TMPFS_ENTRY == 0) {
                target = tmpfs_find_page_list_entry(node, page / PAGES_PER_TMPFS_ENTRY);
            }
        }
    }

    vnode->vnode_size += total_bytes - bytes;
    DEBUG_PRINT("NODE SIZE %i VNODE SIZE %i\n",node->tmpfs_node_size,vnode->vnode_size);
    DEBUG_PRINT("VNODE ADDRESS %x.64\n",vnode);
    release_spinlock(&context->fs_lock);
    return (int64_t)(total_bytes - bytes);
}

/*
 * Read bytes from a tmpfs regular file. The meat and potatoes of this is just finding the page_list_entry with the page
 * in it and from there simply indexing into the page pointer array.
 *
 */
int64_t tmpfs_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {
    struct tmpfs_filesystem_context *context = vnode->filesystem_object;
    acquire_spinlock(&context->fs_lock);
    struct tmpfs_node *node = find_tmpfs_node_from_vnode(vnode);
    uint64_t page = offset > PAGE_SIZE ? offset / PAGE_SIZE : 0;
    offset = offset % PAGE_SIZE;
    const uint64_t total_bytes = bytes;
    uint64_t copy_index = 0;
    struct tmpfs_page_list_entry *target = tmpfs_find_page_list_entry(node, page / PAGES_PER_TMPFS_ENTRY);

    while (bytes > 0) {
        char *data_page = (char *) target->page_list[page / PAGES_PER_TMPFS_ENTRY];
        buffer[copy_index] = data_page[offset];

        copy_index++;
        offset = offset + 1 == PAGE_SIZE ? 0 : offset + 1;
        bytes--;

        if (offset == 0) {
            page++;
            if (page / PAGES_PER_TMPFS_ENTRY == 1) {
                target = tmpfs_find_page_list_entry(node, page / PAGES_PER_TMPFS_ENTRY);
            }
        }
    }
    release_spinlock(&context->fs_lock);
    uint64_t result = total_bytes - bytes;

    return (int64_t) result; // the amount of bytes you would have to write for this to be a problem is so gargantuan that this is ok in my eyes
}

struct vnode *tmpfs_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type) {
    return NULL;
}

void tmpfs_unlink(struct vnode *vnode) {
    nop();
}


/*
 * Nop for open and close since there's no page cache for this at the time of writing and designing this.
 * Maybe in the future it will be like linux where it can be paged out and there will be work to do here but
 * as of now this is not the case.
 */
int64_t tmpfs_open(struct vnode *vnode) {
    nop();
    return 0;
}

void tmpfs_close(struct vnode *vnode, uint64_t handle) {
    nop();
}

struct spinlock kernel_message_lock;
void log_kernel_message(const char *message) {
    if (procfs_online) {
        acquire_spinlock(&kernel_message_lock);
        uint64_t len = strlen(message);
        uint64_t offset = kernel_message->vnode_size;
        vnode_write(kernel_message, offset, len, message);
        DEBUG_PRINT("VNODE SIZE %i OFFSET %i LEN %i\n",kernel_message->vnode_size,offset,len);
        release_spinlock(&kernel_message_lock);
    }
}

void tmpfs_mkfs(const uint64_t filesystem_id, char *directory_to_mount_onto) {
    kprintf("Creating tmpfs filesystem...\n");
    initlock(&kernel_message_lock,KERNEL_MESSAGE_LOCK);
    if (filesystem_id > TMPFS_NUM_SUPERBLOCKS) {
        warn_printf("tmpfs_mkfs: filesystem_id > TMPFS_NUM_SUPERBLOCKS\n");
        return;
    }
    struct vnode *vnode_to_be_mounted = vnode_lookup(directory_to_mount_onto);
    if (vnode_to_be_mounted == NULL) {
        warn_printf("Path passed to tmpfs_mkfs does not return a valid vnode!\n");
        return;
    }

    struct tmpfs_node *root = kzmalloc(sizeof(struct tmpfs_node));

    root->superblock = &superblock[filesystem_id];
    struct tmpfs_filesystem_context *context = kzmalloc(sizeof(struct tmpfs_filesystem_context));
    context->superblock = root->superblock;
    root->superblock->tmpfs_node_count = 0;
    root->superblock->filesystem = context;
    root->superblock->magic = TMPFS_MAGIC;
    root->superblock->tmpfs_node_count = 1;
    init_tree(&root->superblock->node_tree, REGULAR_TREE, 0);

    root->node_type = VNODE_DIRECTORY;
    root->directory_entries.entries = kmalloc(DIRECTORY_ENTRY_ARRAY_SIZE);
    root->parent_tmpfs_node = NULL;
    root->tmpfs_node_number = tmpfs_node_bitmap_get();
    insert_tree_node(&root->superblock->node_tree, root, root->tmpfs_node_number);
    safe_strcpy(root->node_name, "tmpfs", VFS_MAX_NAME_LENGTH);
    struct vnode *tmpfs_root = tmpfs_node_to_vnode(root);

    serial_printf("TMPFS: Created tmpfs root directory\n");

    vnode_mount(vnode_to_be_mounted, tmpfs_root);

    serial_printf("TMPFS: Mounted tmpfs onto %s\n", directory_to_mount_onto);

    struct vnode *procfs = vnode_create(directory_to_mount_onto, "procfs", VNODE_DIRECTORY);

    DEBUG_PRINT("PROC FS PARENT %s\n",procfs->vnode_parent->vnode_name);

    serial_printf("TMPFS: Created procfs directory\n");
    char *path = vnode_get_canonical_path(procfs);
    DEBUG_PRINT("PROCFS PATH %s\n",path);
    struct vnode *sched = vnode_create(path, "sched", VNODE_DIRECTORY);
    serial_printf("TMPFS: Created sched directory under procfs\n");
    struct vnode *kernel_messages = vnode_create(path, "kernel_messages", VNODE_FILE);

    serial_printf("TMPFS: Created kernel_messages file under procfs\n");
    struct vnode *tmp = vnode_create(directory_to_mount_onto, "tmp", VNODE_DIRECTORY);
    serial_printf("TMPFS: Created tmp directory under %s\n", directory_to_mount_onto);
    kprintf("Tmpfs filesystem created.\n");
    kfree(path);
    procfs_root = procfs;
    kernel_message = kernel_messages;

    tmpfs_root->is_cached = true;
    sched->is_cached = true;
    procfs->is_cached = true;
    tmp->is_cached = true;
    procfs_online = true;
    kprintf("Tmpfs Initialized\n");
    log_kernel_message("Tmpfs initialized.\n");
}

/*
 * Finds a given node where the sought-after page is found
 * If the node is not found this means we are reaching a new area and will allocate a new page list entry, allocating the first page as a courtesy
 */
static struct tmpfs_page_list_entry *tmpfs_find_page_list_entry(const struct tmpfs_node *node,
                                                                uint64_t page_list_entry_number) {
    struct doubly_linked_list_node *d_node = node->page_list->head;

    while (d_node != NULL && page_list_entry_number != 0) {
        d_node = d_node->next;
        page_list_entry_number--;
    }
    /*
     *  If it is null this should mean we are reaching outside a page list boundary and need to allocate a new entry
     */
    if (d_node == NULL) {
        struct tmpfs_page_list_entry *entry = kmalloc(sizeof(struct tmpfs_page_list_entry));
        entry->page_list = kmalloc(sizeof(struct tmpfs_page_list_entry *) * PAGES_PER_TMPFS_ENTRY);
        memset(entry->page_list, 0, sizeof(struct tmpfs_page_list_entry *) * PAGES_PER_TMPFS_ENTRY);
        entry->page_list[0] = kmalloc(PAGE_SIZE);
        entry->number_of_pages++;
        doubly_linked_list_insert_tail(node->page_list, entry);
        return entry;
    }

    return d_node->data;
}

/*
 * Find a child node inside the given tmpfs directory
 */
static struct tmpfs_node *tmpfs_find_child(struct tmpfs_node *node, char *name) {
    if (node->node_type != VNODE_DIRECTORY) {
        return NULL;
    }

    struct tmpfs_directory_entries *entries = &node->directory_entries;

    for (size_t i = 0; i < node->tmpfs_node_size; i++) {
        struct tmpfs_node *entry = entries->entries[i];
        if (safe_strcmp(entry->node_name, name, VFS_MAX_NAME_LENGTH)) {
            return entry;
        }
    }

    return NULL;
}

/*
 * Simply creates a vnode with the attributes from a given tmpfs node
 */
static struct vnode *tmpfs_node_to_vnode(struct tmpfs_node *node) {
    struct vnode *vnode = vnode_alloc();
    DEBUG_PRINT("NODE NAME %s\n",node->node_name);
    memset(vnode, 0, sizeof(struct vnode));
    vnode->filesystem_object = node->superblock->filesystem;
    vnode->is_cached = false;
    vnode->vnode_filesystem_id = VNODE_FS_TMPFS;
    vnode->vnode_ops = &tmpfs_ops;
    vnode->vnode_size = node->tmpfs_node_size;
    vnode->vnode_type = node->node_type;
    vnode->vnode_inode_number = node->tmpfs_node_number;
    safe_strcpy(vnode->vnode_name, node->node_name, VFS_MAX_NAME_LENGTH);
    return vnode;
}

/*
 * Fill in the vnode children, this sort of thing happens when you do not create a filesystem from scratch on boot
 * and instead load from an image or something along these lines. In this case the vnodes may not exist yet and the VFS
 * needs to be set up from reading the filesystem
 */
static struct vnode *insert_tmpfs_children_nodes_into_vnode_children(struct vnode *vnode,
                                                                     const struct tmpfs_directory_entries *entries,
                                                                     const size_t num_entries, char *target_name) {
    struct vnode *ret = NULL;
    for (size_t i = 0; i < num_entries; i++) {
        vnode->vnode_children[i] = tmpfs_node_to_vnode(entries->entries[i]);
        vnode->vnode_children[i]->vnode_parent = vnode;
        DEBUG_PRINT("ENTRY NAME %s ADDR %x.64 SIZE %i\n",vnode->vnode_children[i]->vnode_name,vnode->vnode_children[i],vnode->vnode_children[i]->vnode_size);
        if (safe_strcmp(vnode->vnode_children[i]->vnode_name, target_name, VFS_MAX_NAME_LENGTH)) {
            ret = vnode->vnode_children[i];
        }
    }

    return ret;
}

/*
 * Uses a vnode to find a tmpfs_node via a binary search of the superblock node tree
 */
static struct tmpfs_node *find_tmpfs_node_from_vnode(const struct vnode *vnode) {
    struct tmpfs_node *ret = NULL;
    const struct tmpfs_filesystem_context
            *context = vnode->filesystem_object;
    ret = lookup_tree(&context->superblock->node_tree, vnode->vnode_inode_number, false);
    if (ret == NULL) {
        /*
         * A state in which a vnode exists for a tmpfs object but no object exists underneath is invalid and should
         * cause a kernel panic so I can see it and investigate
         */
        panic("tmpfs_node_from_vnode: tmpfs node does not exist");
    }
    return ret;
}

/*
 * When a node is created this function inserts the pointer to the node in the parents entry array
 */
static void insert_tmpfs_node_into_parent_directory_entries(struct tmpfs_node *node) {
    struct tmpfs_node *parent = node->parent_tmpfs_node;
    if (parent->node_type != VNODE_DIRECTORY) {
        panic("insert_tmpfs_node_into_parent_directory_entries: parent is not a directory");
    }

    parent->directory_entries.entries[parent->tmpfs_node_size] = node;
    parent->tmpfs_node_size++;
}

/*
 * Creates a new node object and initializes the type specific field
 */
static struct tmpfs_node *conjure_new_tmpfs_node(char *name, const uint8_t type) {
    struct tmpfs_node *tmpfs_node = kmalloc(sizeof(struct tmpfs_node));
    memset(tmpfs_node, 0, sizeof(struct tmpfs_node));
    safe_strcpy(tmpfs_node->node_name, name, VFS_MAX_NAME_LENGTH);
    tmpfs_node->node_type = type;

    switch (type) {
        case VNODE_DIRECTORY:
            tmpfs_node->directory_entries.entries = kmalloc(sizeof(uintptr_t) * MAX_TMPFS_ENTRIES);
            break;
        case VNODE_FILE:
            tmpfs_node->page_list = kmalloc(sizeof(struct doubly_linked_list));
            doubly_linked_list_init(tmpfs_node->page_list);
            break;
        case VNODE_SYM_LINK:
            break;
        default:
            panic("conjure_new_tmpfs_node: unknown type");
    }

    return tmpfs_node;
}

/*
 * Get a new tnode number and free one respectively
 */
static uint64_t tmpfs_node_bitmap_get() {
    for (size_t i = 0; i < PAGE_SIZE; i++) {
        if (tmpfs_node_number_bitmap[i] != 0xFF) {
            for (size_t j = 0; j < 8; j++) {
                if (!(tmpfs_node_number_bitmap[i] & BIT(j))) {
                    tmpfs_node_number_bitmap[i] |= BIT(j);
                    return (i * 8) + j;
                }
            }
        }
    }
    panic("tmpfs_node_bitmap_get: tmpfs nodes maxed out"); // panic is extreme but I want to know when this happens
}

static void tmpfs_node_bitmap_free(const uint64_t node_number) {
    const uint64_t bit = node_number % 8;
    tmpfs_node_number_bitmap[BYTE(node_number)] &= ~BIT(bit);
}

/*
 * Bottom two functions are for clearing out the list of page_list entries
 */
static void tmpfs_remove_all_data_pages(const struct tmpfs_node *node) {
    uint64_t num_page_list_entries = (node->tmpfs_node_size / (PAGE_SIZE * PAGES_PER_TMPFS_ENTRY)) + 1;
    //plus one since loop breaks on 0
    struct doubly_linked_list_node *head = node->page_list->head;
    while (num_page_list_entries > 0) {
        free_page_list_entry(head->data);
        doubly_linked_list_remove_head(node->page_list);
        head = head->next;
        num_page_list_entries--;
    }
    kfree(node->page_list);
}
//TODO check this out make sure this isn't trying to access out of bounds memory, I remember testing this but looks sus
static void free_page_list_entry(struct tmpfs_page_list_entry *entry) {
    for (size_t i = entry->number_of_pages; i > 0; i--) {
        kfree(entry->page_list[i]);
    }
    kfree(entry->page_list);
    kfree(entry);
}

/*
 * Removes and shifts the node being deleted in the parents entry list to fill its spot, we dont want NULL
 * entries in the array because we only use the size to traverse the array. Must not have any gaps
 */
static void tmpfs_remove_dirent_in_parent_directory(const struct tmpfs_node *node_to_remove) {
    struct tmpfs_node *parent = node_to_remove->parent_tmpfs_node;

    uint64_t index = 0;
    const struct tmpfs_node *current_node = parent->directory_entries.entries[index];

    while (current_node && current_node != node_to_remove) {
        current_node++;
        index++;
    }

    if (current_node == NULL) {
        panic("tmpfs_remove_dirent_in_parent_directory: node not found , invalid state");
    }

    parent->directory_entries.entries[index] = NULL;
    parent->directory_entries.entries[index] = parent->directory_entries.entries[parent->tmpfs_node_size - 1];
    // -1 since it is 1 indexed vs 0 indexed array
    parent->tmpfs_node_size--;
}

/*
 * Delete a regular file, first remove all data pages, remove it from the node tree, and then free the node number from the
 * bitmap entry
 */
static void tmpfs_delete_reg_file(struct tmpfs_node *node) {
    tmpfs_remove_all_data_pages(node);
    remove_tree_node(&node->superblock->node_tree, node->tmpfs_node_number, node, NULL);
    tmpfs_node_bitmap_free(node->tmpfs_node_number);
}

/*
 * Recursively delete a directory. Deletes everything within each directory or just deletes each
 * reg file if the current target is a regular file
 */
static void tmpfs_delete_directory_recursively(struct tmpfs_node *node) {
    for (size_t i = 0; i < node->tmpfs_node_size; i++) {
        struct tmpfs_node *current = node->directory_entries.entries[i];

        remove_tree_node(&node->superblock->node_tree, node->tmpfs_node_number, node, NULL);
        tmpfs_node_bitmap_free(node->tmpfs_node_number);

        switch (current->node_type) {
            case VNODE_DIRECTORY:
                tmpfs_delete_directory_recursively(current);
                break;
            case VNODE_FILE:
                tmpfs_delete_reg_file(current);
                break;
            default:
                panic("tmpfs_delete_directory_recursively: invalid node type");
        }
    }
}
