#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/vga.h"
#include "../include/config.h"
#include "../include/io.h"
#include "../include/task.h"
#include "../include/vfs.h"
#include "../include/mm.h"
#include "../include/timer.h"
#include "../include/display.h"
#include "../include/process.h"
#include "../include/syscall.h"
#include "../include/elf.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern void terminal_clear(void);
extern const char* vfs_basename(const char* path);

static char cmd_buffer[128];
static int cmd_index = 0;

struct service_entry {
    const char* name;
    const char* description;
    uint8_t enabled;
    uint8_t running;
};

static struct service_entry services[] = {
    {"netd", "network bootstrap daemon", 1, 0},
    {"sshd", "remote shell daemon", 0, 0},
    {"audiod", "audio mixer daemon", 0, 0},
    {"guid", "graphical session daemon", 0, 0}
};

#define SERVICE_COUNT (sizeof(services) / sizeof(services[0]))

static int str_cmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

static int str_ncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

static size_t str_len(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void uint_to_str(uint32_t val, char* buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    int i = 0;
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

static void print_num(uint32_t val) {
    char buf[12];
    uint_to_str(val, buf);
    terminal_writestring(buf);
}

static void print_num_padded(uint32_t val, int width) {
    char buf[12];
    uint_to_str(val, buf);
    int len = (int)str_len(buf);
    for (int i = 0; i < width - len; i++)
        terminal_writestring(" ");
    terminal_writestring(buf);
}

static uint8_t read_cmos(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* ── prompt ───────────────────────────────────────────────── */

void print_prompt(void) {
    uint8_t name_c = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t accent = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    terminal_setcolor(name_c);
    terminal_writestring(ERON_USER);
    terminal_setcolor(accent);
    terminal_writestring("@");
    terminal_setcolor(name_c);
    terminal_writestring(ERON_HOSTNAME);
    terminal_setcolor(body);
    terminal_writestring(":/ ");
    terminal_setcolor(accent);
    terminal_writestring("$ ");
}

/* ── embedded user-mode programs ──────────────────────────── */

static void __attribute__((used)) user_hello(void) {
    asm volatile(
        "mov $1, %%eax\n"
        "mov $1, %%ebx\n"
        "lea 1f, %%ecx\n"
        "int $0x80\n"
        "mov $3, %%eax\n"
        "xor %%ebx, %%ebx\n"
        "int $0x80\n"
        "jmp .\n"
        "1: .asciz \"Hello from user mode! (ring 3)\\n\"\n"
        ::: "eax", "ebx", "ecx", "memory"
    );
}

static void __attribute__((used)) user_loop(void) {
    asm volatile(
        "mov $1, %%eax\n"
        "mov $1, %%ebx\n"
        "lea 1f, %%ecx\n"
        "int $0x80\n"
        "mov $3, %%eax\n"
        "xor %%ebx, %%ebx\n"
        "int $0x80\n"
        "jmp .\n"
        "1: .asciz \"Loop finished (preempted and exited)\\n\"\n"
        ::: "eax", "ebx", "ecx", "memory"
    );
}

static void __attribute__((used)) user_crash(void) {
    /* SECURITY: This now triggers a safe user-mode exception instead of halting the system */
    /* Trigger a divide-by-zero which will be caught by the kernel */
    asm volatile(
        "mov $0, %%ebx\n"
        "mov $1, %%eax\n"
        "div %%ebx\n"
        ::: "eax", "ebx", "edx", "memory"
    );
}

/* ── registered programs ──────────────────────────────────── */

static int prog_help(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint8_t hl = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t cmd_c = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(hl);
    terminal_writestring("\nEron Shell (" ERON_SHELL " " ERON_SHELL_VER ")\n\n");

    struct program* table = program_table();
    int count = program_count();

    for (int i = 0; i < count; i++) {
        if (!table[i].active) continue;
        terminal_setcolor(cmd_c);
        terminal_writestring(" ");
        terminal_writestring(table[i].name);
        int pad = 14 - (int)str_len(table[i].name);
        terminal_setcolor(body);
        for (int j = 0; j < pad; j++) terminal_writestring(" ");
        terminal_writestring(table[i].desc);
        terminal_writestring("\n");
    }
    return 0;
}

static int prog_about(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    *((uint32_t*)&vendor[0]) = ebx;
    *((uint32_t*)&vendor[4]) = edx;
    *((uint32_t*)&vendor[8]) = ecx;
    vendor[12] = '\0';

    uint8_t label = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t val = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t dim_c = vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(label);
    terminal_writestring("\nEron OS ");
    terminal_setcolor(val);
    terminal_writestring(ERON_VERSION " (" ERON_CODENAME ")\n");
    terminal_setcolor(dim_c);
    terminal_writestring("─────────────────────────────\n");

    terminal_setcolor(label);
    terminal_writestring(" Kernel    ");
    terminal_setcolor(val);
    terminal_writestring("eron-i386\n");

    terminal_setcolor(label);
    terminal_writestring(" Arch      ");
    terminal_setcolor(val);
    terminal_writestring("i386 (32-bit)\n");

    terminal_setcolor(label);
    terminal_writestring(" CPU       ");
    terminal_setcolor(val);
    terminal_writestring(vendor);
    terminal_writestring("\n");

    terminal_setcolor(label);
    terminal_writestring(" Memory    ");
    terminal_setcolor(val);
    print_num((uint32_t)(mm_total() / 1024));
    terminal_writestring(" kB heap (");
    print_num((uint32_t)(mm_free() / 1024));
    terminal_writestring(" kB free)\n");

    terminal_setcolor(label);
    terminal_writestring(" Uptime    ");
    terminal_setcolor(val);
    print_num(timer_uptime_minutes());
    terminal_writestring("m ");
    print_num(timer_uptime_seconds());
    terminal_writestring("s\n");

    terminal_setcolor(label);
    terminal_writestring(" Shell     ");
    terminal_setcolor(val);
    terminal_writestring(ERON_SHELL " " ERON_SHELL_VER "\n");

    terminal_setcolor(label);
    terminal_writestring(" Display   ");
    terminal_setcolor(val);
    terminal_writestring("VGA 80x25\n");

    terminal_setcolor(label);
    terminal_writestring(" Processes ");
    terminal_setcolor(val);
    print_num((uint32_t)proc_active_count());
    terminal_writestring(" active\n");

    terminal_setcolor(label);
    terminal_writestring(" Syscall   ");
    terminal_setcolor(val);
    terminal_writestring("INT 0x80\n");

    terminal_setcolor(dim_c);
    terminal_writestring(" Author    " ERON_AUTHOR "\n");

    return 0;
}

static int prog_fastfetch(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint8_t logo = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t key = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t val = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(logo);
    terminal_writestring("\n      /\\\n");
    terminal_writestring("     /  \\    ");
    terminal_setcolor(key); terminal_writestring("OS:      "); terminal_setcolor(val); terminal_writestring("EronOS\n");
    terminal_setcolor(logo); terminal_writestring("    / /\\ \\   ");
    terminal_setcolor(key); terminal_writestring("Host:    "); terminal_setcolor(val); terminal_writestring(ERON_HOSTNAME "\n");
    terminal_setcolor(logo); terminal_writestring("   / ____ \\  ");
    terminal_setcolor(key); terminal_writestring("Kernel:  "); terminal_setcolor(val); terminal_writestring(ERON_VERSION "-" ERON_CODENAME "\n");
    terminal_setcolor(logo); terminal_writestring("  /_/    \\_\\ ");
    terminal_setcolor(key); terminal_writestring("Shell:   "); terminal_setcolor(val); terminal_writestring(ERON_SHELL " " ERON_SHELL_VER "\n");
    terminal_setcolor(key); terminal_writestring("              Uptime:  "); terminal_setcolor(val); print_num(timer_uptime_hours()); terminal_writestring("h "); print_num(timer_uptime_minutes()); terminal_writestring("m\n");
    terminal_setcolor(key); terminal_writestring("              Memory:  "); terminal_setcolor(val); print_num((uint32_t)(mm_used() / 1024)); terminal_writestring("/"); print_num((uint32_t)(mm_total() / 1024)); terminal_writestring(" kB\n");
    terminal_setcolor(key); terminal_writestring("              Procs:   "); terminal_setcolor(val); print_num((uint32_t)proc_active_count()); terminal_writestring("\n");
    return 0;
}

static int prog_roadmap(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\nEronOS next-level roadmap:");
    terminal_writestring("\n [1] Stable userland ABI + libc subset");
    terminal_writestring("\n [2] ELF loader hardening + per-process VM");
    terminal_writestring("\n [3] Storage stack: initrd + ext2 driver");
    terminal_writestring("\n [4] Networking: PCI probe + e1000 + TCP/IP");
    terminal_writestring("\n [5] Graphics: VBE framebuffer + compositor");
    terminal_writestring("\n [6] Package manager + signed repos");
    return 0;
}

static struct service_entry* service_find(const char* name) {
    for (size_t i = 0; i < SERVICE_COUNT; i++) {
        if (str_cmp(services[i].name, name) == 0)
            return &services[i];
    }
    return NULL;
}

static void service_print_status(struct service_entry* svc) {
    terminal_writestring(" ");
    terminal_writestring(svc->name);
    terminal_writestring("  ");
    terminal_writestring(svc->running ? "running" : "stopped");
    terminal_writestring("  ");
    terminal_writestring(svc->enabled ? "enabled" : "disabled");
    terminal_writestring("  ");
    terminal_writestring(svc->description);
    terminal_writestring("\n");
}

static int prog_service(int argc, char* argv[]) {
    if (argc == 1 || str_cmp(argv[1], "status") == 0) {
        terminal_writestring("\nSERVICE   STATE    BOOT      DESCRIPTION\n");
        for (size_t i = 0; i < SERVICE_COUNT; i++) service_print_status(&services[i]);
        return 0;
    }

    if (argc < 3) {
        terminal_writestring("\nservice: usage: service <start|stop|enable|disable|status> <name>");
        return 1;
    }

    struct service_entry* svc = service_find(argv[2]);
    if (!svc) {
        terminal_writestring("\nservice: unknown service");
        return 1;
    }

    if (str_cmp(argv[1], "start") == 0) svc->running = 1;
    else if (str_cmp(argv[1], "stop") == 0) svc->running = 0;
    else if (str_cmp(argv[1], "enable") == 0) svc->enabled = 1;
    else if (str_cmp(argv[1], "disable") == 0) svc->enabled = 0;
    else {
        terminal_writestring("\nservice: invalid action");
        return 1;
    }

    terminal_writestring("\n");
    service_print_status(svc);
    return 0;
}

static int prog_touch(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_writestring("\ntouch: usage: touch <file>");
        return 1;
    }
    if (vfs_lookup(argv[1])) return 0;
    if (vfs_mkfile(argv[1], "") < 0) {
        terminal_writestring("\ntouch: cannot create file");
        return 1;
    }
    return 0;
}

static int prog_write(int argc, char* argv[]) {
    if (argc < 3) {
        terminal_writestring("\nwrite: usage: write <file> <text>");
        return 1;
    }

    if (!vfs_lookup(argv[1])) {
        if (vfs_mkfile(argv[1], "") < 0) {
            terminal_writestring("\nwrite: cannot create file");
            return 1;
        }
    }

    char buf[512];
    int p = 0;
    for (int i = 2; i < argc && p < (int)sizeof(buf) - 1; i++) {
        if (i > 2 && p < (int)sizeof(buf) - 1) buf[p++] = ' ';
        for (int j = 0; argv[i][j] && p < (int)sizeof(buf) - 1; j++)
            buf[p++] = argv[i][j];
    }
    buf[p] = '\0';

    if (vfs_write(argv[1], buf, (size_t)p) < 0) {
        terminal_writestring("\nwrite: failed");
        return 1;
    }
    return 0;
}

static int prog_rm(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_writestring("\nrm: usage: rm <path>");
        return 1;
    }
    if (vfs_remove(argv[1]) < 0) {
        terminal_writestring("\nrm: cannot remove");
        return 1;
    }
    return 0;
}

static int prog_mv(int argc, char* argv[]) {
    if (argc < 3) {
        terminal_writestring("\nmv: usage: mv <src> <dst>");
        return 1;
    }
    if (vfs_rename(argv[1], argv[2]) < 0) {
        terminal_writestring("\nmv: failed");
        return 1;
    }
    return 0;
}

static int prog_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_writestring("\nmkdir: usage: mkdir <dir>");
        return 1;
    }
    if (vfs_mkdir(argv[1]) < 0) {
        terminal_writestring("\nmkdir: failed");
        return 1;
    }
    return 0;
}

static int prog_stat(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_writestring("\nstat: usage: stat <path>");
        return 1;
    }
    struct vfs_node* n = vfs_lookup(argv[1]);
    if (!n) {
        terminal_writestring("\nstat: not found");
        return 1;
    }
    terminal_writestring("\nPath: ");
    terminal_writestring(n->path);
    terminal_writestring("\nType: ");
    if (n->type == VFS_DIR) terminal_writestring("dir");
    else if (n->type == VFS_FILE) terminal_writestring("file");
    else if (n->type == VFS_DEV) terminal_writestring("dev");
    else terminal_writestring("other");
    terminal_writestring("\nSize: ");
    print_num(n->size);
    terminal_writestring("\nPerm: ");
    terminal_writestring((n->perm & VFS_PERM_READ) ? "r" : "-");
    terminal_writestring((n->perm & VFS_PERM_WRITE) ? "w" : "-");
    terminal_writestring((n->perm & VFS_PERM_EXEC) ? "x" : "-");
    return 0;
}

static int prog_chmod(int argc, char* argv[]) {
    if (argc < 3) {
        terminal_writestring("\nchmod: usage: chmod <0-7> <path>");
        return 1;
    }

    int mode = argv[1][0] - '0';
    if (argv[1][0] < '0' || argv[1][0] > '7' || argv[1][1] != '\0') {
        terminal_writestring("\nchmod: invalid mode");
        return 1;
    }

    uint16_t perm = 0;
    if (mode & 4) perm |= VFS_PERM_READ;
    if (mode & 2) perm |= VFS_PERM_WRITE;
    if (mode & 1) perm |= VFS_PERM_EXEC;

    if (vfs_chmod(argv[2], perm) < 0) {
        terminal_writestring("\nchmod: failed");
        return 1;
    }
    return 0;
}

static int prog_uname(int argc, char* argv[]) {
    if (argc > 1 && str_cmp(argv[1], "-a") == 0) {
        terminal_writestring("\nEron " ERON_HOSTNAME " " ERON_VERSION "-" ERON_CODENAME " i386 EronOS");
    } else if (argc > 1 && str_cmp(argv[1], "-r") == 0) {
        terminal_writestring("\n" ERON_VERSION);
    } else {
        terminal_writestring("\nEron");
    }
    return 0;
}

static int prog_uptime(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint32_t h = timer_uptime_hours();
    uint32_t m = timer_uptime_minutes();
    uint32_t s = timer_uptime_seconds();

    terminal_writestring("\nup ");
    print_num(h);
    terminal_writestring(":");
    if (m < 10) terminal_writestring("0");
    print_num(m);
    terminal_writestring(":");
    if (s < 10) terminal_writestring("0");
    print_num(s);
    return 0;
}

static int prog_date(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint8_t sec = bcd_to_bin(read_cmos(0x00));
    uint8_t min = bcd_to_bin(read_cmos(0x02));
    uint8_t hour = bcd_to_bin(read_cmos(0x04));
    uint8_t day = bcd_to_bin(read_cmos(0x07));
    uint8_t month = bcd_to_bin(read_cmos(0x08));
    uint8_t year = bcd_to_bin(read_cmos(0x09));

    static const char* months[] = {
        "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    terminal_writestring("\n");
    if (month >= 1 && month <= 12) terminal_writestring(months[month]);
    else terminal_writestring("???");
    terminal_writestring(" ");
    if (day < 10) terminal_writestring("0");
    print_num(day);
    terminal_writestring(" ");
    if (hour < 10) terminal_writestring("0");
    print_num(hour);
    terminal_writestring(":");
    if (min < 10) terminal_writestring("0");
    print_num(min);
    terminal_writestring(":");
    if (sec < 10) terminal_writestring("0");
    print_num(sec);
    terminal_writestring(" UTC 20");
    if (year < 10) terminal_writestring("0");
    print_num(year);
    return 0;
}

static int prog_free(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint8_t hl = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(hl);
    terminal_writestring("\n            total       used       free\n");
    terminal_setcolor(body);
    terminal_writestring("Heap   ");
    print_num_padded((uint32_t)(mm_total() / 1024), 8);
    terminal_writestring(" kB ");
    print_num_padded((uint32_t)(mm_used() / 1024), 8);
    terminal_writestring(" kB ");
    print_num_padded((uint32_t)(mm_free() / 1024), 8);
    terminal_writestring(" kB");
    return 0;
}

static int prog_cpuid(int argc, char* argv[]) {
    (void)argc; (void)argv;
    char buf[256];
    int len = vfs_read("/proc/cpuinfo", buf, sizeof(buf));
    if (len > 0) {
        terminal_writestring("\n");
        terminal_writestring(buf);
    }
    return 0;
}

static int prog_ps(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint8_t hl = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(hl);
    terminal_writestring("\n PID  STATE    NAME\n");
    terminal_setcolor(body);

    struct process* pt = proc_table_ptr();
    for (int i = 0; i < PROC_MAX; i++) {
        if (pt[i].state == PROC_UNUSED) continue;
        terminal_writestring("   ");
        print_num(pt[i].pid);
        switch (pt[i].state) {
        case PROC_RUNNING: terminal_writestring("  run      "); break;
        case PROC_READY:   terminal_writestring("  ready    "); break;
        case PROC_ZOMBIE:  terminal_writestring("  zombie   "); break;
        default:           terminal_writestring("  ?        "); break;
        }
        terminal_writestring(pt[i].name);
        terminal_writestring("\n");
    }
    return 0;
}

static int prog_ls(int argc, char* argv[]) {
    const char* dir = "/";
    if (argc > 1) dir = argv[1];

    struct vfs_node* entries[32];
    int count = vfs_list(dir, entries, 32);

    if (count <= 0) {
        uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_setcolor(err);
        terminal_writestring("\nls: ");
        terminal_writestring(dir);
        terminal_writestring(": not found");
        return 1;
    }

    uint8_t dir_c = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t file_c = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t dev_c = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    uint8_t exec_c = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    terminal_writestring("\n");
    for (int i = 0; i < count; i++) {
        const char* name = vfs_basename(entries[i]->path);
        if (entries[i]->type == VFS_DIR) {
            terminal_setcolor(dir_c);
        } else if (entries[i]->type == VFS_DEV) {
            terminal_setcolor(dev_c);
        } else if (str_ncmp(entries[i]->path, "/bin/", 5) == 0) {
            terminal_setcolor(exec_c);
        } else {
            terminal_setcolor(file_c);
        }
        terminal_writestring(name);
        if (entries[i]->type == VFS_DIR) terminal_writestring("/");
        terminal_writestring("  ");
    }
    return 0;
}

static int prog_cat(int argc, char* argv[]) {
    if (argc < 2) {
        uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_setcolor(err);
        terminal_writestring("\ncat: missing file argument");
        return 1;
    }

    struct vfs_node* node = vfs_lookup(argv[1]);
    if (!node) {
        uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_setcolor(err);
        terminal_writestring("\ncat: ");
        terminal_writestring(argv[1]);
        terminal_writestring(": not found");
        return 1;
    }
    if (node->type == VFS_DIR) {
        uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_setcolor(err);
        terminal_writestring("\ncat: ");
        terminal_writestring(argv[1]);
        terminal_writestring(": is a directory");
        return 1;
    }

    char buf[512];
    int len = vfs_read(argv[1], buf, sizeof(buf));
    if (len >= 0) {
        terminal_writestring("\n");
        terminal_writestring(buf);
    }
    return 0;
}

static int prog_echo(int argc, char* argv[]) {
    terminal_writestring("\n");
    for (int i = 1; i < argc; i++) {
        if (i > 1) terminal_writestring(" ");
        terminal_writestring(argv[i]);
    }
    return 0;
}

static int prog_whoami(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\n" ERON_USER);
    return 0;
}

static int prog_hostname(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\n" ERON_HOSTNAME);
    return 0;
}

static int prog_pwd(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\n/");
    return 0;
}

static int prog_id(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\nuid=0(" ERON_USER ") gid=0(" ERON_USER ")");
    return 0;
}

static int prog_arch(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\ni386");
    return 0;
}

static int prog_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_clear();
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    return 0;
}

static int prog_reboot(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\nRestarting...\n");
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1) inb(0x60);
    } while (temp & 2);
    outb(0x64, 0xFE);
    asm volatile("hlt");
    return 0;
}

static int prog_halt(int argc, char* argv[]) {
    (void)argc; (void)argv;
    terminal_writestring("\nSystem halted.\n");
    asm volatile("cli");
    for (;;) asm volatile("hlt");
    return 0;
}

static int prog_malloc_test(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint8_t hl = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(hl);
    terminal_writestring("\nHeap test:\n");
    terminal_setcolor(body);

    terminal_writestring(" Before: used=");
    print_num((uint32_t)mm_used());
    terminal_writestring(" free=");
    print_num((uint32_t)mm_free());
    terminal_writestring("\n");

    void* p1 = kmalloc(64);
    void* p2 = kmalloc(128);
    void* p3 = kmalloc(256);

    terminal_writestring(" Allocated 64+128+256 bytes\n");
    terminal_writestring(" After:  used=");
    print_num((uint32_t)mm_used());
    terminal_writestring(" free=");
    print_num((uint32_t)mm_free());
    terminal_writestring("\n");

    kfree(p1);
    kfree(p2);
    kfree(p3);
    terminal_writestring(" Freed all blocks");
    return 0;
}

static int prog_display(int argc, char* argv[]) {
    (void)argc; (void)argv;
    struct display_info info;
    display_get_info(&info);

    uint8_t hl = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(hl);
    terminal_writestring("\nDisplay:\n");
    terminal_setcolor(body);
    terminal_writestring(" Mode      ");
    if (info.mode == DISPLAY_TEXT) terminal_writestring("text\n");
    else terminal_writestring("framebuffer\n");
    terminal_writestring(" Size      ");
    print_num(info.width);
    terminal_writestring("x");
    print_num(info.height);
    terminal_writestring("\n Depth     ");
    print_num(info.bpp);
    terminal_writestring(" bpp\n");
    terminal_writestring(" Buffer    0x");
    {
        uint32_t v = info.framebuffer;
        const char hex[] = "0123456789ABCDEF";
        char h[9];
        for (int i = 7; i >= 0; i--) { h[i] = hex[v & 0xF]; v >>= 4; }
        h[8] = '\0';
        terminal_writestring(h);
    }
    return 0;
}

/* ── exec: spawn user-mode process ────────────────────────── */

static int prog_exec(int argc, char* argv[]) {
    if (argc < 2) {
        uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_setcolor(err);
        terminal_writestring("\nexec: usage: exec <program>");
        return 1;
    }

    uint32_t entry = 0;
    const char* name = vfs_basename(argv[1]);

    if (str_cmp(argv[1], "/bin/hello") == 0 || str_cmp(argv[1], "hello") == 0) {
        entry = (uint32_t)user_hello;
        name = "hello";
    } else if (str_cmp(argv[1], "/bin/loop") == 0 || str_cmp(argv[1], "loop") == 0) {
        entry = (uint32_t)user_loop;
        name = "loop";
    } else if (str_cmp(argv[1], "/bin/crash") == 0 || str_cmp(argv[1], "crash") == 0) {
        entry = (uint32_t)user_crash;
        name = "crash";
    } else {
        char buf[4096];
        int len = vfs_read(argv[1], buf, sizeof(buf));
        if (len > 0) {
            entry = elf_load((const uint8_t*)buf, (uint32_t)len);
        }
        if (entry == 0) {
            uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            terminal_setcolor(err);
            terminal_writestring("\nexec: ");
            terminal_writestring(argv[1]);
            terminal_writestring(": not executable");
            return 1;
        }
    }

    terminal_writestring("\n");

    int pid = proc_create(name, entry);
    if (pid < 0) {
        uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_setcolor(err);
        terminal_writestring("exec: process table full");
        return 1;
    }

    struct process* pt = proc_table_ptr();
    struct process* child = NULL;
    for (int i = 0; i < PROC_MAX; i++) {
        if (pt[i].pid == (uint32_t)pid) {
            child = &pt[i];
            break;
        }
    }
    if (!child) return 1;

    while (child->state == PROC_READY || child->state == PROC_RUNNING) {
        asm volatile("sti");
        schedule();
    }

    child->state = PROC_UNUSED;
    return 0;
}

/* ── registration ─────────────────────────────────────────── */

void shell_register_programs(void) {
    program_register("help",    "show commands",          prog_help);
    program_register("about",   "system information",     prog_about);
    program_register("fastfetch","quick system summary",  prog_fastfetch);
    program_register("roadmap", "next-level OS plan",     prog_roadmap);
    program_register("service", "service manager",         prog_service);
    program_register("touch",   "create empty file",       prog_touch);
    program_register("write",   "write text to file",      prog_write);
    program_register("rm",      "remove file or node",     prog_rm);
    program_register("mv",      "rename file or node",     prog_mv);
    program_register("mkdir",   "create directory",         prog_mkdir);
    program_register("chmod",   "change permissions",       prog_chmod);
    program_register("stat",    "file metadata",            prog_stat);
    program_register("chmod",   "change file mode",        prog_chmod);
    program_register("clear",   "clear terminal",         prog_clear);
    program_register("uname",   "kernel identification",  prog_uname);
    program_register("uptime",  "system uptime",          prog_uptime);
    program_register("date",    "current date/time",      prog_date);
    program_register("free",    "memory usage",           prog_free);
    program_register("cpuid",   "CPU information",        prog_cpuid);
    program_register("whoami",  "current user",           prog_whoami);
    program_register("hostname","system hostname",        prog_hostname);
    program_register("pwd",     "current directory",      prog_pwd);
    program_register("ls",      "list files",             prog_ls);
    program_register("cat",     "read file",              prog_cat);
    program_register("echo",    "print text",             prog_echo);
    program_register("ps",      "process list",           prog_ps);
    program_register("id",      "user/group info",        prog_id);
    program_register("arch",    "architecture",           prog_arch);
    program_register("exec",    "run user program",       prog_exec);
    program_register("display", "display info",           prog_display);
    program_register("memtest", "test heap allocator",    prog_malloc_test);
    program_register("reboot",  "restart system",         prog_reboot);
    program_register("halt",    "power off",              prog_halt);
}

/* ── command parsing ──────────────────────────────────────── */

static int parse_args(char* input, char* argv[], int max) {
    int argc = 0;
    char* p = input;
    while (*p && argc < max) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

void execute_command(char* cmd) {
    uint8_t body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t err = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);

    terminal_setcolor(body);

    if (cmd[0] == '\0') {
        print_prompt();
        return;
    }

    char* argv[16];
    int argc = parse_args(cmd, argv, 16);
    if (argc == 0) {
        print_prompt();
        return;
    }

    if (str_cmp(argv[0], "clear") == 0) {
        prog_clear(argc, argv);
        print_prompt();
        return;
    }

    struct program* prog = program_find(argv[0]);
    if (prog) {
        prog->entry(argc, argv);
    } else if (str_cmp(argv[0], "exit") == 0) {
        terminal_setcolor(err);
        terminal_writestring("\neron: no parent process");
    } else if (str_cmp(argv[0], "shutdown") == 0 || str_cmp(argv[0], "poweroff") == 0) {
        prog_halt(argc, argv);
        return;
    } else {
        terminal_setcolor(err);
        terminal_writestring("\neron: ");
        terminal_writestring(argv[0]);
        terminal_writestring(": not found");
    }

    terminal_setcolor(body);
    terminal_writestring("\n");
    print_prompt();
}

void shell_input(char c) {
    if (c == '\n') {
        cmd_buffer[cmd_index] = '\0';
        execute_command(cmd_buffer);
        cmd_index = 0;
    } else if (c == '\b') {
        if (cmd_index > 0) {
            cmd_index--;
            terminal_putchar('\b');
        }
    } else {
        if (cmd_index < 127) {
            cmd_buffer[cmd_index++] = c;
        }
    }
}
