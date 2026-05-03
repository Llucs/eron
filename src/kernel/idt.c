#include "../include/idt.h"
#include <stddef.h>

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load();

void* memset(void* dest, int val, size_t len) {
    unsigned char* ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = (unsigned char)val;
    return dest;
}

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
