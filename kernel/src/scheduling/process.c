//
// Created by dustyn on 12/7/24.
//

#include "include/scheduling/process.h"
#include "include/definitions/elf.h"
#include "include/filesystem/vfs.h"
#include "include/memory/vmm.h"
#include "include/architecture/arch_cpu.h"
#include "include/architecture/x86_64/gdt.h"
#include "include/data_structures/doubly_linked_list.h"
#include "include/scheduling/sched.h"

// We will just have a 10mb sensible max for our elf files since I want to read the whole thing into memory on execute
#define SENSIBLE_FILE_SIZE (10 << 20)
extern uint64_t get_current_stack();
extern uint64_t get_current_base();
uint64_t user_proc_ids = 0;
bool lock_set = false;
struct spinlock lock;

uint64_t get_process_id() {
    if (!lock_set) {
        initlock(&lock, ALLOC_LOCK);
        lock_set = true;
    }

    acquire_spinlock(&lock);
    uint64_t ret = user_proc_ids++;
    release_spinlock(&lock);
    return ret;
}
/*
 * Under construction
 * *jack hammer noises*
 */
int32_t exec(char *path_to_executable, char **argv) {
    uint64_t *top_level_page_table = alloc_virtual_map();
    struct process *current = current_process();
}

struct process *alloc_process(uint64_t state, bool user, struct process *parent) {
    DEBUG_PRINT("alloc_process: User proc : %i\n",user);
    struct process *process = kzmalloc(sizeof(struct process));
    bool init = parent == NULL ? true : false;

    process->kernel_stack =  (void*) (uint64_t)kzmalloc(DEFAULT_STACK_SIZE) + DEFAULT_STACK_SIZE;
    process->handle_list = kzmalloc(sizeof(struct virtual_handle_list *));
    process->handle_list->handle_list = kzmalloc(sizeof(struct doubly_linked_list *));


    process->page_map = kzmalloc(sizeof(struct virt_map));
    process->page_map->top_level = alloc_virtual_map();
    map_kernel_address_space(process->page_map->top_level);
    process->page_map->vm_regions = kzmalloc(sizeof(struct doubly_linked_list));

    process->process_id = get_process_id();
    process->current_register_state = kzmalloc(sizeof(struct register_state));
    process->process_type = USER_PROCESS;

    if (user) {
        process->stack = umalloc(DEFAULT_STACK_SIZE / PAGE_SIZE);
        arch_map_pages(process->page_map->top_level,(uint64_t)process->stack ,(uint64_t *)(uint64_t)(USER_STACK_TOP - DEFAULT_STACK_SIZE),READWRITE | NO_EXECUTE | USER,DEFAULT_STACK_SIZE);
    }

    if (!init) {
        process->current_working_dir = parent->current_working_dir;
        process->parent_process_id = parent->process_id;
        process->effective_priority = parent->effective_priority;
        process->priority = parent->priority;
    } else {
        process->current_working_dir = &vfs_root;
        process->priority = HIGH;
        process->effective_priority = HIGH;
        process->parent_process_id = 0;
#ifdef __x86_64__
    process->current_register_state->rsp = (uint64_t) process->kernel_stack;
#endif
    }


    doubly_linked_list_init(process->page_map->vm_regions);
    doubly_linked_list_init(process->handle_list->handle_list);


    if (init) {
        DEBUG_PRINT("alloc_process: Handle list lock %i\n",process->handle_list->handle_list->lock.locked);
    }

    return process;
}

void free_process(struct process *process) {
    kfree(process->kernel_stack);

    if ((process->page_map->top_level != kernel_pg_map->top_level)) {
        DEBUG_PRINT("free_process: freeing user page map\n");
        arch_dealloc_page_table(process->page_map->top_level);
        free_virtual_map(process->page_map->top_level);
        doubly_linked_list_destroy(process->page_map->vm_regions,true);
        ufree(process->stack);
        kfree(process->page_map);
        kfree(process->page_map->vm_regions);
    }

    doubly_linked_list_destroy(process->handle_list->handle_list,true);
    kfree(process->handle_list->handle_list);
    kfree(process->handle_list);
    kfree(process->current_register_state);

    kfree(process);
}

__attribute__((noreturn))
void exit() {
    sched_exit();
}

void sleep(void *channel) {
    sched_sleep(channel);
}

void wakeup(const void *channel) {
    sched_wakeup(channel);
}

int64_t spawn(char *path_to_executable, uint64_t flags, uint64_t aux_arguments) {
    struct process *current = current_process();

    struct process *new_process = alloc_process(PROCESS_READY,true, current);

    elf_info info;

    const int64_t ret = load_elf(new_process, path_to_executable, 0, &info);

    if (ret != KERN_SUCCESS) {
        return ret;
    }

#ifdef __x86_64__
    new_process->current_register_state->rsp = (uint64_t) new_process->stack;
#endif

    global_enqueue_process(new_process);

    return KERN_SUCCESS;
}

int64_t fork(uint64_t flags, uint64_t aux_arguments) {
    uint64_t *top_level_page_table = alloc_virtual_map();
    struct process *current = current_process();

    struct process *new_process = alloc_process(PROCESS_READY,true, current);

#ifdef __x86_64__
    new_process->current_register_state->rsp = (uint64_t) new_process->stack;
#endif

    global_enqueue_process(new_process);

    return KERN_SUCCESS;
}


void set_kernel_stack(void *kernel_stack) {
#ifdef __x86_64__
    my_cpu()->tss->rsp0 = (uint64_t) kernel_stack;
#endif
}
