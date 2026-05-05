#include "../include/mm.h"
#include <stddef.h>

struct block_header {
    uint32_t size;
    uint32_t magic;
    uint8_t used;
};

#define BLOCK_MAGIC 0xEA0BEEF
#define BLOCK_FREE  0x00
#define BLOCK_USED 0x01

static uint32_t heap_start;
static uint32_t heap_end;
static uint32_t heap_pos;

/* Track allocated blocks for validation */
#define MAX_TRACKED_BLOCKS 64
static struct tracked_block {
    void* ptr;
    uint32_t size;
    uint8_t in_use;
} tracked_blocks[MAX_TRACKED_BLOCKS];

/* tracked_count removed - was unused */

static void track_add(void* ptr, uint32_t size) {
    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        if (!tracked_blocks[i].in_use) {
            tracked_blocks[i].ptr = ptr;
            tracked_blocks[i].size = size;
            tracked_blocks[i].in_use = 1;
            return;
        }
    }
}

static int track_remove(void* ptr) {
    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        if (tracked_blocks[i].in_use && tracked_blocks[i].ptr == ptr) {
            tracked_blocks[i].in_use = 0;
            return 1;
        }
    }
    return 0;
}

static int track_validate(void* ptr) {
    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        if (tracked_blocks[i].in_use && tracked_blocks[i].ptr == ptr) {
            return 1;
        }
    }
    return 0;
}

void mm_init(uint32_t start, uint32_t size) {
    heap_start = (start + 7) & ~7;
    heap_end = heap_start + size;
    heap_pos = heap_start;
    
    /* Initialize tracking table */
    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        tracked_blocks[i].in_use = 0;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return (void*)0;
    
    /* Check for reasonable allocation size */
    if (size > (heap_end - heap_start) / 4) {
        return (void*)0;
    }
    
    size = (size + 7) & ~7;
    uint32_t total = size + sizeof(struct block_header);

    if (heap_pos + total > heap_end)
        return (void*)0;

    struct block_header* hdr = (struct block_header*)heap_pos;
    hdr->size = size;
    hdr->magic = BLOCK_MAGIC;
    hdr->used = BLOCK_USED;

    void* ptr = (void*)(heap_pos + sizeof(struct block_header));
    heap_pos += total;
    
    /* Track this allocation */
    track_add(ptr, size);
    
    return ptr;
}

int kfree(void* ptr) {
    if (!ptr) return 0;
    
    /* Validate pointer before freeing */
    if (!track_validate(ptr)) {
        return 0;
    }
    
    struct block_header* hdr = (struct block_header*)((uint32_t)ptr - sizeof(struct block_header));
    
    /* Sanity check: verify magic and bounds */
    if (hdr->magic != BLOCK_MAGIC) {
        return 0;
    }
    
    /* Check if within heap bounds */
    uint32_t hdr_addr = (uint32_t)hdr;
    if (hdr_addr < heap_start || hdr_addr >= heap_end) {
        return 0;
    }
    
    uint32_t ptr_end = hdr_addr + sizeof(struct block_header) + hdr->size;
    if (ptr_end > heap_end) {
        return 0;
    }
    
    /* Mark block as freed and remove from tracking */
    hdr->used = BLOCK_FREE;
    track_remove(ptr);
    
    return 1;
}

int kfree_all(void) {
    int freed = 0;
    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        if (tracked_blocks[i].in_use) {
            kfree(tracked_blocks[i].ptr);
            freed++;
        }
    }
    return freed;
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
