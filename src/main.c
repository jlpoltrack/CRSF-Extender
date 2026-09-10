#include "config.h"
#include "stats.h"
#include "local_port.h"
#include "link_port.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include <stdio.h>

void bridge_core1(void);

static void trace_pins_init(void) {
    gpio_init(TRACE_DIR_PIN);  gpio_set_dir(TRACE_DIR_PIN, GPIO_OUT);
    gpio_init(TRACE_LINK_PIN); gpio_set_dir(TRACE_LINK_PIN, GPIO_OUT);
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN); gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

int main(void) {
    set_sys_clock_khz(SYS_CLK_KHZ, true);

    // USB must come up on core0 so TinyUSB's IRQ never lands on the shovel core.
    stdio_init_all();

    trace_pins_init();
    link_port_init();
    local_port_init();

    multicore_launch_core1(bridge_core1);

    printf("\n=== RS-422 bridge [%s] ===\n", SIDE_NAME);
    printf("local %u baud inverted 1-wire on GP%d | link %u baud on GP%d/GP%d | sys %lu kHz\n",
           LOCAL_BAUD, LOCAL_PIN, LINK_BAUD, LINK_TX_PIN, LINK_RX_PIN,
           (unsigned long)(clock_get_hz(clk_sys) / 1000));

    absolute_time_t next = make_timeout_time_us(REPORT_INTERVAL_US);
    uint32_t uptime_s = 0;

    for (;;) {
        sleep_until(next);
        next = delayed_by_us(next, REPORT_INTERVAL_US);
        uptime_s++;

        g_snap_req = true;
        while (g_snap_req) tight_loop_contents();
        __dmb();

        stats_report(&g_snap, uptime_s);

#ifdef PICO_DEFAULT_LED_PIN
        gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
#endif
    }
}
