#include "../include/virtual_mm.h"
#include "../include/mm.h"
#include "../include/process.h"
#include <stddef.h>
#include <stdbool.h>

static struct page_directory* kernel_pdir;
static struct page_directory* current_pdir;
static phys_addr_t phys_base;
static uint32_t total_pages;

/* Removed unused: page_tables_free, free_table_count */

static phys_addr_t next_free_page;
static phys_addr_t user_heap_start;
static phys_addr_t user_heap_pos;
static phys_addr_t user_heap_end;

static void* user_heap_base;

static phys_addr_t align_up(phys_addr_t addr, uint32_t align) {
    return (addr + align - 1) & ~(align - 1);
}

void vmm_init(phys_addr_t kernel_end) {
    kernel_end = align_up(kernel_end, 4096);
    phys_base = kernel_end + (16 * 1024 * 1024);
    
    kernel_pdir = (struct page_directory*)kmalloc(sizeof(struct page_directory));
    if (!kernel_pdir) {
        kernel_pdir = (struct page_directory*)kernel_end;
        kernel_end += sizeof(struct page_directory);
    }
    
    for (int i = 0; i < 1024; i++) {
        kernel_pdir->entries[i] = 0;
    }
    
    int pde_idx = KERNEL_VIRT_START >> 22;
    for (int i = pde_idx; i < 1024; i++) {
        struct page_table* pt = (struct page_table*)
            align_up(kernel_end + (i - pde_idx) * sizeof(struct page_table), 4096);
        
        kernel_pdir->entries[i] = ((phys_addr_t)pt) | PG_PRESENT | PG_WRITABLE;
        
        for (int j = 0; j < 1024; j++) {
            virt_addr_t virt = ((i << 22) | (j << 12));
            phys_addr_t phys = (virt - KERNEL_VIRT_START) + 0xC0000000;
            kernel_pdir->entries[i] = phys | PG_PRESENT | PG_WRITABLE;
        }
    }
    
    current_pdir = kernel_pdir;
    next_free_page = phys_base;
    total_pages = (16 * 1024 * 1024) / PAGE_SIZE;
    
    user_heap_start = VIRT_ADDR_START;
    user_heap_pos = VIRT_ADDR_START + (8 * 1024 * 1024);
    user_heap_end = VIRT_ADDR_END - (4 * 1024 * 1024);
    
    user_heap_base = (void*)0x09000000;
}

int vmm_alloc_page(virt_addr_t virt) {
    if (!current_pdir) return -1;
    
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (!current_pdir->entries[pde_idx]) {
        struct page_table* pt = (struct page_table*)kmalloc(sizeof(struct page_table));
        if (!pt) return -1;
        
        for (int i = 0; i < 1024; i++) {
            pt->entries[i] = 0;
        }
        
        current_pdir->entries[pde_idx] = ((phys_addr_t)pt) | PG_PRESENT | PG_WRITABLE | PG_USER;
    }
    
    struct page_table* pt = (struct page_table*)(current_pdir->entries[pde_idx] & PAGE_MASK);
    
    if (pt->entries[pte_idx] & PG_PRESENT) {
        return 0;
    }
    
    phys_addr_t phys_page = next_free_page;
    next_free_page += PAGE_SIZE;
    
    if (!phys_page || next_free_page > phys_base + (total_pages * PAGE_SIZE)) {
        return -1;
    }
    
    pt->entries[pte_idx] = phys_page | PG_PRESENT | PG_WRITABLE | PG_USER;
    
    return 0;
}

void vmm_free_page(virt_addr_t virt) {
    if (!current_pdir) return;
    
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (!current_pdir->entries[pde_idx]) return;
    
    struct page_table* pt = (struct page_table*)(current_pdir->entries[pde_idx] & PAGE_MASK);
    pt->entries[pte_idx] = 0;
}

int vmm_map_user_pages(virt_addr_t virt, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint32_t addr = virt + (i * PAGE_SIZE);
        if (vmm_alloc_page(addr)) {
            return -1;
        }
    }
    return 0;
}

void vmm_switch_pdir(struct page_directory* pdir) {
    current_pdir = pdir;
    phys_addr_t pdir_phys = (phys_addr_t)pdir;
    asm volatile("mov %0, %%cr3" : : "r"(pdir_phys));
}

struct page_directory* vmm_get_current_pdir(void) {
    return current_pdir;
}

void* vmm_alloc_user_heap(uint32_t size) {
    size = (size + 7) & ~7;
    
    if (user_heap_pos + size > user_heap_end) {
        return NULL;
    }
    
    void* result = (void*)user_heap_pos;
    user_heap_pos += size;
    
    vmm_map_user_pages((virt_addr_t)result, (size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    return result;
}

void vmm_free_user_heap(void* ptr, uint32_t size) {
    (void)ptr;
    (void)size;
}

phys_addr_t vmm_get_phys_from_virt(virt_addr_t virt) {
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (!current_pdir->entries[pde_idx]) return 0;
    
    struct page_table* pt = (struct page_table*)(current_pdir->entries[pde_idx] & PAGE_MASK);
    if (!(pt->entries[pte_idx] & PG_PRESENT)) return 0;
    
    return (pt->entries[pte_idx] & PAGE_MASK) | (virt & ~PAGE_MASK);
}

int vmm_is_mapped(virt_addr_t virt) {
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (!current_pdir->entries[pde_idx]) return 0;
    
    struct page_table* pt = (struct page_table*)(current_pdir->entries[pde_idx] & PAGE_MASK);
    return (pt->entries[pte_idx] & PG_PRESENT) ? 1 : 0;
}