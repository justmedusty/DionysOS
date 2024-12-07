//
// Created by dustyn on 9/29/24.
//

#pragma once
#include "include/types.h"
#include "include/scheduling/process.h"
#include "include/filesystem/tempfs.h"
#include "include/filesystem/ext2.h"
#include "include/filesystem/vfs.h"

#define ROOT_INODE 0
#define CURRENT_DIRECTORY_NAME "."
#define PARENT_DIRECTORY_NAME ".."

#define ROOT_DIRECTORY_NAME "/"


struct file_handle {
      uint32_t handle;
      uint64_t offset;
      struct vnode *file_vnode;
      struct process *process;
}

