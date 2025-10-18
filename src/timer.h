/* Minimal Timer Header */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(void);
uint64_t timer_get_ticks(void);
void timer_delay_us(uint32_t microseconds);
void timer_delay_ms(uint32_t milliseconds);

#endif
