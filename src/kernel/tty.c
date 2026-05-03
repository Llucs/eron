#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_clear_row(size_t y) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = y * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
}

void terminal_scroll() {
    for (size_t y = 3; y < 21; y++) {
        for (size_t x = 3; x < 77; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 3; x < 77; x++) {
        terminal_buffer[21 * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_initialize(void) {
	terminal_row = 10;
	terminal_column = 5;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 5;
        if (++terminal_row >= 21) {
            terminal_scroll();
            terminal_row = 21;
        }
        return;
    }
    
    if (c == '\b') {
        if (terminal_column > 5) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        }
        return;
    }

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column >= 77) {
		terminal_column = 5;
		if (++terminal_row >= 21) {
			terminal_scroll();
            terminal_row = 21;
        }
	}
}

void terminal_writestring(const char* data) {
	for (size_t i = 0; data[i] != '\0'; i++)
		terminal_putchar(data[i]);
}

void draw_box(size_t x, size_t y, size_t width, size_t height, uint8_t color) {
    for (size_t i = x; i < x + width; i++) {
        terminal_putentryat('-', color, i, y);
        terminal_putentryat('-', color, i, y + height - 1);
    }
    for (size_t j = y; j < y + height; j++) {
        terminal_putentryat('|', color, x, j);
        terminal_putentryat('|', color, x + width - 1, j);
    }
    terminal_putentryat('+', color, x, y);
    terminal_putentryat('+', color, x + width - 1, y);
    terminal_putentryat('+', color, x, y + height - 1);
    terminal_putentryat('+', color, x + width - 1, y + height - 1);
}

void terminal_write_centered(const char* text, size_t row, uint8_t color) {
    size_t len = 0;
    while (text[len] != '\0') len++;
    size_t start_x = (VGA_WIDTH - len) / 2;
    for (size_t i = 0; i < len; i++) {
        terminal_putentryat(text[i], color, start_x + i, row);
    }
}
