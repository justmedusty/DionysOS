//
// Created by dustyn on 12/25/24.
//

#ifndef SYSTEM_CALLS_H
#define SYSTEM_CALLS_H

enum system_calls {
    SYS_WRITE,
    SYS_READ,
    SYS_SEEK,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_EXIT,
    SYS_WAIT,
    SYS_SPAWN,
    SYS_EXEC,
    SYS_CREATE,
    SYS_HEAP_GROW,
    SYS_HEAP_SHRINK,
};
#endif //SYSTEM_CALLS_H
