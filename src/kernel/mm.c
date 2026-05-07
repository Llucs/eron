#include "../include/mm.h"
#include <stddef.h>

struct block_header {
    uint32_t size;
    uint32_t magic;
    uint8_t used;
};

#define BLOCK_MAGIC 0xEA0BEEF
#define BLOCK_FREE  0x00
#define BLOCK_USED  0x01
#define ALIGN8(x)   (((x) + 7) & ~7)
#define MIN_SPLIT_PAYLOAD 16

static uint32_t heap_start;
static uint32_t heap_end;

/* Track allocated blocks for pointer validation */
#define MAX_TRACKED_BLOCKS 256
static struct tracked_block {
    void* ptr;
    uint32_t size;
    uint8_t in_use;
} tracked_blocks[MAX_TRACKED_BLOCKS];

static uint32_t block_total_size(const struct block_header* hdr) {
    return sizeof(struct block_header) + hdr->size;
}

static struct block_header* first_block(void) {
    return (struct block_header*)heap_start;
}

static struct block_header* next_block(struct block_header* hdr) {
    uint32_t addr = (uint32_t)hdr + block_total_size(hdr);
    if (addr >= heap_end) return (struct block_header*)0;
    return (struct block_header*)addr;
}

static int is_block_valid(const struct block_header* hdr) {
    uint32_t addr = (uint32_t)hdr;
    if (addr < heap_start || addr + sizeof(struct block_header) > heap_end)
        return 0;
    if (hdr->magic != BLOCK_MAGIC)
        return 0;
    if (addr + block_total_size(hdr) > heap_end)
        return 0;
    return 1;
}

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

static void split_block(struct block_header* hdr, uint32_t wanted_payload) {
    uint32_t remaining_payload = hdr->size - wanted_payload;
    uint32_t minimum_needed = sizeof(struct block_header) + MIN_SPLIT_PAYLOAD;

    if (remaining_payload <= minimum_needed) return;

    struct block_header* new_hdr = (struct block_header*)((uint32_t)hdr + sizeof(struct block_header) + wanted_payload);
    new_hdr->size = remaining_payload - sizeof(struct block_header);
    new_hdr->magic = BLOCK_MAGIC;
    new_hdr->used = BLOCK_FREE;

    hdr->size = wanted_payload;
}

static void coalesce_free_blocks(void) {
    struct block_header* hdr = first_block();

    while ((uint32_t)hdr < heap_end) {
        if (!is_block_valid(hdr)) return;

        struct block_header* nxt = next_block(hdr);
        if (!nxt) return;

        if (!is_block_valid(nxt)) return;

        if (hdr->used == BLOCK_FREE && nxt->used == BLOCK_FREE) {
            hdr->size += sizeof(struct block_header) + nxt->size;
            continue;
        }

        hdr = nxt;
    }
}

void mm_init(uint32_t start, uint32_t size) {
    heap_start = ALIGN8(start);
    heap_end = heap_start + size;

    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        tracked_blocks[i].in_use = 0;
    }

    struct block_header* hdr = first_block();
    hdr->size = heap_end - heap_start - sizeof(struct block_header);
    hdr->magic = BLOCK_MAGIC;
    hdr->used = BLOCK_FREE;
}

void* kmalloc(size_t size) {
    if (size == 0) return (void*)0;

    uint32_t wanted = ALIGN8((uint32_t)size);

    struct block_header* hdr = first_block();
    while ((uint32_t)hdr < heap_end) {
        if (!is_block_valid(hdr)) return (void*)0;

        if (hdr->used == BLOCK_FREE && hdr->size >= wanted) {
            split_block(hdr, wanted);
            hdr->used = BLOCK_USED;

            void* ptr = (void*)((uint32_t)hdr + sizeof(struct block_header));
            track_add(ptr, hdr->size);
            return ptr;
        }

        struct block_header* nxt = next_block(hdr);
        if (!nxt) break;
        hdr = nxt;
    }

    return (void*)0;
}

int kfree(void* ptr) {
    if (!ptr) return 0;
    if (!track_validate(ptr)) return 0;

    struct block_header* hdr = (struct block_header*)((uint32_t)ptr - sizeof(struct block_header));
    if (!is_block_valid(hdr)) return 0;

    if (!track_remove(ptr)) return 0;

    hdr->used = BLOCK_FREE;
    coalesce_free_blocks();
    return 1;
}

int kfree_all(void) {
    int freed = 0;
    for (int i = 0; i < MAX_TRACKED_BLOCKS; i++) {
        if (tracked_blocks[i].in_use) {
            if (kfree(tracked_blocks[i].ptr)) freed++;
        }
    }
    return freed;
}

size_t mm_used(void) {
    size_t used = 0;
    struct block_header* hdr = first_block();

    while ((uint32_t)hdr < heap_end) {
        if (!is_block_valid(hdr)) break;
        if (hdr->used == BLOCK_USED)
            used += hdr->size;

        struct block_header* nxt = next_block(hdr);
        if (!nxt) break;
        hdr = nxt;
    }

    return used;
}

size_t mm_free(void) {
    size_t free_bytes = 0;
    struct block_header* hdr = first_block();

    while ((uint32_t)hdr < heap_end) {
        if (!is_block_valid(hdr)) break;
        if (hdr->used == BLOCK_FREE)
            free_bytes += hdr->size;

        struct block_header* nxt = next_block(hdr);
        if (!nxt) break;
        hdr = nxt;
    }

    return free_bytes;
}

size_t mm_total(void) {
    return heap_end - heap_start - sizeof(struct block_header);
}
