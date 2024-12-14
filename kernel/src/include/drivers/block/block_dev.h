//
// Created by dustyn on 12/13/24.
//

#ifndef BLOCK_DEV_H
#define BLOCK_DEV_H


#define BLOCK_DEV_RAMDISK 0
#define BLOCK_DEV_FLASH 1
#define BLOCK_DEV_DISK 2

struct block_device{
  uint64_t device_type;
  uint64_t device_id;
  uint64_t block_size;
};

#endif //BLOCK_DEV_H
