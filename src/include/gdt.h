#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_ENTRIES 6

#define SEG_KCODE 0x08
#define SEG_KDATA 0x10
#define SEG_UCODE 0x18
#define SEG_UDATA 0x20
#define SEG_TSS   0x28

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif
