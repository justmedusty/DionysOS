//
// Created by dustyn on 12/25/24.
//
#include "stdint.h"
#include "include/system_call/system_calls.h"
int32_t system_call_dispatch(){

}

void register_syscall_dispatch(){
    set_syscall_handler();
}