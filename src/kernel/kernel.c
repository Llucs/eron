#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/vga.h"
#include "../include/io.h"
#include "../include/idt.h"
#include "../include/teclado.h"
#include "../include/config.h"

void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_setpos(size_t x, size_t y);
void terminal_writestring(const char* data);
void terminal_write_at(const char* text, size_t x, size_t y, uint8_t color);
void draw_box(size_t x, size_t y, size_t width, size_t height, uint8_t color);
void terminal_write_centered(const char* text, size_t row, uint8_t color);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);

extern void keyboard_handler_asm(void);

void draw_header(void) {
    uint8_t color_header = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    for (size_t x = 0; x < 80; x++) {
        terminal_putentryat(' ', color_header, x, 0);
        terminal_putentryat(' ', color_header, x, 1);
    }
    terminal_write_centered("ERON OS - " ERON_VERSION, 0, color_header);
    terminal_write_centered("Sistema Operacional Educacional", 1, color_header);
}

void draw_status_bar(void) {
    uint8_t color_status = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_GREEN);
    for (size_t x = 0; x < 80; x++) {
        terminal_putentryat(' ', color_status, x, 24);
    }
    terminal_write_at("[ STATUS: ATIVO ]", 2, 24, color_status);
    terminal_write_at("Mem: 128MB", 60, 24, color_status);
}

void draw_ui(void) {
    uint8_t color_accent = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    draw_header();
    draw_box(2, 2, 76, 20, color_accent);
    draw_status_bar();
}

void kernel_main(void) {
    terminal_initialize();

    idt_install();
    idt_set_gate(33, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    asm volatile("sti");

    uint8_t color_body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t color_accent = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t color_highlight = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    draw_ui();

    terminal_write_centered("Eron OS", 5, color_highlight);
    terminal_write_centered("Desenvolvido por " ERON_AUTHOR, 7, color_body);

    terminal_write_at("> Kernel carregado com sucesso", 4, 10, color_accent);
    terminal_write_at("> Modo VGA 80x25 ativo", 4, 12, color_accent);
    terminal_write_at("> IDT e PIC configurados", 4, 13, color_accent);
    terminal_write_at("> Teclado PS/2 ativo", 4, 14, color_accent);
    terminal_write_at("> Sistema pronto", 4, 16, color_highlight);

    terminal_write_at("Digite 'help' para ver os comandos disponiveis.", 4, 18, color_body);

    terminal_setpos(3, 20);
    terminal_setcolor(color_body);
    terminal_writestring("eron> ");

    while (true) {
        asm volatile("hlt");
    }
}
