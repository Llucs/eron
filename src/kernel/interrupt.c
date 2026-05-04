#include "../include/process.h"
#include "../include/io.h"
#include "../include/vga.h"
#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);
extern void timer_tick(struct trapframe* tf);
extern void keyboard_irq(void);
extern void syscall_handle(struct trapframe* tf);

static void print_hex(uint32_t v) {
    const char hex[] = "0123456789ABCDEF";
    char h[9];
    for (int i = 7; i >= 0; i--) { h[i] = hex[v & 0xF]; v >>= 4; }
    h[8] = '\0';
    terminal_writestring(h);
}

static void print_dec(uint32_t v) {
    char buf[12];
    if (v == 0) { terminal_writestring("0"); return; }
    int i = 0;
    char tmp[12];
    while (v) { tmp[i++] = '0' + v % 10; v /= 10; }
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
    terminal_writestring(buf);
}

static const char* fault_name(uint32_t n) {
    switch (n) {
    case 0:  return "division error";
    case 6:  return "invalid opcode";
    case 13: return "general protection";
    case 14: return "page fault";
    default: return "exception";
    }
}

void interrupt_dispatch(struct trapframe* tf) {
    switch (tf->int_no) {
    case 0: case 6: case 13: case 14: {
        uint8_t err_c = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        uint8_t body  = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        if ((tf->cs & 3) == 3) {
            struct process* p = proc_current();
            terminal_setcolor(err_c);
            terminal_writestring("\n[fault] ");
            terminal_writestring(fault_name(tf->int_no));
            terminal_writestring(" in pid ");
            if (p) print_dec(p->pid);
            terminal_writestring(" at 0x");
            print_hex(tf->eip);
            if (tf->int_no == 13 || tf->int_no == 14) {
                terminal_writestring(" err=0x");
                print_hex(tf->err_code);
            }
            terminal_setcolor(body);
            terminal_writestring("\n");
            proc_exit(-1);
        } else {
            terminal_setcolor(err_c);
            terminal_writestring("\n\nKERNEL PANIC: ");
            terminal_writestring(fault_name(tf->int_no));
            terminal_writestring(" at 0x");
            print_hex(tf->eip);
            terminal_writestring("\nSystem halted.");
            asm volatile("cli");
            for (;;) asm volatile("hlt");
        }
        break;
    }
    case 32:
        timer_tick(tf);
        break;
    case 33:
        keyboard_irq();
        outb(0x20, 0x20);
        break;
    case 0x80:
        syscall_handle(tf);
        break;
    }
}
