//
// Created by dustyn on 9/14/24.
//

#ifndef _DFS_H_
#define _DFS_H_

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


void dfs_init();
void dfs_yield();
void dfs_run();
void dfs_preempt();
void dfs_claim_process();

#endif