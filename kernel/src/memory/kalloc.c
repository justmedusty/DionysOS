//
// Created by dustyn on 7/6/24.
//
#include "include/types.h"
#include "include/memory/pmm.h"
#include "include/memory/kalloc.h"

#include <include/data_structures/hash_table.h>
#include <include/data_structures/spinlock.h>

#include "include/memory/slab.h"
#include "include/drivers/serial/uart.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_paging.h"

struct spinlock alloc_lock;
/*
 * Init Memory for Kernel Heap.
 * Because 32 bytes is a common needed size so far in this kernel, I have allocated a metric fuckload of slab memory for it (comparatively)
 */
int heap_init() {
    initlock(&alloc_lock, ALLOC_LOCK);
    int size = 8;
    for (uint64_t i = 0; i < NUM_SLABS; i++) {
            heap_create_slab(&slabs[i],size,DEFAULT_SLAB_SIZE);
        size <<= 1;
        if(size >= PAGE_SIZE){
            break;
        }
    }

    serial_printf("Kernel Heap Initialized\n");
    return 0;
}

/*
 * kalloc tries to allocate from the cached slab memory wherever it can, otherwise it just invokes
 * physalloc. When physalloc is invoked, the start of the memory is changed to the proper HHDM value.
 */

/*
 *TODO fix nested dependency issue rendering the allocator not lockable
 *Possible fix is an even higher level wrapper function, a check to see if this
 *thread of control is holding the lock and allowing them through, having per cpu
 *buddy pools etc.
 */

// Going to entertain a higher level wrapper function for locking for the time being. If this works fine I will leave it otherwise I will
// revisit for a more robust locking scheme
void *kmalloc(uint64_t size) {
    acquire_spinlock(&alloc_lock);
    void *ret = _kalloc(size);
    release_spinlock(&alloc_lock);
    return ret;
}

void *kzmalloc(uint64_t size) {
    acquire_spinlock(&alloc_lock);
    void *ret = _kalloc(size);
    memset(ret, 0, size); // I am sure this is not the "official" way to do a kzmalloc you would have idle kthreads zeroing pages in a pool but this is fine for now
    release_spinlock(&alloc_lock);
    return ret;
}
void *_kalloc(uint64_t size) {

    if(size < PAGE_SIZE) {
        struct slab *slab = heap_slab_for(size);
        if (slab != NULL) {
            return heap_allocate_from_slab(slab);
        }
    }


    uint64_t page_count = (size + (PAGE_SIZE)) / PAGE_SIZE;
    void *return_value = P2V(phys_alloc(page_count));

    if (return_value == NULL) {
        return NULL;
    }
    return return_value;
}
/*
 * Krealloc is just realloc for the kernel. Allocate a bigger block of memory, copy the previous contents in.
 * I will use this as a soapbox opportunity to say that realloc is not a safe function in a secure context. You have no
 * way to know if the memory that just deallocated was erased if there was sensitive data in it. That is always something
 * that needs to be kept in mind and it is best to avoid realloc and to use malloc and zero yourself.
 */
void *krealloc(void *address, uint64_t new_size) {
    acquire_spinlock(&alloc_lock);
    if (address == NULL) {
        return _kalloc(new_size);
    }

    if (((uint64_t) address & 0xFFF) == 0) {
        struct metadata *metadata = (struct metadata *) (address - PAGE_SIZE);
        if (((metadata->size + (PAGE_SIZE - 1)) / PAGE_SIZE) == ((new_size + (PAGE_SIZE - 1)) / PAGE_SIZE)) {
            metadata->size = new_size;
            release_spinlock(&alloc_lock);
            return address;
        }

        void *new_address =_kalloc(new_size);
        if (new_address == NULL) {
            release_spinlock(&alloc_lock);
            return NULL;
        }

        if (metadata->size > new_size) {
            memcpy(new_address, address, new_size);
        } else {
            memcpy(new_address, address, metadata->size);
        }

        _kfree(address);
        release_spinlock(&alloc_lock);
        return new_address;
    }

    struct header *slab_header = (struct header *) ((uint64_t) address & ~0xFFF);
    struct slab *slab = slab_header->slab;

    if (new_size > slab->entry_size) {
        void *new_address =_kalloc(new_size);
        if (new_address == NULL) {
            return NULL;
        }

        memcpy(new_address, address, slab->entry_size);
        heap_free_in_slab(slab, address);
        release_spinlock(&alloc_lock);
        return new_address;
    }
    release_spinlock(&alloc_lock);
    return address;
}

/*
 * _kfree( frees kernel memory, if it is a multiple of page size, ie any bits in 0xFFF, then it is freed from the slab cache. Otherwise phys_dealloc is invoked.
 */
void _kfree(void* address) {
    if (address == NULL) {
        return;
    }

    if ((uint64_t)address & 0xFFF) {
        goto slab;
    }

    for (uint64_t i = 0; i < NUM_SLABS; i++) {
        struct header* slab_header = (struct header*)((uint64_t)address & ~((DEFAULT_SLAB_SIZE * PAGE_SIZE) - 1));
        if (slab_header && slab_header->slab && slabs[i].start_address == slab_header) {
            goto slab;
        }
    }

    phys_dealloc(V2P(address));
    return;


slab:

    struct header* slab_header = (struct header*)((uint64_t)address & ~((DEFAULT_SLAB_SIZE * PAGE_SIZE) - 1));

    heap_free_in_slab(slab_header->slab, address);
}

void kfree(void *address) {
    acquire_spinlock(&alloc_lock);
    _kfree(address);
    release_spinlock(&alloc_lock);
}

