//
// Created by dustyn on 12/22/24.
//

#include "include/filesystem/tmpfs.h"

#include <include/data_structures/binary_tree.h>
#include <include/definitions/string.h>

static struct tmpfs_node *tmpfs_find_child(struct tmpfs_node *node, char *name);

static struct vnode *insert_tempfs_children_nodes_into_vnode_children(struct vnode *vnode,
                                                                      const struct tmpfs_directory_entries *entries,
                                                                      const size_t num_entries, char *target_name);

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
    struct tmpfs_filesystem_object *tmpfs = vnode->filesystem_object;
    struct tmpfs_node *target_node = lookup_tree(&tmpfs->node_tree, vnode->vnode_inode_number,false);

    if (target_node == NULL) {
        panic("tmpfs_lookup vnode exists but not corresponding tmpfs node found in tree");
    }

    struct vnode *child = insert_tempfs_children_nodes_into_vnode_children(
        vnode, &target_node->directory_entries, target_node->t_node_size, name);
    vnode->num_children = target_node->t_node_size;
    return child;
}

struct vnode *tmpfs_create(struct vnode *parent, char *name, uint8_t type) {
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

uint64_t tmpfs_open(struct vnode *vnode) {
}

void tmpfs_close(struct vnode *vnode, uint64_t handle) {
}

void tmpfs_mkfs() {
}

static struct tmpfs_node *tmpfs_find_child(struct tmpfs_node *node, char *name) {
    if (node->node_type != DIRECTORY) {
        return NULL;
    }

    struct tmpfs_directory_entries *entries = &node->directory_entries;

    for (size_t i = 0; i < node->t_node_size; i++) {
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
    vnode->vnode_size = node->t_node_size;
    vnode->vnode_type = node->node_type;
    safe_strcpy(vnode->vnode_name, node->node_name,VFS_MAX_NAME_LENGTH);
    return vnode;
}

static struct vnode *insert_tempfs_children_nodes_into_vnode_children(struct vnode *vnode,
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
