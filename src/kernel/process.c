#include "../include/process.h"
#include "../include/gdt.h"
#include "../include/tss.h"
#include <stddef.h>

extern void swtch(struct context** old, struct context* new_ctx);
extern void trapret(void);

static struct process procs[PROC_MAX];
static struct process* current_proc;
static uint32_t next_pid = 1;

static void p_strcpy(char* d, const char* s, int max) {
    int i;
    for (i = 0; i < max - 1 && s[i]; i++) d[i] = s[i];
    d[i] = '\0';
}

void proc_init(void) {
    for (int i = 0; i < PROC_MAX; i++) {
        procs[i].state = PROC_UNUSED;
        procs[i].pid   = 0;
    }
    procs[0].pid   = 0;
    p_strcpy(procs[0].name, "kernel", PROC_NAME_LEN);
    procs[0].state = PROC_RUNNING;
    procs[0].ctx   = NULL;
    current_proc   = &procs[0];
}

int proc_create(const char* name, uint32_t entry) {
    struct process* p = NULL;
    for (int i = 1; i < PROC_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            p = &procs[i];
            break;
        }
    }
    if (!p) return -1;

    p->pid = next_pid++;
    p_strcpy(p->name, name, PROC_NAME_LEN);
    p->exit_code = 0;

    uint32_t sp = (uint32_t)p->kernel_stack + PROC_KSTACK_SZ;

    /* trapframe at top of kernel stack */
    sp -= sizeof(struct trapframe);
    p->tf = (struct trapframe*)sp;

    p->tf->gs  = SEG_UDATA | 3;
    p->tf->fs  = SEG_UDATA | 3;
    p->tf->es  = SEG_UDATA | 3;
    p->tf->ds  = SEG_UDATA | 3;
    p->tf->edi = 0;
    p->tf->esi = 0;
    p->tf->ebp = 0;
    p->tf->_esp = 0;
    p->tf->ebx = 0;
    p->tf->edx = 0;
    p->tf->ecx = 0;
    p->tf->eax = 0;
    p->tf->int_no   = 0;
    p->tf->err_code = 0;
    p->tf->eip      = entry;
    p->tf->cs       = SEG_UCODE | 3;
    p->tf->eflags   = 0x202;   /* IF set */
    p->tf->user_esp = (uint32_t)p->user_stack + PROC_USTACK_SZ;
    p->tf->user_ss  = SEG_UDATA | 3;

    /* context for swtch (below trapframe) */
    sp -= sizeof(struct context);
    p->ctx = (struct context*)sp;
    p->ctx->edi = 0;
    p->ctx->esi = 0;
    p->ctx->ebx = 0;
    p->ctx->ebp = 0;
    p->ctx->eip = (uint32_t)trapret;

    p->state = PROC_READY;
    return (int)p->pid;
}

void schedule(void) {
    if (!current_proc) return;

    struct process* prev = current_proc;

    /* find slot of current process */
    int cur = 0;
    for (int i = 0; i < PROC_MAX; i++) {
        if (&procs[i] == prev) { cur = i; break; }
    }

    /* round-robin: find next READY process */
    int next_slot = -1;
    for (int i = 1; i <= PROC_MAX; i++) {
        int idx = (cur + i) % PROC_MAX;
        if (procs[idx].state == PROC_READY) {
            next_slot = idx;
            break;
        }
    }

    if (next_slot < 0) {
        /* no ready process — go back to kernel if current exited */
        if (prev->state != PROC_RUNNING && cur != 0) {
            next_slot = 0;
            if (procs[0].state != PROC_READY &&
                procs[0].state != PROC_RUNNING)
                return;
        } else {
            return;
        }
    }

    struct process* next = &procs[next_slot];
    if (next == prev) return;

    if (prev->state == PROC_RUNNING)
        prev->state = PROC_READY;
    next->state = PROC_RUNNING;

    if (next_slot != 0)
        tss_set_kernel_stack((uint32_t)next->kernel_stack + PROC_KSTACK_SZ);

    current_proc = next;
    swtch(&prev->ctx, next->ctx);
}

struct process* proc_current(void) { return current_proc; }

void proc_exit(int code) {
    if (!current_proc || current_proc->pid == 0) return;
    current_proc->state     = PROC_ZOMBIE;
    current_proc->exit_code = code;
    schedule();
}

void proc_yield(void) {
    schedule();
}

int proc_active_count(void) {
    int n = 0;
    for (int i = 0; i < PROC_MAX; i++)
        if (procs[i].state != PROC_UNUSED) n++;
    return n;
}

struct process* proc_table_ptr(void) { return procs; }
