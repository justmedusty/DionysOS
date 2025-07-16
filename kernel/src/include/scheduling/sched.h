//
// Created by dustyn on 9/14/24.
//

#ifndef _SCHED_H_
#define _SCHED_H_

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


void sched_init(void);
void sched_yield(void);
void sched_run(void);
void sched_preempt(void);
void sched_claim_process(void);
void sched_exit(void);
_Noreturn void scheduler_main(void);
void sched_sleep(void *sleep_channel);
void sched_wakeup(const void *wakeup_channel);
void global_enqueue_process(struct process *process);
#endif