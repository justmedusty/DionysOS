//
// Created by dustyn on 6/16/24.
//

#ifndef DIONYSOS_TYPES_H
#define DIONYSOS_TYPES_H
#include <stdint.h>
#define TRUE 1
#define FALSE 0

/*For x86_64 paging, when I implement RISC-V in the future this will be a different layout and need to be ifdefd*/
typedef uint64_t pte_t;
typedef uint64_t pmd_t;
typedef uint64_t pud_t;
typedef uint64_t p4d_t;
#endif //DIONYSOS_TYPES_H
