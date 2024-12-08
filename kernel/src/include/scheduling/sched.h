//
// Created by dustyn on 9/14/24.
//

#ifndef _SCHED_H_
#define _SCHED_H_

extern void restore_execution();
extern void get_regs();

#define BASE_QUANTUM 50 /* Base Time Quantum,  can be multiplied by task prio not sure how I'll do that yet */

enum task_priority {
    LOW = 0,
    LOW_MEDIUM = 1,
    MEDIUM = 2,
    MEDIUM_HIGH = 3,
    HIGH = 4,
    URGENT = 5,
    REAL_TIME = 6
};


void sched_init();
void sched__yield();
void sched_run();
void sched_preempt();
void sched_claim_process();
void sched_exit();
#endif