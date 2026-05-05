#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/mm.h"
#include "../include/timer.h"
#include "../include/idt.h"
#include "../include/vfs.h"

extern void terminal_writestring(const char* data);
extern void syscall_handler_asm(void);

/* User space memory boundaries (typically 0x08000000 to 0xC0000000) */
#define USER_SPACE_START 0x08000000
#define USER_SPACE_END   0xC0000000

/* Kernel memory boundaries */
#define KERNEL_SPACE_START 0xC0000000
#define KERNEL_SPACE_END   0xFFFFFFFF

/* Validate that a pointer is in user space */
static int validate_user_ptr(const void* ptr, size_t size) {
    uint32_t addr = (uint32_t)ptr;
    uint32_t end = addr + size;
    
    /* Check for NULL or obviously invalid */
    if (addr == 0 || addr < USER_SPACE_START) return 0;
    if (end > USER_SPACE_END || end < addr) return 0;
    
    return 1;
}

/* Validate that a pointer is in kernel space */
static int validate_kernel_ptr(const void* ptr, size_t size) {
    uint32_t addr = (uint32_t)ptr;
    uint32_t end = addr + size;
    
    /* Check for NULL */
    if (addr == 0) return 0;
    if (end > KERNEL_SPACE_END || end < addr) return 0;
    
    return 1;
}

/* Validate memory read access */
static int validate_read(const void* ptr, size_t size) {
    return validate_user_ptr(ptr, size);
}

/* Validate memory write access */
static int validate_write(const void* ptr, size_t size) {
    return validate_user_ptr(ptr, size);
}

void syscall_handle(struct trapframe* tf) {
    switch (tf->eax) {
    case SYS_WRITE: {
        /* Validate: ebx = fd, ecx = buffer, edx = count */
        if (tf->ebx == 1 && tf->ecx) {
            /* Validate user pointer before using */
            if (validate_user_ptr((const void*)tf->ecx, 1)) {
                terminal_writestring((const char*)tf->ecx);
            }
        }
        tf->eax = 0;
        break;
    }

    case SYS_READ: {
        /* SYS_READ not fully implemented - return error */
        tf->eax = (uint32_t)-1;
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

    case SYS_MALLOC: {
        /* Validate size request */
        if (tf->ebx < 0x100000) { /* Max 1MB allocation */
            tf->eax = (uint32_t)kmalloc(tf->ebx);
        } else {
            tf->eax = 0;
        }
        break;
    }

    case SYS_FREE: {
        /* Validate pointer before freeing */
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

    case SYS_OPEN:
    case SYS_CLOSE:
    case SYS_STAT:
        /* VFS operations - validate pointers */
        if (tf->ebx && validate_user_ptr((const void*)tf->ebx, 1)) {
            /* File operations would go here */
        }
        tf->eax = (uint32_t)-1;
        break;

    case SYS_EXEC:
        /* SECURE: Only kernel can exec, not from userspace directly */
        tf->eax = (uint32_t)-1;
        break;

    default:
        tf->eax = (uint32_t)-1;
        break;
    }
}

void syscall_init(void) {
    idt_set_gate(0x80, (uint32_t)syscall_handler_asm, 0x08, 0xEE);
}
