#include "../include/mm.h"

struct block_header {
    uint32_t size;
    uint32_t magic;
    uint8_t used;
};

#define BLOCK_MAGIC 0xEA0BEEF

static uint32_t heap_start;
static uint32_t heap_end;
static uint32_t heap_pos;

void mm_init(uint32_t start, uint32_t size) {
    heap_start = (start + 7) & ~7;
    heap_end = heap_start + size;
    heap_pos = heap_start;
}

void* kmalloc(size_t size) {
    if (size == 0) return (void*)0;
    size = (size + 7) & ~7;
    uint32_t total = size + sizeof(struct block_header);

    if (heap_pos + total > heap_end)
        return (void*)0;

    struct block_header* hdr = (struct block_header*)heap_pos;
    hdr->size = size;
    hdr->magic = BLOCK_MAGIC;
    hdr->used = 1;

    void* ptr = (void*)(heap_pos + sizeof(struct block_header));
    heap_pos += total;
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    struct block_header* hdr = (struct block_header*)((uint32_t)ptr - sizeof(struct block_header));
    if (hdr->magic == BLOCK_MAGIC)
        hdr->used = 0;
}

size_t mm_used(void) {
    return heap_pos - heap_start;
}

size_t mm_free(void) {
    return heap_end - heap_pos;
}

size_t mm_total(void) {
    return heap_end - heap_start;
}
