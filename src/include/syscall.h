#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_EXIT    3
#define SYS_GETPID  4
#define SYS_MALLOC  5
#define SYS_FREE    6
#define SYS_OPEN    7
#define SYS_CLOSE   8
#define SYS_TIME    9
#define SYS_YIELD  10
#define SYS_EXEC   11
#define SYS_WAIT   12
#define SYS_STAT   13
#define SYS_MAX    16

struct trapframe;

void syscall_init(void);
void syscall_handle(struct trapframe* tf);

#endif
