#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_PROGRAMS 32
#define PROG_NAME_LEN 16
#define PROG_DESC_LEN 48

typedef int (*program_fn)(int argc, char* argv[]);

struct program {
    char name[PROG_NAME_LEN];
    char desc[PROG_DESC_LEN];
    program_fn entry;
    uint8_t active;
};

void programs_init(void);
int program_register(const char* name, const char* desc, program_fn entry);
struct program* program_find(const char* name);
int program_count(void);
struct program* program_table(void);

#endif
