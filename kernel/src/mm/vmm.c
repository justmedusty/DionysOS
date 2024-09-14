//
// Created by dustyn on 6/21/24.
//

#include "include/mem/vmm.h"
#include "include/arch/arch_vmm.h"

struct virt_map* kernel_pg_map;

void kvm_init(p4d_t *pgdir){
    arch_map_kernel_address_space(pgdir);
}

void vmm_init(){
    arch_init_vmm();
}

struct vm_region* create_region(){
}

void attatch_region(struct virt_map *){

}

void detatch_region(struct virt_map *){

}
