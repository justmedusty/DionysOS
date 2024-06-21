//
// Created by dustyn on 6/21/24.
//

#ifndef KERNEL_PHYSICAL_ALLOCATOR_H
#define KERNEL_PHYSICAL_ALLOCATOR_H

#define KERNEL_START 0xffffffff80000000
#define FREE         0x0
#define USED         0x1
#define ERROR        0xFFFFFFFFFFFFFFFF
#define PAGE_SIZE    0x1000
#define PRE_ALLOC    20

extern uint64 end_kernel;
//may need to worry about alignment, fine for now
typedef struct pageframe_t{
    void *address;
    int8 is_allocated;
    int16 ref_count;
    int8 dirty;
    uint8 protection_flags;
    uint32 pageframe_number;
    struct pageframe *next;
    struct pageframe *prev;
} pageframe_t;
#endif //KERNEL_PHYSICAL_ALLOCATOR_H
