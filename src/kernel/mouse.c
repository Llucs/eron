#include "../include/mouse.h"
#include "../include/io.h"
#include "../include/idt.h"
#include <stdbool.h>
#include <stdint.h>

#define MOUSE_BUF_SIZE 128

static volatile uint8_t mouse_buf[MOUSE_BUF_SIZE];
static volatile int mouse_head = 0;
static volatile int mouse_tail = 0;

static int mouse_x = 400;
static int mouse_y = 300;
static int mouse_buttons = 0;
static int cursor_visible = 1;
static int mouse_initialized = 0;

static void mouse_wait_write(uint8_t type) {
    for (int i = 0; i < 100000; i++) {
        if (type == 0) {
            if (!(inb(0x64) & 0x20)) return;
        } else {
            if (!(inb(0x64) & 0x02)) return;
        }
    }
}

static void mouse_cmd_write(uint8_t val) {
    mouse_wait_write(1);
    outb(0x60, val);
}

static uint8_t mouse_data_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(0x64) & 0x01) {
            return inb(0x60);
        }
    }
    return 0xFF;
}

void mouse_init(void) {
    if (mouse_initialized) return;
    
    uint8_t status;
    
    mouse_cmd_write(0xA8);
    asm volatile("cli");
    mouse_cmd_write(0x20);
    status = mouse_data_read();
    status |= 0x02;
    mouse_cmd_write(0x60);
    mouse_cmd_write(status);
    mouse_cmd_write(0xD4);
    mouse_cmd_write(0xF4);
    mouse_data_read();
    asm volatile("sti");
    
    mouse_initialized = 1;
    mouse_x = 400;
    mouse_y = 300;
}

void mouse_handler(void) {
    static uint8_t mouse_packet[3];
    static int packet_idx = 0;
    
    uint8_t data = inb(0x60);
    
    if (data == 0xFA) {
        packet_idx = 0;
    } else if (data == 0xFE) {
        packet_idx = 0;
    } else if (data == 0xFF) {
        packet_idx = 0;
    }
    
    if (!(data & 0x08)) {
        return;
    }
    
    mouse_packet[packet_idx++] = data;
    
    if (packet_idx >= 3) {
        packet_idx = 0;
        
        int buttons = mouse_packet[0] & 0x07;
        int dx = (mouse_packet[0] & 0x10) ? mouse_packet[1] - 256 : mouse_packet[1];
        int dy = (mouse_packet[0] & 0x20) ? mouse_packet[2] - 256 : mouse_packet[2];
        
        mouse_x += dx;
        mouse_y -= dy;
        
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > 799) mouse_x = 799;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > 599) mouse_y = 599;
        
        mouse_buttons = buttons;
        
        int next = (mouse_head + 1) % MOUSE_BUF_SIZE;
        if (next != mouse_tail) {
            struct mouse_packet pkt;
            pkt.movement_x = (int8_t)dx;
            pkt.movement_y = (int8_t)dy;
            pkt.movement_z = 0;
            pkt.buttons = buttons;
            *(struct mouse_packet*)&mouse_buf[mouse_head] = pkt;
            mouse_head = next;
        }
    }
    
    outb(0x20, 0x20);
}

int mouse_get_packet(struct mouse_packet* pkt) {
    if (mouse_head == mouse_tail) return 0;
    
    if (pkt) {
        *pkt = *(struct mouse_packet*)&mouse_buf[mouse_tail];
        mouse_tail = (mouse_tail + 1) % MOUSE_BUF_SIZE;
    }
    return 1;
}

int mouse_available(void) {
    return (mouse_head != mouse_tail) ? 1 : 0;
}

void mouse_get_position(int* x, int* y) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
}

void mouse_set_position(int x, int y) {
    mouse_x = x;
    mouse_y = y;
}

int mouse_get_buttons(void) {
    return mouse_buttons;
}

void mouse_show_cursor(int show) {
    cursor_visible = show;
}

void mouse_hide(void) {
    cursor_visible = 0;
}