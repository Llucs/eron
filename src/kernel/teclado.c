#include "../include/teclado.h"
#include "../include/io.h"
#include <stdbool.h>
#include <stdint.h>

extern void terminal_putchar(char c);

static bool shift_pressed = false;
static bool caps_lock = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

#define KB_BUF_SZ 128
static volatile char kb_buf[KB_BUF_SZ];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;
static volatile int kb_state = 0;

#define KB_STATE_SHIFT 0x01
#define KB_STATE_CTRL 0x02
#define KB_STATE_ALT 0x04
#define KB_STATE_CAPS 0x08

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

void keyboard_irq(void) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        kb_state |= KB_STATE_SHIFT;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        kb_state &= ~KB_STATE_SHIFT;
        return;
    }
    if (scancode == 0x1D) {
        ctrl_pressed = true;
        kb_state |= KB_STATE_CTRL;
        return;
    }
    if (scancode == 0x9D) {
        ctrl_pressed = false;
        kb_state &= ~KB_STATE_CTRL;
        return;
    }
    if (scancode == 0x38) {
        alt_pressed = true;
        kb_state |= KB_STATE_ALT;
        return;
    }
    if (scancode == 0xB8) {
        alt_pressed = false;
        kb_state &= ~KB_STATE_ALT;
        return;
    }
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        if (caps_lock) kb_state |= KB_STATE_CAPS;
        else kb_state &= ~KB_STATE_CAPS;
        return;
    }
    if (scancode == 0xE0) {
        return;
    }

    if (!(scancode & 0x80)) {
        char c;
        bool use_upper = shift_pressed;
        if (scancode >= 0x10 && scancode <= 0x32)
            if (caps_lock) use_upper = !use_upper;

        if (use_upper) c = kbdus_upper[scancode];
        else           c = kbdus_lower[scancode];

        if (c != 0) {
            if (c != '\n' && c != '\b')
                terminal_putchar(c);

            int next = (kb_head + 1) % KB_BUF_SZ;
            if (next != kb_tail) {
                kb_buf[kb_head] = c;
                kb_head = next;
            }
        }
    }
}

char keyboard_getchar(void) {
    if (kb_head == kb_tail) return 0;
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SZ;
    return c;
}

int keyboard_available(void) {
    return (kb_head != kb_tail) ? 1 : 0;
}

int keyboard_get_state(void) {
    return kb_state;
}

void keyboard_clear_buffer(void) {
    kb_head = 0;
    kb_tail = 0;
}