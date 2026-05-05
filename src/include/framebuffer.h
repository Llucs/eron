#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#define FB_MAX_WIDTH  1920
#define FB_MAX_HEIGHT 1080
#define FB_MAX_BPP   32

enum fb_format {
    FB_FORMAT_RGB565 = 0,
    FB_FORMAT_RGB888 = 1,
    FB_FORMAT_BGRX8888 = 2
};

struct framebuffer_info {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t format;
    void* buffer;
    uint32_t size;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t red_shift;
    uint32_t green_shift;
    uint32_t blue_shift;
};

struct vesa_mode_info {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t window_a_start;
    uint16_t window_b_start;
    uint32_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t char_width;
    uint8_t char_height;
    uint8_t planes;
    uint8_t bits_per_pixel;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved1;
    uint16_t red_mask;
    uint8_t red_position;
    uint8_t red_size;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t green_size;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t blue_size;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t reserved_size;
    uint8_t colorplanes;
    uint8_t colorplane_size;
    uint8_t page_granularity;
    uint16_t max_window;
    uint16_t window_alignment;
    uint32_t buffer_start;
    uint32_t buffer_size;
    uint32_t window_b_start_alignment;
    uint8_t reserved2[206];
} __attribute__((packed));

void fb_init(void);
int fb_set_mode(uint32_t width, uint32_t height, uint32_t bpp);
void fb_get_info(struct framebuffer_info* info);
void fb_clear(uint32_t color);
void fb_putpixel(int x, int y, uint32_t color);
uint32_t fb_getpixel(int x, int y);
void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void fb_draw_circle(int cx, int cy, int r, uint32_t color);
void fb_draw_filled_circle(int cx, int cy, int r, uint32_t color);
void fb_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void fb_draw_string(int x, int y, const char* s, uint32_t fg, uint32_t bg);
void fb_copy(const void* src, int x, int y, int w, int h);
void fb_swap(void);

uint32_t fb_make_rgb(uint8_t r, uint8_t g, uint8_t b);
uint32_t fb_make_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif