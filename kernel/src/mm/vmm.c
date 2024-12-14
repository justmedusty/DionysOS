//
// Created by dustyn on 6/21/24.
//

#include "include/mem/vmm.h"
#include "include/arch/arch_vmm.h"

struct virt_map* kernel_pg_map;

void arch_kvm_init(p4d_t *pgdir){
    map_kernel_address_space(pgdir);
}

void arch_vmm_init(){
    init_vmm();
}

struct vm_region* arch_create_region(){
}

void arch_attach_region(struct virt_map *){

}

void arch_detach_region(struct virt_map *){

}
