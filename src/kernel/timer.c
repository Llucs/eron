#include "../include/timer.h"
#include "../include/io.h"
#include "../include/config.h"
#include "../include/process.h"

static volatile uint32_t ticks = 0;
static uint32_t frequency = PIT_FREQ;

void timer_init(uint32_t freq) {
    frequency = freq;
    uint32_t divisor = 1193182 / freq;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void timer_tick(struct trapframe* tf) {
    ticks++;
    outb(0x20, 0x20);

    if ((tf->cs & 3) == 3 && (ticks % 10) == 0)
        schedule();
}

uint32_t timer_ticks(void)          { return ticks; }
uint32_t timer_seconds(void)        { return ticks / frequency; }
uint32_t timer_uptime_hours(void)   { return timer_seconds() / 3600; }
uint32_t timer_uptime_minutes(void) { return (timer_seconds() % 3600) / 60; }
uint32_t timer_uptime_seconds(void) { return timer_seconds() % 60; }
