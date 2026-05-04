#include "../include/syscall.h"
#include "../include/mm.h"
#include "../include/timer.h"
#include "../include/idt.h"

extern void terminal_writestring(const char* data);

extern void syscall_handler_asm(void);

struct regs {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

void syscall_dispatch(struct regs* r) {
    switch (r->eax) {
    case SYS_WRITE:
        if (r->ebx == 1 && r->ecx) {
            terminal_writestring((const char*)r->ecx);
        }
        r->eax = 0;
        break;

    case SYS_MALLOC:
        r->eax = (uint32_t)kmalloc(r->ebx);
        break;

    case SYS_FREE:
        kfree((void*)r->ebx);
        r->eax = 0;
        break;

    case SYS_TIME:
        r->eax = timer_seconds();
        break;

    case SYS_GETPID:
        r->eax = 0;
        break;

    default:
        r->eax = (uint32_t)-1;
        break;
    }
}

void syscall_init(void) {
    idt_set_gate(0x80, (uint32_t)syscall_handler_asm, 0x08, 0xEE);
}
