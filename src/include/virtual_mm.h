#ifndef VIRTUAL_MM_H
#define VIRTUAL_MM_H

#include <stdint.h>

#define VIRT_ADDR_START  0x08000000
#define VIRT_ADDR_END   0xC0000000
#define KERNEL_VIRT_START 0xC0000000

#define PAGE_SIZE      4096
#define PAGE_MASK      (~(PAGE_SIZE - 1))

#define MAX_PAGE_DIRS  4
#define MAX_PAGE_TABLES 1024

#define PG_PRESENT     0x01
#define PG_WRITABLE    0x02
#define PG_USER        0x04
#define PG_ACCESSED    0x20
#define PG_DIRTY       0x40
#define PG_COW         0x80

struct page_directory {
    uint32_t entries[1024] __attribute__((aligned(4096)));
};

struct page_table {
    uint32_t entries[1024] __attribute__((aligned(4096)));
};

typedef uint32_t phys_addr_t;
typedef uint32_t virt_addr_t;

void vmm_init(phys_addr_t kernel_end);
int vmm_alloc_page(virt_addr_t virt);
void vmm_free_page(virt_addr_t virt);
int vmm_map_user_pages(virt_addr_t virt, uint32_t count);
void vmm_switch_pdir(struct page_directory* pdir);
struct page_directory* vmm_get_current_pdir(void);
int vmm_copy_on_write(void);
void* vmm_alloc_user_heap(uint32_t size);
void vmm_free_user_heap(void* ptr, uint32_t size);

phys_addr_t vmm_get_phys_from_virt(virt_addr_t virt);
int vmm_is_mapped(virt_addr_t virt);

#endif