#include "../include/teclado.h"
#include "../include/io.h"
#include <stdbool.h>

extern void terminal_putchar(char c);
extern void shell_input(char c);

unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',
    0, ' ', 0
};

void teclado_handler() {
    uint8_t scancode = inb(0x60);
    if (scancode & 0x80) {
        // Tecla solta
    } else {
        char c = kbdus[scancode];
        if (c != 0) {
            terminal_putchar(c);
            shell_input(c);
        }
    }
    outb(0x20, 0x20); // ACK para o PIC
}

void teclado_install() {
    // A instalação real depende de configurar o IDT gate para o IRQ1
    // Por enquanto, esta é a base do driver
}
