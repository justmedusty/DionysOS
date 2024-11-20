//
// Created by dustyn on 6/24/24.
//

#ifndef KERNEL_ARCH_SMP_H
#define KERNEL_ARCH_SMP_H
void smp_init();
extern uint8_t smp_enabled;
extern uint8_t cpus_online;
extern uint64_t bootstrap_lapic_id;
extern uint64_t cpu_count;
#endif //KERNEL_ARCH_SMP_H
