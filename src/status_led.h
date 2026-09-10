#ifndef RS422_STATUS_LED_H
#define RS422_STATUS_LED_H

#include <stdint.h>

void status_led_init(void);
// Core0 only. Components 0-255; keep them low, the LED is very bright.
void status_led_set(uint8_t r, uint8_t g, uint8_t b);

#endif
