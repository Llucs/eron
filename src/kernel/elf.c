#include "../include/elf.h"
#include <stddef.h>

/* User space memory boundaries */
#define USER_SPACE_START 0x08000000
#define USER_SPACE_END   0xC0000000

/* Kernel space boundaries */
#define KERNEL_SPACE_START 0xC0000000

int elf_validate(const uint8_t* data, uint32_t size) {
    if (size < sizeof(struct elf32_ehdr))
        return -1;
    const struct elf32_ehdr* eh = (const struct elf32_ehdr*)data;
    if (eh->e_magic != ELF_MAGIC)   return -1;
    if (eh->e_class != 1)           return -1;  /* 32-bit */
    if (eh->e_data  != 1)           return -1;  /* little-endian */
    if (eh->e_type  != ET_EXEC)     return -1;
    if (eh->e_machine != EM_386)    return -1;
    return 0;
}

/* Validate address is in allowed user space range */
static int validate_load_addr(uint32_t addr, uint32_t size) {
    if (addr == 0) return 0;
    if (addr < USER_SPACE_START) return 0;
    if (addr + size > USER_SPACE_END) return 0;
    /* Make sure it doesn't overlap kernel space */
    if (addr >= KERNEL_SPACE_START) return 0;
    return 1;
}

uint32_t elf_load(const uint8_t* data, uint32_t size) {
    if (elf_validate(data, size) != 0)
        return 0;

    const struct elf32_ehdr* eh = (const struct elf32_ehdr*)data;
    if (eh->e_phoff == 0 || eh->e_phnum == 0)
        return 0;

    /* Check for too many program headers (DoS protection) */
    if (eh->e_phnum > 16)
        return 0;

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const struct elf32_phdr* ph =
            (const struct elf32_phdr*)(data + eh->e_phoff +
                                      i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_offset + ph->p_filesz > size)
            return 0;
        if (ph->p_vaddr == 0 || ph->p_memsz == 0)
            continue;

        /* SECURITY: Validate load address is in user space */
        if (!validate_load_addr(ph->p_vaddr, ph->p_memsz))
            return 0;

        /* SECURITY: Check for overlapping segments */
        for (uint16_t j = 0; j < i; j++) {
            const struct elf32_phdr* prev =
                (const struct elf32_phdr*)(data + eh->e_phoff +
                                          j * eh->e_phentsize);
            if (prev->p_type != PT_LOAD)
                continue;
            uint32_t prev_end = prev->p_vaddr + prev->p_memsz;
            uint32_t cur_end = ph->p_vaddr + ph->p_memsz;
            if ((ph->p_vaddr >= prev->p_vaddr && ph->p_vaddr < prev_end) ||
                (prev->p_vaddr >= ph->p_vaddr && prev->p_vaddr < cur_end)) {
                return 0; /* Overlap detected */
            }
        }

        uint8_t* dest = (uint8_t*)ph->p_vaddr;
        const uint8_t* src = data + ph->p_offset;
        for (uint32_t j = 0; j < ph->p_filesz; j++)
            dest[j] = src[j];
        for (uint32_t j = ph->p_filesz; j < ph->p_memsz; j++)
            dest[j] = 0;
    }
    return eh->e_entry;
}
