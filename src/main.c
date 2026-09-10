#include "config.h"
#include "stats.h"
#include "local_port.h"
#include "link_port.h"
#include "status_led.h"

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

    const uint gnd_pins[] = { GND_PIN_A, GND_PIN_B };
    for (unsigned i = 0; i < 2; i++) {
        gpio_init(gnd_pins[i]);
        gpio_put(gnd_pins[i], 0);
        gpio_set_dir(gnd_pins[i], GPIO_OUT);
        gpio_set_drive_strength(gnd_pins[i], GPIO_DRIVE_STRENGTH_12MA);
    }
}

int main(void) {
    set_sys_clock_khz(SYS_CLK_KHZ, true);

    // USB must come up on core0 so TinyUSB's IRQ never lands on the shovel core.
    stdio_init_all();

    trace_pins_init();
    status_led_init();
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

        // Bounded: a wedged core1 must not also cost us the report that would
        // tell us core1 is wedged.
        g_snap_req = true;
        absolute_time_t give_up = make_timeout_time_ms(50);
        while (g_snap_req && !time_reached(give_up)) tight_loop_contents();
        __dmb();

        // 1 Hz blink: green = link up, red = link down, blue = core1 wedged.
        bool on = uptime_s & 1u;
        if (g_snap_req) {
            g_snap_req = false;
            printf("[%s %6lus] core1 not responding to snapshot request\n",
                   SIDE_NAME, (unsigned long)uptime_s);
            status_led_set(0, 0, on ? 24 : 0);
        } else {
            stats_report(&g_snap, uptime_s);
            if (g_snap.link_up) status_led_set(0, on ? 24 : 0, 0);
            else                status_led_set(on ? 24 : 0, 0, 0);
        }
    }
}
