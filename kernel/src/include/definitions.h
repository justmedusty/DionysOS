//
// Created by dustyn on 8/12/24.
//

#pragma once
#define NULL (void *) 0

#define uint8_t_MAX 0xFF
#define uint16_t_MAX 0xFFFF
#define uint32_t_MAX 0xFFFFFFFF
#define uint64_t_MAX 0xFFFFFFFFFFFFFFFF

//The ID's I'm going to put below are for spinlock contexts in case there are deadlock/contention issues later
#define SERIAL_LOCK 0
#define SMP_BOOSTRAP_LOCK 1
#define DFS_LOCK 2
#define VFS_LOCK 3
#define TEMPFS_LOCK 4 // may need to make multiple but this is fine for now
#define PMM_LOCK 5
#define BTREE_LOCK 6 // there are many of these but its still an insight
#define BUDDY_LOCK 7
#define RAMDISK_LOCK 8