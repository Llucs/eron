#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/mm.h"
#include "../include/timer.h"
#include "../include/idt.h"
#include "../include/vfs.h"

extern void terminal_writestring(const char* data);
extern void syscall_handler_asm(void);

#define USER_SPACE_START 0x08000000
#define USER_SPACE_END   0xC0000000
#define KERNEL_SPACE_START 0xC0000000
#define KERNEL_SPACE_END   0xFFFFFFFF

static int validate_user_ptr(const void* ptr, size_t size) {
    uint32_t addr = (uint32_t)ptr;
    uint32_t end = addr + size;
    if (addr == 0 || addr < USER_SPACE_START) return 0;
    if (end > USER_SPACE_END || end < addr) return 0;
    return 1;
}

/* Removed unused: validate_kernel_ptr, validate_read, validate_write */

static int do_write(int fd, const char* buf, size_t count) {
    if (fd == 1 || fd == 2) {
        if (validate_user_ptr(buf, count)) {
            terminal_writestring(buf);
        }
        return (int)count;
    }
    return -1;
}

static int do_read(int fd, char* buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

void syscall_handle(struct trapframe* tf) {
    switch (tf->eax) {
    case SYS_WRITE: {
        tf->eax = do_write((int)tf->ebx, (const char*)tf->ecx, (size_t)tf->edx);
        break;
    }

    case SYS_READ: {
        tf->eax = do_read((int)tf->ebx, (char*)tf->ecx, (size_t)tf->edx);
        break;
    }

    case SYS_EXIT:
        proc_exit((int)tf->ebx);
        break;

    case SYS_GETPID: {
        struct process* p = proc_current();
        tf->eax = p ? p->pid : 0;
        break;
    }

    case SYS_GETTID: {
        struct process* p = proc_current();
        tf->eax = p ? p->pid : 0;
        break;
    }

    case SYS_GETPPID: {
        tf->eax = 0;
        break;
    }

    case SYS_GETUID: {
        tf->eax = 0;
        break;
    }

    case SYS_GETEUID: {
        tf->eax = 0;
        break;
    }

    case SYS_GETGID: {
        tf->eax = 0;
        break;
    }

    case SYS_GETEGID: {
        tf->eax = 0;
        break;
    }

    case SYS_MALLOC: {
        if (tf->ebx < 0x100000) {
            tf->eax = (uint32_t)kmalloc(tf->ebx);
        } else {
            tf->eax = 0;
        }
        break;
    }

    case SYS_FREE: {
        if (tf->ebx && validate_user_ptr((const void*)tf->ebx, 1)) {
            kfree((void*)tf->ebx);
        }
        tf->eax = 0;
        break;
    }

    case SYS_TIME:
        tf->eax = timer_seconds();
        break;

    case SYS_YIELD:
        tf->eax = 0;
        proc_yield();
        break;

    case SYS_SLEEP: {
        uint32_t target = timer_seconds() + tf->ebx;
        while (timer_seconds() < target) {
            proc_yield();
        }
        tf->eax = 0;
        break;
    }

    case SYS_OPEN: {
        if (tf->ebx && validate_user_ptr((const void*)tf->ebx, 1)) {
            const char* path = (const char*)tf->ebx;
            struct vfs_node* n = vfs_lookup(path);
            if (n) {
                tf->eax = 0;
            } else {
                tf->eax = (uint32_t)-1;
            }
        } else {
            tf->eax = (uint32_t)-1;
        }
        break;
    }

    case SYS_CLOSE: {
        tf->eax = 0;
        break;
    }

    case SYS_LSEEK: {
        tf->eax = (uint32_t)-1;
        break;
    }

    case SYS_STAT: {
        if (tf->ebx && validate_user_ptr((const void*)tf->ebx, 1)) {
            tf->eax = (uint32_t)-1;
        }
        tf->eax = (uint32_t)-1;
        break;
    }

    case SYS_EXEC: {
        tf->eax = (uint32_t)-1;
        break;
    }

    case SYS_FORK: {
        tf->eax = 0;
        break;
    }

    case SYS_KILL: {
        tf->eax = (uint32_t)-1;
        break;
    }

    case SYS_ALARM: {
        tf->eax = 0;
        break;
    }

    case SYS_BRK: {
        tf->eax = 0;
        break;
    }

    case SYS_DUP: {
        tf->eax = (int)tf->ebx;
        break;
    }

    case SYS_DUP2: {
        tf->eax = (int)tf->ebx;
        break;
    }

    case SYS_PIPE: {
        tf->eax = (uint32_t)-1;
        break;
    }

    case SYS_CHMOD: {
        tf->eax = 0;
        break;
    }

    case SYS_MKDIR: {
        if (tf->ebx && validate_user_ptr((const void*)tf->ebx, 1)) {
            tf->eax = vfs_mkdir((const char*)tf->ebx);
        } else {
            tf->eax = (uint32_t)-1;
        }
        break;
    }

    case SYS_RMDIR:
    case SYS_UNLINK: {
        tf->eax = 0;
        break;
    }

    case SYS_RENAME: {
        tf->eax = 0;
        break;
    }

    case SYS_UMASK: {
        tf->eax = 0;
        break;
    }

    case SYS_GETCWD:
    case SYS_GETPGRP:
    case SYS_GETSID:
    case SYS_SETSID: {
        tf->eax = 0;
        break;
    }
    case SYS_SETPGRP: {
        tf->eax = 0;
        break;
    }

    default:
        tf->eax = (uint32_t)-1;
        break;
    }
}

void syscall_init(void) {
    idt_set_gate(0x80, (uint32_t)syscall_handler_asm, 0x08, 0xEE);
}
