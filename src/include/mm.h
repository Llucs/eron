#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_SIZE 0x100000  /* 1 MB */

void mm_init(uint32_t start, uint32_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
size_t mm_used(void);
size_t mm_free(void);
size_t mm_total(void);

#endif
