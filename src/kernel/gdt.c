#include "../include/gdt.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gp;

extern void gdt_flush(uint32_t);

void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t gran)
{
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0,       0,    0);       /* null          */
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);    /* kernel code   */
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);    /* kernel data   */
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF);    /* user code     */
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);    /* user data     */
    /* entry 5 (TSS) set up by tss_init */

    gdt_flush((uint32_t)&gp);
}
