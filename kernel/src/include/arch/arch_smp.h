//
// Created by dustyn on 6/24/24.
//

#ifndef KERNEL_ARCH_SMP_H
#define KERNEL_ARCH_SMP_H
void smp_init();
extern uint8 smp_enabled;
extern uint8 cpus_online;
extern uint64 bootstrap_lapic_id;
extern uint64 cpu_count;
#endif //KERNEL_ARCH_SMP_H
