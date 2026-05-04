#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/vga.h"
#include "../include/config.h"
#include "../include/io.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern void terminal_setpos(size_t x, size_t y);
extern void terminal_clear_content(void);
extern void draw_box(size_t x, size_t y, size_t width, size_t height, uint8_t color);
extern void terminal_write_centered(const char* text, size_t row, uint8_t color);
extern void terminal_write_at(const char* text, size_t x, size_t y, uint8_t color);
extern void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
extern void draw_header(void);
extern void draw_status_bar(void);
extern void terminal_initialize(void);

static char cmd_buffer[128];
static int cmd_index = 0;

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0')
            return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
}

static void print_prompt(void) {
    uint8_t prompt_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    uint8_t body_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_setcolor(prompt_color);
    terminal_writestring("eron");
    terminal_setcolor(body_color);
    terminal_writestring("> ");
}

static void cmd_help(void) {
    uint8_t title_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t body_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(title_color);
    terminal_writestring("\n  Comandos disponiveis:");
    terminal_setcolor(body_color);
    terminal_writestring("\n  help     - Mostra esta ajuda");
    terminal_writestring("\n  clear    - Limpa a tela");
    terminal_writestring("\n  info     - Informacoes do sistema");
    terminal_writestring("\n  version  - Versao do sistema");
    terminal_writestring("\n  uptime   - Tempo desde o boot");
    terminal_writestring("\n  mem      - Informacoes de memoria");
    terminal_writestring("\n  cpuid    - Informacoes do processador");
    terminal_writestring("\n  echo     - Repete o texto digitado");
    terminal_writestring("\n  reboot   - Reinicia o sistema");
    terminal_writestring("\n  halt     - Desliga o sistema");
}

static void cmd_info(void) {
    uint8_t title_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t body_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(title_color);
    terminal_writestring("\n  Eron OS - Informacoes");
    terminal_setcolor(body_color);
    terminal_writestring("\n  Versao:  " ERON_VERSION);
    terminal_writestring("\n  Autor:   " ERON_AUTHOR);
    terminal_writestring("\n  Arch:    i386 (32-bit)");
    terminal_writestring("\n  Video:   VGA 80x25 texto");
    terminal_writestring("\n  Kernel:  Monolitico");
}

static void cmd_version(void) {
    terminal_writestring("\n  Eron OS " ERON_VERSION);
}

static volatile uint32_t tick_count = 0;

void timer_tick(void) {
    tick_count++;
}

static void cmd_uptime(void) {
    terminal_writestring("\n  Sistema ativo desde o boot.");
    terminal_writestring("\n  (Timer PIT nao configurado)");
}

static void cmd_mem(void) {
    uint8_t title_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t body_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(title_color);
    terminal_writestring("\n  Memoria do Sistema");
    terminal_setcolor(body_color);
    terminal_writestring("\n  Modo:  Flat (sem paginacao)");
    terminal_writestring("\n  Stack: 16 KB");
    terminal_writestring("\n  VGA:   0xB8000 (4000 bytes)");
    terminal_writestring("\n  Kernel: carregado em 0x100000");
}

static void cmd_cpuid(void) {
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0));

    *((uint32_t*)&vendor[0]) = ebx;
    *((uint32_t*)&vendor[4]) = edx;
    *((uint32_t*)&vendor[8]) = ecx;
    vendor[12] = '\0';

    uint8_t title_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t body_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    terminal_setcolor(title_color);
    terminal_writestring("\n  Processador");
    terminal_setcolor(body_color);
    terminal_writestring("\n  Vendor: ");
    terminal_writestring(vendor);

    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));

    uint32_t family = (eax >> 8) & 0xF;
    uint32_t model = (eax >> 4) & 0xF;
    uint32_t stepping = eax & 0xF;

    char num_buf[12];
    terminal_writestring("\n  Family: ");
    num_buf[0] = '0' + (family / 10);
    num_buf[1] = '0' + (family % 10);
    num_buf[2] = '\0';
    terminal_writestring(num_buf);

    terminal_writestring("  Model: ");
    num_buf[0] = '0' + (model / 10);
    num_buf[1] = '0' + (model % 10);
    num_buf[2] = '\0';
    terminal_writestring(num_buf);

    terminal_writestring("  Stepping: ");
    num_buf[0] = '0' + (stepping / 10);
    num_buf[1] = '0' + (stepping % 10);
    num_buf[2] = '\0';
    terminal_writestring(num_buf);
}

static void cmd_echo(const char* args) {
    terminal_writestring("\n  ");
    if (args && *args) {
        terminal_writestring(args);
    }
}

static void cmd_clear(void) {
    terminal_initialize();
    uint8_t color_accent = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    draw_header();
    draw_box(2, 2, 76, 20, color_accent);
    draw_status_bar();

    terminal_setpos(3, 3);
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static void cmd_reboot(void) {
    terminal_writestring("\n  Reiniciando...");
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1)
            inb(0x60);
    } while (temp & 2);
    outb(0x64, 0xFE);
    asm volatile("hlt");
}

static void cmd_halt(void) {
    terminal_writestring("\n  Desligando o sistema...");
    terminal_writestring("\n  Voce pode desligar o computador.");
    asm volatile("cli");
    for (;;) {
        asm volatile("hlt");
    }
}

void execute_command(char* cmd) {
    uint8_t body_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t err_color = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);

    terminal_setcolor(body_color);

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
        print_prompt();
        return;
    } else if (strcmp(cmd, "info") == 0) {
        cmd_info();
    } else if (strcmp(cmd, "version") == 0) {
        cmd_version();
    } else if (strcmp(cmd, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(cmd, "mem") == 0) {
        cmd_mem();
    } else if (strcmp(cmd, "cpuid") == 0) {
        cmd_cpuid();
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        cmd_echo(cmd + 5);
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo("");
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "halt") == 0 || strcmp(cmd, "shutdown") == 0) {
        cmd_halt();
    } else if (cmd[0] != '\0') {
        terminal_setcolor(err_color);
        terminal_writestring("\n  Comando nao encontrado: ");
        terminal_writestring(cmd);
        terminal_setcolor(body_color);
        terminal_writestring("\n  Digite 'help' para ajuda.");
    }

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
