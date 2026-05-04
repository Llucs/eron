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

static const size_t CONTENT_TOP = 3;
static const size_t CONTENT_BOTTOM = 20;
static const size_t CONTENT_LEFT = 3;
static const size_t CONTENT_RIGHT = 76;

void terminal_scroll(void) {
    for (size_t y = CONTENT_TOP; y < CONTENT_BOTTOM; y++) {
        for (size_t x = CONTENT_LEFT; x <= CONTENT_RIGHT; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = CONTENT_LEFT; x <= CONTENT_RIGHT; x++) {
        terminal_buffer[CONTENT_BOTTOM * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_initialize(void) {
    terminal_row = CONTENT_TOP;
    terminal_column = CONTENT_LEFT;
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

void terminal_setpos(size_t x, size_t y) {
    terminal_column = x;
    terminal_row = y;
}

size_t terminal_getrow(void) {
    return terminal_row;
}

size_t terminal_getcolumn(void) {
    return terminal_column;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = CONTENT_LEFT;
        if (++terminal_row > CONTENT_BOTTOM) {
            terminal_scroll();
            terminal_row = CONTENT_BOTTOM;
        }
        return;
    }

    if (c == '\b') {
        if (terminal_column > CONTENT_LEFT) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        }
        return;
    }

    if (c == '\t') {
        size_t spaces = 4 - (terminal_column % 4);
        for (size_t i = 0; i < spaces && terminal_column <= CONTENT_RIGHT; i++) {
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
            terminal_column++;
        }
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column > CONTENT_RIGHT) {
        terminal_column = CONTENT_LEFT;
        if (++terminal_row > CONTENT_BOTTOM) {
            terminal_scroll();
            terminal_row = CONTENT_BOTTOM;
        }
    }
}

void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}

void terminal_write_at(const char* text, size_t x, size_t y, uint8_t color) {
    for (size_t i = 0; text[i] != '\0'; i++) {
        terminal_putentryat(text[i], color, x + i, y);
    }
}

void draw_box(size_t x, size_t y, size_t width, size_t height, uint8_t color) {
    terminal_putentryat(0xC9, color, x, y);
    terminal_putentryat(0xBB, color, x + width - 1, y);
    terminal_putentryat(0xC8, color, x, y + height - 1);
    terminal_putentryat(0xBC, color, x + width - 1, y + height - 1);

    for (size_t i = x + 1; i < x + width - 1; i++) {
        terminal_putentryat(0xCD, color, i, y);
        terminal_putentryat(0xCD, color, i, y + height - 1);
    }
    for (size_t j = y + 1; j < y + height; j++) {
        terminal_putentryat(0xBA, color, x, j);
        terminal_putentryat(0xBA, color, x + width - 1, j);
    }
}

void terminal_write_centered(const char* text, size_t row, uint8_t color) {
    size_t len = 0;
    while (text[len] != '\0') len++;
    size_t start_x = (VGA_WIDTH - len) / 2;
    for (size_t i = 0; i < len; i++) {
        terminal_putentryat(text[i], color, start_x + i, row);
    }
}

void terminal_clear_content(void) {
    for (size_t y = CONTENT_TOP; y <= CONTENT_BOTTOM; y++) {
        for (size_t x = CONTENT_LEFT; x <= CONTENT_RIGHT; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = CONTENT_TOP;
    terminal_column = CONTENT_LEFT;
}
