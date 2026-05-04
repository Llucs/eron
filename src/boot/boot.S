# Eron OS — boot and interrupt stubs

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384
.global stack_top
stack_top:

# ── entry point ───────────────────────────────────────────

.section .text
.global _start
.type _start, @function
_start:
    movl $stack_top, %esp
    andl $-16, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b
.size _start, . - _start

# ── IDT load ─────────────────────────────────────────────

.global idt_load
.extern idtp
idt_load:
    lidt (idtp)
    ret

# ── GDT flush ────────────────────────────────────────────

.global gdt_flush
gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.gdt_done
.gdt_done:
    ret

# ── TSS flush ────────────────────────────────────────────

.global tss_flush
tss_flush:
    mov $0x28, %ax
    ltr %ax
    ret

# ── context switch ───────────────────────────────────────
# void swtch(struct context **old, struct context *new)

.global swtch
swtch:
    mov 4(%esp), %eax
    mov 8(%esp), %edx
    push %ebp
    push %ebx
    push %esi
    push %edi
    mov %esp, (%eax)
    mov %edx, %esp
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    ret

# ── return to user mode (restore trapframe) ──────────────

.global trapret
trapret:
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret

# ── ISR stubs (CPU exceptions — no error code) ──────────

.global isr0
isr0:
    push $0
    push $0
    jmp int_common

.global isr6
isr6:
    push $0
    push $6
    jmp int_common

# ── ISR stubs (CPU exceptions — with error code) ────────

.global isr13
isr13:
    push $13
    jmp int_common

.global isr14
isr14:
    push $14
    jmp int_common

# ── IRQ stubs ────────────────────────────────────────────

.global timer_handler_asm
timer_handler_asm:
    push $0
    push $32
    jmp int_common

.global keyboard_handler_asm
keyboard_handler_asm:
    push $0
    push $33
    jmp int_common

# ── syscall stub (INT 0x80) ──────────────────────────────

.global syscall_handler_asm
syscall_handler_asm:
    push $0
    push $0x80
    jmp int_common

# ── common interrupt frame ───────────────────────────────

.extern interrupt_dispatch
int_common:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call interrupt_dispatch
    add $4, %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret
