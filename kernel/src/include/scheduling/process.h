//
// Created by dustyn on 9/14/24.
//

#pragma once
#include "include/types.h"
#include "include/arch/arch_vmm.h"
#include "include/arch/arch_cpu.h"
#include ""

struct process {
  uint16 process_id;
  uint16 parent_process_id;
  struct page_map* page_map;
  struct cpu* current_cpu; /* Which run queue , if any is this process on? */
  uint8 current_state;


}
