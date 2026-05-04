#include "../include/tss.h"
#include "../include/gdt.h"

static struct tss_entry tss;

extern void tss_flush(void);

void tss_init(uint32_t ss0, uint32_t esp0) {
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    gdt_set_gate(5, base, limit, 0x89, 0x00);

    uint8_t* p = (uint8_t*)&tss;
    for (uint32_t i = 0; i < sizeof(tss); i++)
        p[i] = 0;

    tss.ss0  = ss0;
    tss.esp0 = esp0;
    tss.iomap_base = sizeof(tss);

    tss_flush();
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}
