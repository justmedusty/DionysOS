//
// Created by dustyn on 8/12/24.
//
#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <include/memory/pmm.h>


#ifdef _DEBUG_
#define DEBUG_PRINT(string, ...) debug_printf(string, ## __VA_ARGS__)
#else
#define DEBUG_PRINT(string, ...)
#endif

typedef unsigned char byte;

#define BIT(bit) ((1UL << bit))
#define BYTE(num) (num / 8)


extern struct vnode *procfs_root;
extern struct vnode *kernel_message;
extern bool procfs_online;

typedef void (*worker_function)(void *args);

//The ID's I'm going to put below are for spinlock contexts in case there are deadlock/contention issues later
enum {
    SERIAL_LOCK,
    SMP_BOOSTRAP_LOCK,
    SCHED_LOCK,
    VFS_LOCK,
    DIOSFS_LOCK, // may need to make multiple but this is fine for now
    PMM_LOCK,
    BTREE_LOCK, // there are many of these but it's still an insight
    BUDDY_LOCK,
    RAMDISK_LOCK,
    ALLOC_LOCK,
    DOUBLY_LINKED_LIST_LOCK,
    SINGLE_LINKED_LIST_LOCK,
    FRAME_LOCK,
    QUEUE_LOCK
};

#define SPRINTF_MAX_LEN 4096

enum seek_values {
    SEEK_BEGIN = 0,
    SEEK_CUR = 1,
    SEEK_END = 2,
};


enum kernel_error_codes {
    // General Success and Error Codes
    KERN_SUCCESS = 0,              // Operation successful
    KERN_BUSY = -16,               // Device or resource busy (EBUSY)
    KERN_NO_MEM = -12,             // Out of memory (ENOMEM)
    KERN_INVALID_ARG = -22,        // Invalid argument (EINVAL)
    KERN_PERMISSION = -13,         // Permission denied (EACCES)
    KERN_NOT_FOUND = -2,           // No such file or directory (ENOENT)
    KERN_EXISTS = -17,             // File exists (EEXIST)
    KERN_IO_ERROR = -5,            // I/O error (EIO)
    KERN_INTERRUPTED = -4,         // Interrupted system call (EINTR)
    KERN_NOT_SUPPORTED = -95,      // Operation not supported (ENOTSUP)
    KERN_WRONG_TYPE = -19,         // Incorrect type provided (custom)
    KERN_MAX_REACHED = -20,        // Maximum limit reached (custom)
    KERN_NO_SYS = -21,

    // Filesystem and Mounting Errors
    KERN_NO_MOUNT = -100,          // No mount point found (custom)
    KERN_ALREADY_MOUNTED = -101,   // Already mounted (custom)
    KERN_NOT_MOUNTED = -102,       // Not mounted (custom)
    KERN_FS_CORRUPTED = -103,      // Filesystem corrupted (custom)
    KERN_FS_READ_ONLY = -104,      // Filesystem is read-only (EROFS)
    KERN_NO_SPACE = -105,          // No space left on device (ENOSPC)
    KERN_QUOTA_EXCEEDED = -106,    // Quota exceeded (EDQUOT)
    KERN_BAD_HANDLE = -156,

    // Process Management Errors
    KERN_NO_CHILD = -107,          // No child processes (ECHILD)
    KERN_ZOMBIE_PROCESS = -108,    // Process is in zombie state (custom)
    KERN_ACCESS_DENIED = -109,     // Access denied (EACCES)
    KERN_NO_RESOURCE = -110,       // No resources available (ENOMEM)

    // Networking Errors
    KERN_NET_DOWN = -111,          // Network is down (ENETDOWN)
    KERN_NET_UNREACH = -112,       // Network unreachable (ENETUNREACH)
    KERN_NET_RESET = -113,         // Connection reset by network (ENETRESET)
    KERN_CONN_ABORTED = -114,      // Connection aborted (ECONNABORTED)
    KERN_CONN_RESET = -115,        // Connection reset by peer (ECONNRESET)
    KERN_NO_BUFFER = -116,         // No buffer space available (ENOBUFS)
    KERN_CONN_REFUSED = -117,      // Connection refused (ECONNREFUSED)
    KERN_ADDR_IN_USE = -118,       // Address already in use (EADDRINUSE)
    KERN_ADDR_NOT_AVAIL = -119,    // Address not available (EADDRNOTAVAIL)

    // Device and Driver Errors
    KERN_NO_DEVICE = -120,         // No such device (ENODEV)
    KERN_DEVICE_NOT_READY = -121,  // Device not ready (custom)
    KERN_DEVICE_FAILED = -122,     // Device failure (custom)
    KERN_DRIVER_NOT_FOUND = -123,  // Driver not found (custom)
    KERN_DRIVER_FAILED = -124,     // Driver initialization failed (custom)

    // Synchronization and Locking Errors
    KERN_DEADLOCK = -125,          // Deadlock detected (EDEADLK)
    KERN_LOCK_FAILED = -126,       // Lock acquisition failed (custom)
    KERN_ALREADY_LOCKED = -127,    // Resource already locked (custom)

    // Memory and Resource Allocation Errors
    KERN_OUT_OF_BOUNDS = -128,     // Access out of bounds (custom)
    KERN_OVERFLOW = -129,          // Overflow detected (custom)
    KERN_UNDERFLOW = -130,         // Underflow detected (custom)
    KERN_NULL_POINTER = -131,      // Null pointer dereferenced (custom)
    KERN_PAGE_FAULT = -132,        // Page fault occurred (custom)

    // Miscellaneous Errors
    KERN_TIMEOUT = -133,           // Operation timed out (ETIMEDOUT)
    KERN_UNSUPPORTED_OPERATION = -134, // Operation unsupported in current state (custom)
    KERN_UNEXPECTED = -135,        // Unexpected error (custom)
    KERN_ILLEGAL_OPERATION = -136, // Illegal operation (custom)
    KERN_BAD_DESCRIPTOR = -137,    // Bad file descriptor (EBADF)
    KERN_OVERLOADED = -138,        // System overloaded (custom)
};


static inline uint64_t min(uint64_t one, uint64_t two) {
    return one > two ? two : one;
}

static inline uint64_t max(uint64_t one, uint64_t two) {
    return one < two ? two : one;
}

static inline void to_big_endian(void *pointer, uint64_t bytes) {
    char *value = pointer;
    for (int i = 0; i < bytes; i += 2) {
        char temp = value[i];
        value[i] = value[i + 1];
        value[i + 1] = temp;
    }
}

/*
 * Core function prototypes
 */

int64_t write(uint64_t handle, char *buffer, uint64_t bytes);

int64_t read(uint64_t handle, char *buffer, uint64_t bytes);

int64_t open(char *path);

void close(uint64_t handle);

int64_t mount(char *mount_point, char *mounted_filesystem);

int64_t unmount(char *path);

int64_t rename(char *path, char *new_name);

int64_t create(char *path, char *name, uint64_t type);

int64_t get_size(uint64_t handle);

int64_t seek(uint64_t handle, uint64_t whence);

void ksprintf(char *buffer, char *str, ...);

void warn_printf(char *str, ...);

void info_printf(char *str, ...);

void kprintf(char *str, ...);

void kprintf_color(uint32_t color, char *str, ...);

void err_printf(char *str, ...);

void debug_printf(char *str, ...);

void exit_process();

void log_kernel_message(const char *message);

#define DIONYSOS_ASCII_STRING \
"     _____    ____         _____  _____   ______    _____      _____        ______          _____             ______  \n"\
" ___|\\    \\  |    |   ____|\\    \\|\\    \\ |\\     \\  |\\    \\    /    /|   ___|\\     \\    ____|\\    \\        ___|\\     \\ \n"\
"|    |\\    \\ |    |  /     /\\    \\\\\\    \\| \\     \\ | \\    \\  /    / |  |    |\\     \\  /     /\\    \\      |    |\\     \\\n"\
"|    | |    ||    | /     /  \\    \\\\|    \\  \\     ||  \\____\\/    /  /  |    |/____/| /     /  \\    \\     |    |/____/|\n"\
"|    | |    ||    ||     |    |    ||     \\  |    | \\ |    /    /  /___|    \\|   | ||     |    |    | ___|    \\|   | |\n"\
"|    | |    ||    ||     |    |    ||      \\ |    |  \\|___/    /  /|    \\    \\___|/ |     |    |    ||    \\    \\___|/ \n"\
"|    | |    ||    ||\\     \\  /    /||    |\\ \\|    |      /    /  / |    |\\     \\    |\\     \\  /    /||    |\\     \\    \n"\
"|____|/____/||____|| \\_____\\/____/ ||____||\\_____/|     /____/  /  |\\ ___\\|_____|   | \\_____\\/____/ ||\\ ___\\|_____|   \n"\
"|    /    | ||    | \\ |    ||    | /|    |/ \\|   ||    |`    | /   | |    |     |    \\ |    ||    | /| |    |     |   \n"\
"|____|____|/ |____|  \\|____||____|/ |____|   |___|/    |_____|/     \\|____|_____|     \\|____||____|/  \\|____|_____|   \n"\
"  \\(    )/     \\(       \\(    )/      \\(       )/         )/           \\(    )/          \\(    )/        \\(    )/     \n"\
"   '    '       '        '    '        '       '          '             '    '            '    '          '    '      \n"
#define DIONYSOS_ASCII_STRING2 "                                ___           ___                       ___           ___           ___     \n" \
"     _____                     /\\  \\         /\\  \\                     /\\__\\         /\\  \\         /\\__\\    \n" \
"    /::\\  \\       ___         /::\\  \\        \\:\\  \\         ___       /:/ _/_       /::\\  \\       /:/ _/_   \n" \
"   /:/\\:\\  \\     /\\__\\       /:/\\:\\  \\        \\:\\  \\       /|  |     /:/ /\\  \\     /:/\\:\\  \\     /:/ /\\  \\  \n" \
"  /:/  \\:\\__\\   /:/__/      /:/  \\:\\  \\   _____\\:\\  \\     |:|  |    /:/ /::\\  \\   /:/  \\:\\  \\   /:/ /::\\  \\ \n" \
" /:/__/ \\:|__| /::\\  \\     /:/__/ \\:\\__\\ /::::::::\\__\\    |:|  |   /:/_/:/\\:\\__\\ /:/__/ \\:\\__\\ /:/_/:/\\:\\__\\\n" \
" \\:\\  \\ /:/  / \\/\\:\\  \\__  \\:\\  \\ /:/  / \\:\\~~\\~~\\/__/  __|:|__|   \\:\\/:/ /:/  / \\:\\  \\ /:/  / \\:\\/:/ /:/  /\n" \
"  \\:\\  /:/  /   ~~\\:\\/\\__\\  \\:\\  /:/  /   \\:\\  \\       /::::\\  \\    \\::/ /:/  /   \\:\\  /:/  /   \\::/ /:/  / \n" \
"   \\:\\/:/  /       \\::/  /   \\:\\/:/  /     \\:\\  \\      ~~~~\\:\\  \\    \\/_/:/  /     \\:\\/:/  /     \\/_/:/  /  \n" \
"    \\::/  /        /:/  /     \\::/  /       \\:\\__\\          \\:\\__\\     /:/  /       \\::/  /        /:/  /   \n" \
"     \\/__/         \\/__/       \\/__/         \\/__/           \\/__/     \\/__/         \\/__/         \\/__/    \n"

#define DIONYSOS_ASCII_STRING3 "       _             _          _            _           _        _         _            _            _        \n" \
"      /\\ \\          /\\ \\       /\\ \\         /\\ \\     _  /\\ \\     /\\_\\      / /\\         /\\ \\         / /\\      \n" \
"     /  \\ \\____     \\ \\ \\     /  \\ \\       /  \\ \\   /\\_\\\\ \\ \\   / / /     / /  \\       /  \\ \\       / /  \\     \n" \
"    / /\\ \\_____\\    /\\ \\_\\   / /\\ \\ \\     / /\\ \\ \\_/ / / \\ \\ \\_/ / /     / / /\\ \\__   / /\\ \\ \\     / / /\\ \\__  \n" \
"   / / /\\/___  /   / /\\/_/  / / /\\ \\ \\   / / /\\ \\___/ /   \\ \\___/ /     / / /\\ \\___\\ / / /\\ \\ \\   / / /\\ \\___\\ \n" \
"  / / /   / / /   / / /    / / /  \\ \\_\\ / / /  \\/____/     \\ \\ \\_/      \\ \\ \\ \\/___// / /  \\ \\_\\  \\ \\ \\ \\/___/ \n" \
" / / /   / / /   / / /    / / /   / / // / /    / / /       \\ \\ \\        \\ \\ \\     / / /   / / /   \\ \\ \\       \n" \
"/ / /   / / /   / / /    / / /   / / // / /    / / /         \\ \\ \\   _    \\ \\ \\   / / /   / / /_    \\ \\ \\      \n" \
"\\ \\ \\__/ / /___/ / /__  / / /___/ / // / /    / / /           \\ \\ \\ /_/\\__/ / /  / / /___/ / //_\\__/ / /      \n" \
" \\ \\___\\/ //\\__\\/ /___\\/ / /____\\/ // / /    / / /             \\ \\_\\\\ \\/___/ /  / / /____\\/ / \\ \\/___/ /       \n" \
"  \\/_____\\/\\/_________/\\/_________/ \\/_/     \\/_/               \\/_/ \\_____\\/   \\/_________/   \\_____\\/        \n"

#define AUTHOR_ASCII_STRING " __        __    _ _   _               _             ____            _                  ____ _ _     _     \n \\ \\      / / __(_) |_| |_ ___ _ __   | |__  _   _  |  _ \\ _   _ ___| |_ _   _ _ __    / ___(_) |__ | |__  \n  \\ \\ /\\ / / '__| | __| __/ _ \\ '_ \\  | '_ \\| | | | | | | | | | / __| __| | | | '_ \\  | |  _| | '_ \\| '_ \\ \n   \\ V  V /| |  | | |_| ||  __/ | | | | |_) | |_| | | |_| | |_| \\__ \\ |_| |_| | | | | | |_| | | |_) | |_) |\n    \\_/\\_/ |_|  |_|\\__|\\__\\___|_| |_| |_.__/ \\__, | |____/ \\__,_|___/\\__|\\__, |_| |_|  \\____|_|_.__/|_.__/ \n                                             |___/                       |___/                              \n"

#define DIONYSOS_ART \
".    .\n" \
"    .       =O===\n" \
" . _       %- - %%%\n" \
"  (_)     % <    D%%%\n" \
"   |      ' ,  ,$$$\n" \
"  -/-     %%%%  |\n" \
"   |//; ,---' _ |----.\n" \
"    \\ )(           /  )\n" \
"    | \\/ \\.   '  _.|,  \\\n" \
"    |  \\ /(   /    / \\_ \\\n" \
"     \\ /  (        | /  )\n" \
"          (  ,  ___|/ ,`\n" \
"          (   _/ (  |  \\_\n" \
"        _/\\--//     ,\\\\\\\n" \
"  _______ _/\\\\   / |      |_________\n" \
"|_______/   \\\\ / _/   ,   |________|\n" \
"__|___#/     \\  _  '      |##____|___\n" \
"/______/ _    |  / \\   |   /__________\\\n" \
"  |   /           _/\\_/  b'ger\n" \
"  /_     '_,____\\/ )(\n" \
" |/ \\_   /        (  \\\n" \
"  | /-\\_/          Oooo\n" \
"  )(. \n" \
" /  )\n" \
"oooO\n"

#define ICARUS_ART \
"                 )\n" \
"               /  )\n" \
"              /  / )\n" \
"         -   /  / /\n" \
"            '  / / -\n" \
"           / _/ / /\n" \
"     _    / _/_, /          ,\n" \
"   + $$$ / _/_/_/          \\       |\n" \
" /- + $$/ _/_/_/      /\n" \
" \\`_ $$/'_/_/    .    ______   _\n" \
"    \\ (  / ___,_____ _ _____,\n" \
"    |  `(|/_,_,__ ________/\n" \
"    |.   |''_,_______)\n" \
"     \\   (_\n" \
"      \\  / |-._\n" \
"       \\.' /|/ \\_._\n" \
"       /_/   _/    /-'__\n" \
"         \\     \\'       \\.___\n" \
"          '.   /,     |_/_   |._\n" \
"            \\ / )   '.     '_/, )\n" \
"             (_(     -\\_   /  \\ \\\n" \
"                \\__      |-'   |/\n" \
"                  \\._  /_/_\n" \
"                     \\_/\\' )\n" \
"                         \\ |\n" \
"                         |/   b'ger\n" \
"\n" \
"              ~~~~~~~\n" \
"       ~~~~~~~~~   ~~~~\n"
#endif
