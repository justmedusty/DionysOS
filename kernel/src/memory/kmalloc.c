//
// Created by dustyn on 7/6/24.
//
#include "include/definitions/types.h"
#include "include/memory/pmm.h"
#include "include/memory/kalloc.h"

#include <include/data_structures/hash_table.h>
#include <include/data_structures/spinlock.h>
#include <include/drivers/display/framebuffer.h>

#include "include/memory/slab.h"
#include "include/drivers/serial/uart.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_paging.h"

struct spinlock alloc_lock;
struct spinlock userlock;
/*
 * Init Memory for Kernel Heap.
 * Because 32 bytes is a common needed size so far in this kernel, I have allocated a metric fuckload of slab memory for it (comparatively)
 */
int heap_init() {
    kprintf("Initializing Kernel Heap...\n");
    initlock(&alloc_lock, ALLOC_LOCK);
    initlock(&userlock, ALLOC_LOCK);
    int size = 8;
    for (uint64_t i = 0; i < NUM_SLABS; i++) {
        heap_create_slab(&slabs[i], size,DEFAULT_SLAB_SIZE_PAGES);
        size <<= 1;
        if (size >= PAGE_SIZE) {
            break;
        }
    }

    serial_printf("Kernel Heap Initialized\n");
    kprintf("Kernel Heap Initialized\n");
    return 0;
}

/*
 * kalloc tries to allocate from the cached slab memory wherever it can, otherwise it just invokes
 * physalloc. When physalloc is invoked, the start of the memory is changed to the proper HHDM value.
 */

/*
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

void *umalloc(uint64_t pages) {
    acquire_spinlock(&userlock);
    void *return_value = Phys2Virt(phys_alloc(pages,USER_POOL));

    if (return_value == NULL) {
        release_spinlock(&userlock);
        return NULL;
    }
    release_spinlock(&userlock);
    return return_value;
}

/*
 * Zeros memory before passing it to you, generally there would be some sort of zero'd page cache but we don't need to go that far
 * with a hobby operating system. Just zero on demand.
 */
void *kzmalloc(uint64_t size) {
    acquire_spinlock(&alloc_lock);
    void *ret = _kalloc(size);
    memset(ret, 0, size);
    // I am sure this is not the "official" way to do a kzmalloc you would have idle kthreads zeroing pages in a pool but this is fine for now
    release_spinlock(&alloc_lock);
    return ret;
}

/*
 * These two are lockless alloc functions which the main alloc functions wrap around. This allows us to alloc from inside a
 * locked context and not worry about deadlocking yourself. Without these deadlock will occur very quickly whenever new bookkeeping
 * objects are allocated during an allocation call chain. This happens a lot.
 */
void *_kalloc(uint64_t size) {
    if (size < PAGE_SIZE) {
        struct slab *slab = heap_slab_for(size);
        if (slab != NULL) {
            void *addr = heap_allocate_from_slab(slab);
            return addr;
        }
    }


    uint64_t page_count = (size + (PAGE_SIZE)) / PAGE_SIZE;
    void *return_value = Phys2Virt(phys_alloc(page_count,KERNEL_POOL));

    if (return_value == NULL) {
        return NULL;
    }
    return return_value;
}

/*
 * _kfree( frees kernel memory, if it is a multiple of page size, ie any bits in 0xFFF, then it is freed from the slab cache. Otherwise, phys_dealloc is invoked.
 */
void _kfree(void *address) {
    if (address == NULL) {
        return;
    }

    if ((uint64_t) address & 0xFFF) {
        goto slab;
    }

    phys_dealloc(Virt2Phys(address));
    return;

slab:
    struct header *slab_header = (struct header *) ((uint64_t) address & ~((DEFAULT_SLAB_SIZE_PAGES * PAGE_SIZE) - 1));
    heap_free_in_slab(slab_header->slab, address);
}

/*
 * Krealloc is just realloc for the kernel. Allocate a bigger block of memory, copy the previous contents in.
 * I will use this as a soapbox opportunity to say that realloc is not a safe function in a secure context. You have no
 * way to know if the memory that just deallocated was erased if there was sensitive data in it. That is always something
 * that needs to be kept in mind, and it is best to avoid realloc and to use malloc and zero yourself.
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

        void *new_address = _kalloc(new_size);
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
        void *new_address = _kalloc(new_size);
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


void kfree(void *address) {
    acquire_spinlock(&alloc_lock);
    _kfree(address);
    release_spinlock(&alloc_lock);
}

