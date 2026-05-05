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
#define SYS_FORK   14
#define SYS_KILL   15
#define SYS_ALARM  16
#define SYS_SLEEP  17
#define SYS_GETTID 18
#define SYS_GETPGID 19
#define SYS_SETPGID 20
#define SYS_GETUID 21
#define SYS_GETGID 22
#define SYS_GETPPID 23
#define SYS_DUP    24
#define SYS_DUP2   25
#define SYS_PIPE   26
#define SYS_READLINK 27
#define SYS_SYMLINK 28
#define SYS_UNLINK 29
#define SYS_MKDIR  30
#define SYS_RMDIR  31
#define SYS_CHMOD  32
#define SYS_CHOWN  33
#define SYS_UMASK  34
#define SYS_TIMES  35
#define SYS_BRK    36
#define SYS_SETSID 37
#define SYS_GETSID 38
#define SYS_GETGROUPS 39
#define SYS_SETGROUPS 40
#define SYS_GETPGRP  41
#define SYS_SETPGRP  42
#define SYS_SETUID  43
#define SYS_SETGID  44
#define SYS_GETEUID 45
#define SYS_GETEGID 46
#define SYS_IOCTL  47
#define SYS_ACCESS 48
#define SYS_RENAME 49
#define SYS_MKNOD  50
#define SYS_LINK   51
#define SYS_TRUNCATE 52
#define SYS_FTRUNCATE 53
#define SYS_GETCWD 54
#define SYS_CAPGET 55
#define SYS_CAPSET 56
#define SYS_LSEEK  57
#define SYS_MAX    60

struct trapframe;

void syscall_init(void);
void syscall_handle(struct trapframe* tf);

#endif
