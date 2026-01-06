//
// Created by dustyn on 9/14/24.
//

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "include/filesystem/vfs.h"
#include <include/memory/kmalloc.h>
#include "include/architecture/arch_cpu.h"
#include "include/architecture/arch_vmm.h"
#include "include/scheduling/process.h"


/*
 *	Process states
 */

enum process_state {
    PROCESS_RUNNING,
    PROCESS_SLEEPING,
    PROCESS_DEAD,
    DEBUG_STATE,
    PROCESS_READY
};


/*
 *  Process types
 */
enum process_type {
    KERNEL_THREAD,
    USER_THREAD,
    USER_PROCESS
};


//2 pages
#define DEFAULT_STACK_SIZE 0x2000ULL

struct process {
    uint8_t current_state;
    uint8_t priority;
    uint8_t effective_priority; // Going to use a base priority and an effective priority similar to the real and effective uid idea in linux so that you can promote processes who are being passed over.
    uint16_t process_id;
    uint16_t parent_process_id;
    uint64_t signal; /* This will probably end up being some sort of queue, but I will put this here for now */
    uint64_t signal_mask;
    uint64_t time_quantum;
    uint64_t ticks_taken;
    uint64_t ticks_slept;/* How many timer ticks has this process ran ? Will inherently be somewhat approximate since it won't know half ticks, quarter ticks etc*/
    uint8_t process_type;
    uint64_t start_time;
    uint8_t file_descriptors[16];
    uint64_t affinity;
    bool inside_kernel;
    bool interrupt_state; //for use in saving/restoring interrupt state with spinlocks
    void *stack;
    void *kernel_stack;
    void *syscall_stack;
    void *sleep_channel;
    struct virtual_handle_list *handle_list;
    struct virt_map *page_map;
    struct cpu *current_cpu; /* Which run queue , if any is this process on? */
    struct register_state *current_register_state;
    struct vnode *current_working_dir;
};


struct register_state {
#ifdef __x86_64__
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
#endif
    bool interrupts_enabled;
};

enum spawn_flags {
    SPAWN_FLAG_NONE            = 0,        // No special flags
    SPAWN_FLAG_INHERIT_FILES   = 1 << 0,   // Inherit file handles/descriptors
    SPAWN_FLAG_INHERIT_ENV     = 1 << 1,   // Inherit environment variables
    SPAWN_FLAG_COPY_ADDRESS    = 1 << 2,   // Copy parent address space
    SPAWN_FLAG_SHARE_ADDRESS   = 1 << 3,   // Share address space (thread-like)
    SPAWN_FLAG_NEW_PROCESS     = 1 << 4,   // Spawn as a new process (default)
    SPAWN_FLAG_NEW_THREAD      = 1 << 5,   // Spawn as a thread in same process
    SPAWN_FLAG_DETACHED        = 1 << 6,   // Detached from parent
    SPAWN_FLAG_NEW_SESSION     = 1 << 7,   // Start in new session
    SPAWN_FLAG_SET_PRIORITY    = 1 << 8,   // Set custom priority
    SPAWN_FLAG_CUSTOM_STACK    = 1 << 9,   // Use custom stack memory
    SPAWN_FLAG_ISOLATE_FS      = 1 << 10,  // Filesystem isolation (virtual FS)
    SPAWN_FLAG_ISOLATE_NET     = 1 << 11,  // Network isolation
    SPAWN_FLAG_DEBUGGABLE      = 1 << 12,  // Allow debugger to attach
    SPAWN_FLAG_NO_WAIT         = 1 << 13,  // Parent doesn't wait on child
    SPAWN_FLAG_INIT_PROCESS    = 1 << 14,  // Mark as system init/root process
    SPAWN_FLAG_SHARED_PAGE_MAP  = 1 << 15, // Share page map (for threading)
};




void exit();
void sleep(void *channel);
void wakeup(const void *channel);
void set_kernel_stack(void *kernel_stack);
struct process *alloc_process(uint64_t state, bool user, struct process *parent);
void free_process(struct process *process);
#endif
