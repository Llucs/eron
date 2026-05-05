#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

#define MOUSE_IRQ 12

#define MOUSE_CMD_SET_SCALE11   0xE6
#define MOUSE_CMD_SET_SCALE21   0xE7
#define MOUSE_CMD_ENABLE     0xF4
#define MOUSE_CMD_DISABLE  0xF5
#define MOUSE_CMD_READID   0xF2
#define MOUSE_CMD_SETRES   0xE8
#define MOUSE_CMD_SETREP  0xF3
#define MOUSE_CMD_DEFAULT  0xF6

#define MOUSE_ACK       0xFA
#define MOUSE_NACK      0xFE
#define MOUSE_ERROR    0xFC

#define MOUSE_BUF_SIZE 128

struct mouse_packet {
    int8_t movement_x;
    int8_t movement_y;
    int8_t movement_z;
    uint8_t buttons;
};

#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

void mouse_init(void);
void mouse_handler(void);
int mouse_get_packet(struct mouse_packet* pkt);
int mouse_available(void);
void mouse_get_position(int* x, int* y);
void mouse_set_position(int x, int y);
int mouse_get_buttons(void);
void mouse_show_cursor(int show);
void mouse_hide(void);

#endif