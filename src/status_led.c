#include "status_led.h"
#include "config.h"

#include "hardware/pio.h"
#include "ws2812.pio.h"

// PIO1 so the LED never touches PIO0's instruction memory or the bridge SMs.
#define PIO_LED  pio1
#define SM_LED   0

void status_led_init(void) {
    uint offset = pio_add_program(PIO_LED, &ws2812_program);
    ws2812_program_init(PIO_LED, SM_LED, offset, STATUS_LED_PIN, 800000.0f);
    status_led_set(0, 0, 0);
}

void status_led_set(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    // Non-blocking: dropping one heartbeat update is harmless.
    if (!pio_sm_is_tx_fifo_full(PIO_LED, SM_LED))
        pio_sm_put(PIO_LED, SM_LED, grb << 8u);
}
