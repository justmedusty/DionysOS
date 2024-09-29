//
// Created by dustyn on 9/18/24.
//
#include "include/filesystem/tempfs.h"
#include "include/data_structures/spinlock.h"
#include "include/definitions.h"

struct spinlock tempfs_lock;


void tempfs_init() {
  initlock(&tempfs_lock,TEMPFS_LOCK);
};
