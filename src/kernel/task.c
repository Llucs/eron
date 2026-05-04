#include "../include/task.h"
#include <stddef.h>

static struct program prog_table[MAX_PROGRAMS];
static int prog_count = 0;

static void task_strcpy(char* dst, const char* src, size_t max) {
    size_t i;
    for (i = 0; i < max - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

static int task_strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

void programs_init(void) {
    for (int i = 0; i < MAX_PROGRAMS; i++)
        prog_table[i].active = 0;
    prog_count = 0;
}

int program_register(const char* name, const char* desc, program_fn entry) {
    if (prog_count >= MAX_PROGRAMS) return -1;

    struct program* p = &prog_table[prog_count++];
    task_strcpy(p->name, name, PROG_NAME_LEN);
    task_strcpy(p->desc, desc, PROG_DESC_LEN);
    p->entry = entry;
    p->active = 1;
    return 0;
}

struct program* program_find(const char* name) {
    for (int i = 0; i < prog_count; i++) {
        if (prog_table[i].active && task_strcmp(prog_table[i].name, name) == 0)
            return &prog_table[i];
    }
    return (void*)0;
}

int program_count(void) {
    return prog_count;
}

struct program* program_table(void) {
    return prog_table;
}
