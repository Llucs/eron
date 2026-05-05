#include "../include/idt.h"
#include <stddef.h>
#include "../include/vga.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load();
extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

extern void isr0(void);
extern void isr6(void);
extern void isr13(void);
extern void isr14(void);

/* External references to exception handlers */
extern void isr8(void);   /* Double Fault */
extern void isr9(void);   /* Segment Not Present */
extern void isr11(void); /* Segment Not Present */
extern void isr12(void); /* Stack Fault */
extern void isr13(void); /* General Protection Fault */
extern void isr14(void); /* Page Fault */

void* memset(void* dest, int val, size_t len) {
    unsigned char* ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = (unsigned char)val;
    return dest;
}

/* Handle critical exceptions - UNUSED for now but useful for debugging */
/* static void handle_exception(const char* name, uint32_t error_code) { */
/*     /\* Disable interrupts - we're in a bad state *\/ */
/*     asm volatile("cli"); */
/*     terminal_setcolor(0x0C); /\* Red text *\/ */
/*     terminal_writestring("\nKERNEL PANIC: "); */
/*     terminal_writestring(name); */
/*     for (;;) { asm volatile("hlt"); } */
/* } */

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    idt_load();
}
