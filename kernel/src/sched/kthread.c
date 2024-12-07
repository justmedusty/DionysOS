//
// Created by dustyn on 12/7/24.
//
#include <stdint.h>
#include "include/scheduling/kthread.h"
#include "include/mem/kalloc.h"
#include "include/scheduling/process.h"

void kthread_init(){
  struct process *proc = _kalloc(sizeof(struct process));
}
