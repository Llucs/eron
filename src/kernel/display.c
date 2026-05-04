#include "../include/display.h"

void display_get_info(struct display_info* info) {
    info->mode = DISPLAY_TEXT;
    info->width = 80;
    info->height = 25;
    info->bpp = 4;
    info->pitch = 160;
    info->framebuffer = 0xB8000;
}

enum display_mode display_current_mode(void) {
    return DISPLAY_TEXT;
}
