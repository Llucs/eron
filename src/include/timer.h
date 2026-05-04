#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(uint32_t freq);
void timer_handler(void);
uint32_t timer_ticks(void);
uint32_t timer_seconds(void);
uint32_t timer_uptime_hours(void);
uint32_t timer_uptime_minutes(void);
uint32_t timer_uptime_seconds(void);

#endif
