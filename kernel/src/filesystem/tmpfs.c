//
// Created by dustyn on 12/22/24.
//

#include "include/filesystem/tmpfs.h"

#include <include/architecture/arch_asm_functions.h>
#include <include/data_structures/binary_tree.h>
#include <include/definitions/string.h>

static struct tmpfs_node *tmpfs_find_child(struct tmpfs_node *node, char *name);

static struct vnode *insert_tmpfs_children_nodes_into_vnode_children(struct vnode *vnode,
                                                                      const struct tmpfs_directory_entries *entries,
                                                                       size_t num_entries, char *target_name);
static struct tmpfs_node *find_tmpfs_node_from_vnode(struct vnode *vnode);
static void insert_tmpfs_node_into_parent_directory_entries(struct tmpfs_node *node);
static struct tmpfs_node *spawn_new_tmpfs_node(char *name, uint8_t type);

char tmpfs_node_number_bitmap[PAGE_SIZE];

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

struct vnode *tmpfs_lookup(struct vnode *vnode, char *name) {
    if (vnode->is_cached && vnode->vnode_flags & VNODE_CHILD_MEMORY_ALLOCATED) {
        panic("tmpfs_lookup: vnode is cached already");
        // not a huge deal but a logic bug nonetheless so panic for visibility
    }
    struct tmpfs_filesystem_context *tmpfs = vnode->filesystem_object;
    struct tmpfs_node *target_node = lookup_tree(&tmpfs->node_tree, vnode->vnode_inode_number,false);

    if (target_node == NULL) {
        panic("tmpfs_lookup vnode exists but not corresponding tmpfs node found in tree");
    }

    struct vnode *child = insert_tmpfs_children_nodes_into_vnode_children(
        vnode, &target_node->directory_entries, target_node->tmpfs_node_size, name);
    vnode->num_children = target_node ->tmpfs_node_size;
    return child;
}

struct vnode *tmpfs_create(struct vnode *parent, char *name, uint8_t type) {
    struct vnode *vnode = vnode_alloc();
    memset(vnode, 0, sizeof(*vnode));
    struct tmpfs_node *parent_tmpfs_node = find_tmpfs_node_from_vnode(parent);
    struct tmpfs_node *child = spawn_new_tmpfs_node(name, type);
    child->parent_tmpfs_node = parent_tmpfs_node;
    insert_tmpfs_node_into_parent_directory_entries(child);



}

void tmpfs_rename(const struct vnode *vnode, char *name) {
}

void tmpfs_remove(const struct vnode *vnode) {
}

uint64_t tmpfs_write(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {
}


uint64_t tmpfs_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes) {
}

struct vnode *tmpfs_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type) {
}

void tmpfs_unlink(struct vnode *vnode) {
}
/*
 * Nop for open and close since there's no page cache for this at the time of writing and desigining this.
 * Maybe in the future it will be like linux where it can be paged out and there will be work to do here but
 * as of now this is not the case.
 */
uint64_t tmpfs_open(struct vnode *vnode) {
    nop();
    return 0UL;
}

void tmpfs_close(struct vnode *vnode, uint64_t handle) {
    nop();
}

void tmpfs_mkfs() {

}

static struct tmpfs_node *tmpfs_find_child(struct tmpfs_node *node, char *name) {
    if (node->node_type != DIRECTORY) {
        return NULL;
    }

    struct tmpfs_directory_entries *entries = &node->directory_entries;

    for (size_t i = 0; i < node ->tmpfs_node_size; i++) {
        struct tmpfs_node *entry = entries->entries[i];
        if (safe_strcmp(entry->node_name, name,VFS_MAX_NAME_LENGTH)) {
            return entry;
        }
    }

    return NULL;
}

static struct vnode *tmpfs_node_to_vnode(struct tmpfs_node *node) {
    struct vnode *vnode = vnode_alloc();
    memset(vnode, 0, sizeof(struct vnode));
    vnode->filesystem_object = node->superblock->filesystem;
    vnode->is_cached = false;
    vnode->vnode_filesystem_id = VNODE_FS_TMPFS;
    vnode->vnode_ops = &tmpfs_ops;
    vnode->vnode_size = node ->tmpfs_node_size;
    vnode->vnode_type = node->node_type;
    safe_strcpy(vnode->vnode_name, node->node_name,VFS_MAX_NAME_LENGTH);
    return vnode;
}

static struct vnode *insert_tmpfs_children_nodes_into_vnode_children(struct vnode *vnode,
                                                                      const struct tmpfs_directory_entries *entries,
                                                                      const size_t num_entries, char *target_name) {
    struct vnode *ret = NULL;
    for (size_t i = 0; i < num_entries; i++) {
        vnode->vnode_children[i] = tmpfs_node_to_vnode(entries->entries[i]);
        vnode->vnode_children[i]->vnode_parent = vnode;
        if (safe_strcmp(vnode->vnode_children[i]->vnode_name, target_name, VFS_MAX_NAME_LENGTH)) {
            ret = vnode->vnode_children[i];
        }
    }
    return ret;
}

static struct tmpfs_node *find_tmpfs_node_from_vnode(struct vnode *vnode) {

    struct tmpfs_node *ret = NULL;
    struct tmpfs_filesystem_context *context = vnode->filesystem_object;
    ret = lookup_tree(&context->node_tree, vnode->vnode_inode_number, false);

    if (ret != NULL) {
        return ret;
    }

    panic("tmpfs_node_from_vnode: tmpfs node does not exist");


}

static void insert_tmpfs_node_into_parent_directory_entries(struct tmpfs_node *node) {
    struct tmpfs_node *parent = node->parent_tmpfs_node;
    if (parent->node_type != DIRECTORY) {
        panic("insert_tmpfs_node_into_parent_directory_entries: parent is not a directory");
    }

    parent->directory_entries.entries[parent->tmpfs_node_size] = node;
    parent->tmpfs_node_size++;
}

static struct tmpfs_node *spawn_new_tmpfs_node(char *name, const uint8_t type) {
    struct tmpfs_node *tmpfs_node = kmalloc(sizeof(struct tmpfs_node));
    memset(tmpfs_node, 0, sizeof(struct tmpfs_node));
    safe_strcpy(tmpfs_node->node_name, name, VFS_MAX_NAME_LENGTH);
    tmpfs_node->node_type = type;
    return tmpfs_node;

}