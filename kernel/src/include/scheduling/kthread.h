//
// Created by dustyn on 12/7/24.
//

#ifndef KTHREAD_H
#define KTHREAD_H
#pragma once
#include "include/definitions/definitions.h"
void kthread_init();
void kthread_main();
void kthread_work(worker_function function, void *args);
#endif //KTHREAD_H
