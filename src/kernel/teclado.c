#include "../include/teclado.h"
#include "../include/io.h"
#include <stdbool.h>
#include <stdint.h>

extern void terminal_putchar(char c);
extern void shell_input(char c);

static bool shift_pressed = false;
static bool caps_lock = false;

static unsigned char kbdus_lower[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',
    0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    0, 0, 0, '-',
    0, 0, 0, '+',
    0, 0, 0, 0, 0,
    0, 0, 0,
    0, 0
};

static unsigned char kbdus_upper[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0, '*',
    0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    0, 0, 0, '-',
    0, 0, 0, '+',
    0, 0, 0, 0, 0,
    0, 0, 0,
    0, 0
};

void teclado_handler(void) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        outb(0x20, 0x20);
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        outb(0x20, 0x20);
        return;
    }

    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        outb(0x20, 0x20);
        return;
    }

    if (!(scancode & 0x80)) {
        char c;
        bool use_upper = shift_pressed;

        if (scancode >= 0x10 && scancode <= 0x32) {
            if (caps_lock) use_upper = !use_upper;
        }

        if (use_upper)
            c = kbdus_upper[scancode];
        else
            c = kbdus_lower[scancode];

        if (c != 0) {
            if (c == '\n' || c == '\b') {
                shell_input(c);
            } else {
                terminal_putchar(c);
                shell_input(c);
            }
        }
    }
    outb(0x20, 0x20);
}

void teclado_install(void) {
}
