#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/vga.h"
#include "../include/io.h"
#include "../include/idt.h"
#include "../include/gdt.h"
#include "../include/tss.h"
#include "../include/teclado.h"
#include "../include/config.h"
#include "../include/mm.h"
#include "../include/timer.h"
#include "../include/vfs.h"
#include "../include/task.h"
#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/virtual_mm.h"

void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_writestring(const char* data);
void terminal_putchar(char c);

extern void keyboard_handler_asm(void);
extern void timer_handler_asm(void);
extern void isr0(void);
extern void isr6(void);
extern void isr13(void);
extern void isr14(void);
extern uint32_t _kernel_end;
extern uint32_t stack_top;

void shell_register_programs(void);
void shell_input(char c);
void print_prompt(void);

static int proc_cpuinfo_read(char* buf, size_t size);
static int proc_meminfo_read(char* buf, size_t size);
static int proc_uptime_read(char* buf, size_t size);
static void enable_paging(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static void boot_log(const char* msg) {
    uint8_t arrow = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t txt = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_setcolor(arrow);
    terminal_writestring(" >> ");
    terminal_setcolor(txt);
    terminal_writestring(msg);
    terminal_writestring("\n");
}

static void vfs_populate(void) {
    vfs_mkdir("/");
    vfs_mkdir("/bin");
    vfs_mkdir("/dev");
    vfs_mkdir("/etc");
    vfs_mkdir("/home");
    vfs_mkdir("/proc");
    vfs_mkdir("/tmp");
    vfs_mkdir("/var");

    vfs_mkfile("/etc/hostname", ERON_HOSTNAME);
    vfs_mkfile("/etc/version", "Eron OS " ERON_VERSION " (" ERON_CODENAME ")");
    vfs_mkfile("/etc/motd", "Welcome to Eron OS.\nType 'help' for commands.");
    vfs_mkfile("/etc/os-release",
        "NAME=EronOS\nVERSION=" ERON_VERSION "\nCODENAME=" ERON_CODENAME
        "\nARCH=i386\nAUTHOR=" ERON_AUTHOR);

    vfs_mkproc("/proc/cpuinfo", proc_cpuinfo_read);
    vfs_mkproc("/proc/meminfo", proc_meminfo_read);
    vfs_mkproc("/proc/uptime", proc_uptime_read);
    vfs_mkfile("/proc/version", "Eron " ERON_VERSION " (i386)");

    vfs_mkdev("/dev/tty0");
    vfs_mkdev("/dev/kbd0");
    vfs_mkdev("/dev/null");
    vfs_mkdev("/dev/vga0");

    vfs_mkfile("/bin/hello", "[elf32-i386] user-mode hello");
    vfs_mkfile("/bin/loop",  "[elf32-i386] user-mode loop test");
    vfs_mkfile("/bin/crash", "[elf32-i386] user-mode fault test");
}

void kernel_main(void) {
    terminal_initialize();

    uint8_t white = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t grey = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t dim = vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(white);
    terminal_writestring("\n Eron OS ");
    terminal_setcolor(grey);
    terminal_writestring(ERON_VERSION);
    terminal_setcolor(dim);
    terminal_writestring(" (" ERON_CODENAME ")\n");
    terminal_setcolor(grey);
    terminal_writestring(" (c) " ERON_AUTHOR "\n\n");

    gdt_init();
    boot_log("GDT: kernel/user segments loaded");

    idt_install();
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)timer_handler_asm,    0x08, 0x8E);
    idt_set_gate(33, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);
    boot_log("IDT: 256 interrupt gates");

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
    boot_log("PIC: remapped to INT 32-47");

    tss_init(0x10, (uint32_t)&stack_top);
    boot_log("TSS: ring transition ready");

    timer_init(PIT_FREQ);
    boot_log("PIT: timer at 100 Hz");

    asm volatile("sti");
    boot_log("PS/2: keyboard ready");
    boot_log("VGA: 80x25 text mode");

    mm_init((uint32_t)&_kernel_end, HEAP_SIZE);
    boot_log("Memory: 1 MB heap initialized");

    vmm_init((uint32_t)&_kernel_end + 0x100000);
    enable_paging();
    boot_log("Paging: virtual memory enabled");

    vfs_init();
    vfs_populate();
    boot_log("VFS: filesystem mounted");

    proc_init();
    boot_log("Process: scheduler initialized");

    programs_init();
    shell_register_programs();

    char prog_msg[48];
    int pc = program_count();
    int idx = 0;

    prog_msg[idx++] = 'P'; prog_msg[idx++] = 'r'; prog_msg[idx++] = 'o';
    prog_msg[idx++] = 'g'; prog_msg[idx++] = 'r'; prog_msg[idx++] = 'a';
    prog_msg[idx++] = 'm'; prog_msg[idx++] = 's'; prog_msg[idx++] = ':';
    prog_msg[idx++] = ' ';

    if (pc == 0) {
        prog_msg[idx++] = '0';
    } else {
        char digits[10];
        int d = 0;
        while (pc > 0 && d < 10) {
            digits[d++] = '0' + (pc % 10);
            pc /= 10;
        }
        for (int i = d - 1; i >= 0; i--) prog_msg[idx++] = digits[i];
    }

    prog_msg[idx++] = ' ';
    prog_msg[idx++] = 'r'; prog_msg[idx++] = 'e'; prog_msg[idx++] = 'g';
    prog_msg[idx++] = 'i'; prog_msg[idx++] = 's'; prog_msg[idx++] = 't';
    prog_msg[idx++] = 'e'; prog_msg[idx++] = 'r'; prog_msg[idx++] = 'e';
    prog_msg[idx++] = 'd';
    prog_msg[idx] = '\0';
    boot_log(prog_msg);

    syscall_init();
    boot_log("Syscall: INT 0x80 handler active");

    terminal_writestring("\n");
    terminal_setcolor(white);
    terminal_writestring(" System ready.\n\n");
    terminal_setcolor(grey);

    print_prompt();

    while (true) {
        char c;
        while ((c = keyboard_getchar()) != 0)
            shell_input(c);

        struct process* pt = proc_table_ptr();
        for (int i = 1; i < PROC_MAX; i++) {
            if (pt[i].state == PROC_ZOMBIE)
                pt[i].state = PROC_UNUSED;
        }

        asm volatile("sti; hlt");
    }
}

/* /proc dynamic readers */

static void str_append(char* buf, size_t* pos, size_t max, const char* s) {
    while (*s && *pos < max - 1) buf[(*pos)++] = *s++;
    buf[*pos] = '\0';
}

static void num_append(char* buf, size_t* pos, size_t max, uint32_t val) {
    char tmp[12];
    int i = 0;
    if (val == 0) { tmp[i++] = '0'; }
    else { while (val > 0) { tmp[i++] = '0' + val % 10; val /= 10; } }
    for (int j = i - 1; j >= 0 && *pos < max - 1; j--)
        buf[(*pos)++] = tmp[j];
    buf[*pos] = '\0';
}

static int proc_cpuinfo_read(char* buf, size_t size) {
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    *((uint32_t*)&vendor[0]) = ebx;
    *((uint32_t*)&vendor[4]) = edx;
    *((uint32_t*)&vendor[8]) = ecx;
    vendor[12] = '\0';

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));

    size_t p = 0;
    str_append(buf, &p, size, "processor : 0\nvendor    : ");
    str_append(buf, &p, size, vendor);
    str_append(buf, &p, size, "\nfamily    : ");
    num_append(buf, &p, size, (eax >> 8) & 0xF);
    str_append(buf, &p, size, "\nmodel     : ");
    num_append(buf, &p, size, (eax >> 4) & 0xF);
    str_append(buf, &p, size, "\nstepping  : ");
    num_append(buf, &p, size, eax & 0xF);
    str_append(buf, &p, size, "\nflags     : ");
    if (edx & (1 << 0)) str_append(buf, &p, size, "fpu ");
    if (edx & (1 << 4)) str_append(buf, &p, size, "tsc ");
    if (edx & (1 << 23)) str_append(buf, &p, size, "mmx ");
    if (edx & (1 << 25)) str_append(buf, &p, size, "sse ");
    if (edx & (1 << 26)) str_append(buf, &p, size, "sse2 ");
    return (int)p;
}

static int proc_meminfo_read(char* buf, size_t size) {
    size_t p = 0;
    str_append(buf, &p, size, "HeapTotal : ");
    num_append(buf, &p, size, (uint32_t)(mm_total() / 1024));
    str_append(buf, &p, size, " kB\nHeapUsed  : ");
    num_append(buf, &p, size, (uint32_t)(mm_used() / 1024));
    str_append(buf, &p, size, " kB\nHeapFree  : ");
    num_append(buf, &p, size, (uint32_t)(mm_free() / 1024));
    str_append(buf, &p, size, " kB\nStack     : 16 kB");
    return (int)p;
}

static int proc_uptime_read(char* buf, size_t size) {
    size_t p = 0;
    num_append(buf, &p, size, timer_seconds());
    str_append(buf, &p, size, " seconds");
    return (int)p;
}
