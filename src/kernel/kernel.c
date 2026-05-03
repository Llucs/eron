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
void terminal_writestring(const char* data);
void draw_box(size_t x, size_t y, size_t width, size_t height, uint8_t color);
void terminal_write_centered(const char* text, size_t row, uint8_t color);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);

extern void keyboard_handler_asm();

void kernel_main(void) {
    terminal_initialize();

    /* Inicializacao do sistema de interrupcoes */
    idt_install();
    idt_set_gate(33, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);

    /* Reprogramacao do PIC */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    asm volatile("sti");

    uint8_t color_header = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    uint8_t color_body = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t color_status = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    uint8_t color_accent = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);

    /* Interface Visual */
    for (size_t x = 0; x < 80; x++) {
        terminal_putentryat(' ', color_header, x, 0);
    }
    
    char header_text[32];
    // Simples concatenacao manual para evitar dependencias
    terminal_write_centered("ERON OS - " ERON_VERSION, 0, color_header);

    draw_box(2, 2, 76, 20, color_accent);
    terminal_write_centered("Eron OS", 5, color_accent);
    terminal_write_centered("Desenvolvido por " ERON_AUTHOR, 7, color_body);

    terminal_setcolor(color_body);
    terminal_putentryat('>', color_accent, 5, 10);
    terminal_writestring(" Kernel carregado.");
    
    terminal_putentryat('>', color_accent, 5, 12);
    terminal_writestring(" Modo VGA 80x25.");

    terminal_putentryat('>', color_accent, 5, 14);
    terminal_writestring(" Sistema pronto.");

    for (size_t x = 0; x < 80; x++) {
        terminal_putentryat(' ', color_status, x, 24);
    }
    terminal_putentryat('[', color_status, 2, 24);
    terminal_writestring(" STATUS: ATIVO ");
    terminal_putentryat(']', color_status, 18, 24);
    
    terminal_writestring("\neron> ");

    while (true) {
        asm volatile("hlt");
    }
}
