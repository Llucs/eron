#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/mm.h"
#include "../include/timer.h"
#include "../include/idt.h"

extern void terminal_writestring(const char* data);
extern void syscall_handler_asm(void);

void syscall_handle(struct trapframe* tf) {
    switch (tf->eax) {
    case SYS_WRITE:
        if (tf->ebx == 1 && tf->ecx)
            terminal_writestring((const char*)tf->ecx);
        tf->eax = 0;
        break;

    case SYS_EXIT:
        proc_exit((int)tf->ebx);
        break;

    case SYS_GETPID: {
        struct process* p = proc_current();
        tf->eax = p ? p->pid : 0;
        break;
    }

    case SYS_MALLOC:
        tf->eax = (uint32_t)kmalloc(tf->ebx);
        break;

    case SYS_FREE:
        kfree((void*)tf->ebx);
        tf->eax = 0;
        break;

    case SYS_TIME:
        tf->eax = timer_seconds();
        break;

    case SYS_YIELD:
        tf->eax = 0;
        proc_yield();
        break;

    default:
        tf->eax = (uint32_t)-1;
        break;
    }
}

void syscall_init(void) {
    idt_set_gate(0x80, (uint32_t)syscall_handler_asm, 0x08, 0xEE);
}
