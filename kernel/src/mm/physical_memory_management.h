//
// Created by dustyn on 6/21/24.
//

#ifndef KERNEL_PHYSICAL_ALLOCATOR_H
#define KERNEL_PHYSICAL_ALLOCATOR_H

#define KERNEL_START 0xffffffff80000000;
extern uint64 end_kernel;
//may need to worry about alignment, fine for now
struct pageframe_t{
    void *address;
    int8 is_allocated;
    int16 ref_count;
    int8 dirty;
    uint8 protection_flags;
    int pageframe_number;
    struct pageframe *next;
    struct pageframe *prev;
};
#endif //KERNEL_PHYSICAL_ALLOCATOR_H
