#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROC_MAX        16
#define PROC_KSTACK_SZ  4096
#define PROC_USTACK_SZ  4096
#define PROC_NAME_LEN   16

enum proc_state {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_ZOMBIE
};

struct trapframe {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, _esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss;
};

struct context {
    uint32_t edi, esi, ebx, ebp, eip;
};

struct process {
    uint32_t pid;
    char name[PROC_NAME_LEN];
    enum proc_state state;
    uint8_t kernel_stack[PROC_KSTACK_SZ] __attribute__((aligned(16)));
    uint8_t user_stack[PROC_USTACK_SZ] __attribute__((aligned(16)));
    struct context* ctx;
    struct trapframe* tf;
    int exit_code;
    struct page_directory* pdir;
};

void proc_init(void);
int  proc_create(const char* name, uint32_t entry);
void schedule(void);
struct process* proc_current(void);
void proc_exit(int code);
void proc_yield(void);
int  proc_active_count(void);
struct process* proc_table_ptr(void);

#endif
