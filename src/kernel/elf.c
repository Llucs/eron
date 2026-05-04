#include "../include/elf.h"

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

uint32_t elf_load(const uint8_t* data, uint32_t size) {
    if (elf_validate(data, size) != 0)
        return 0;

    const struct elf32_ehdr* eh = (const struct elf32_ehdr*)data;
    if (eh->e_phoff == 0 || eh->e_phnum == 0)
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

        uint8_t* dest = (uint8_t*)ph->p_vaddr;
        const uint8_t* src = data + ph->p_offset;
        for (uint32_t j = 0; j < ph->p_filesz; j++)
            dest[j] = src[j];
        for (uint32_t j = ph->p_filesz; j < ph->p_memsz; j++)
            dest[j] = 0;
    }
    return eh->e_entry;
}
