#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/vga.h"
#include "../include/config.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_initialize(void);
extern void draw_box(size_t x, size_t y, size_t width, size_t height, uint8_t color);
extern void terminal_write_centered(const char* text, size_t row, uint8_t color);

char cmd_buffer[128];
int cmd_index = 0;

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void execute_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        terminal_writestring("\nComandos: help, clear, info");
    } else if (strcmp(cmd, "clear") == 0) {
        terminal_initialize();
        uint8_t color_header = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        uint8_t color_accent = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        draw_box(2, 2, 76, 20, color_accent);
        terminal_write_centered("ERON OS - " ERON_VERSION, 0, color_header);
    } else if (strcmp(cmd, "info") == 0) {
        terminal_writestring("\nEron OS " ERON_VERSION "\nAutor: " ERON_AUTHOR);
    } else if (cmd[0] != '\0') {
        terminal_writestring("\nComando desconhecido.");
    }
    terminal_writestring("\neron> ");
}

void shell_input(char c) {
    if (c == '\n') {
        cmd_buffer[cmd_index] = '\0';
        execute_command(cmd_buffer);
        cmd_index = 0;
    } else if (c == '\b') {
        if (cmd_index > 0) cmd_index--;
    } else {
        if (cmd_index < 127) {
            cmd_buffer[cmd_index++] = c;
        }
    }
}
