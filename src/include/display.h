#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stddef.h>

/*
 * Display abstraction layer.
 *
 * Current: VGA text mode (80x25, 16 colors)
 * Future:  VESA Linear Framebuffer (LFB) for graphical mode
 *
 * To add GUI support:
 *  1. Request LFB via multiboot header flags
 *  2. Implement display_init_fb() using the framebuffer address
 *  3. Add pixel drawing primitives (putpixel, fillrect, bitblt)
 *  4. Implement a window manager on top of the framebuffer
 *  5. Switch display_mode to DISPLAY_FRAMEBUFFER
 */

enum display_mode {
    DISPLAY_TEXT,
    DISPLAY_FRAMEBUFFER
};

struct display_info {
    enum display_mode mode;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    uint32_t framebuffer;
};

void display_get_info(struct display_info* info);
enum display_mode display_current_mode(void);

#endif
