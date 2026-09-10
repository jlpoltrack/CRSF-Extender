#include "config.h"
#include "stats.h"
#include "local_port.h"
#include "link_port.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/sync.h"
#include <string.h>

#define RING_SZ   512
#define RING_MASK (RING_SZ - 1)

volatile bool g_snap_req = false;
stats_t       g_snap;

static stats_t st;

// Link -> local staging. Producer and consumer are both core1, so no atomics.
static uint8_t  ring[RING_SZ];
static uint16_t r_head, r_tail;

static inline uint16_t ring_count(void) { return (uint16_t)(r_head - r_tail) & RING_MASK; }
static inline bool ring_push(uint8_t b) {
    uint16_t n = (uint16_t)(r_head + 1) & RING_MASK;
    if (n == r_tail) return false;
    ring[r_head] = b; r_head = n; return true;
}
static inline uint8_t ring_pop(void) { uint8_t b = ring[r_tail]; r_tail = (uint16_t)(r_tail + 1) & RING_MASK; return b; }

static void stats_reset_window(stats_t *s) {
    // Sticky across the reporting window: state, not per-second counts.
    uint32_t keep_consec = s->miss_consec;
    uint32_t keep_up     = s->link_up;
    uint8_t  keep_burst[16];
    uint8_t  keep_cap    = s->last_burst_cap;
    uint16_t keep_len    = s->last_burst_len;
    memcpy(keep_burst, s->last_burst, sizeof(keep_burst));

    memset(s, 0, sizeof(*s));
    s->period_min_us = UINT32_MAX;
    s->turn_min_us   = UINT32_MAX;
    s->miss_consec   = keep_consec;
    s->link_up       = keep_up;
    s->last_burst_cap = keep_cap;
    s->last_burst_len = keep_len;
    memcpy(s->last_burst, keep_burst, sizeof(keep_burst));
}

void bridge_core1(void) {
    stats_reset_window(&st);

    bool     local_busy   = false;   // a burst is arriving on the local wire
    bool     driving      = false;   // we own the local wire
    bool     awaiting     = false;   // local burst ended, reply not yet seen
    bool     link_busy    = false;

    uint32_t local_last_us = 0, burst_start_us = 0, burst_end_us = 0;
    uint32_t prev_start_us = 0, link_last_us = 0, tx_last_us = 0;

    for (;;) {
        uint32_t now = time_us_32();

        if (g_snap_req) {
            g_snap = st;
            stats_reset_window(&st);
            __dmb();
            g_snap_req = false;
        }

        // ---- local wire -> link -------------------------------------------
        uint8_t b;
        while (local_rx_get(&b)) {
            now = time_us_32();
            if (!local_busy) {
                local_busy = true;
                burst_start_us = now;
                st.local_bursts++;
                if (prev_start_us) {
                    uint32_t p = now - prev_start_us;
                    if (p < st.period_min_us) st.period_min_us = p;
                    if (p > st.period_max_us) st.period_max_us = p;
                    st.period_sum_us += p; st.period_n++;
                }
                prev_start_us = now;
                // With no far end connected a missing reply is expected, not a fault.
                if (awaiting && st.link_up) { st.miss++; st.miss_consec++; }
                awaiting = false;
                st.last_burst_cap = 0;
                st.last_burst_len = 0;
            }
            local_last_us = now;
            st.local_bytes++;
            if (st.last_burst_cap < sizeof(st.last_burst)) st.last_burst[st.last_burst_cap++] = b;
            if (st.last_burst_len < 0xffff) st.last_burst_len++;
            if (!link_tx_put(b)) st.err_link_ovr++;   // link TX FIFO backed up
        }

        if (local_busy && (now - local_last_us) >= GUARD_US) {
            local_busy = false;
            burst_end_us = local_last_us;
            uint32_t d = burst_end_us - burst_start_us;
            if (d > st.burst_max_us) st.burst_max_us = d;
            awaiting = true;
        }

        // ---- link -> local wire -------------------------------------------
        while (link_rx_get(&b)) {
            now = time_us_32();
            gpio_xor_mask(1u << TRACE_LINK_PIN);
            if (!link_busy) { link_busy = true; st.link_bursts++; }
            link_last_us = now;
            st.link_bytes++;
            st.link_up = 1;
            if (awaiting) {
                uint32_t t = now - burst_end_us;
                if (t < st.turn_min_us) st.turn_min_us = t;
                if (t > st.turn_max_us) st.turn_max_us = t;
                st.turn_n++;
                awaiting = false;
                st.miss_consec = 0;
            }
            if (!ring_push(b)) st.err_local_ovr++;
        }
        if (link_busy && (now - link_last_us) >= GUARD_US) link_busy = false;

        // ---- drive the local wire when it is free --------------------------
        if (!driving) {
            if (ring_count()) {
                if (local_busy) {
                    st.err_contention++;   // reply arrived mid-burst: wait it out
                } else if ((now - local_last_us) >= GUARD_US) {
                    local_drive();
                    driving = true;
                    tx_last_us = now;
                }
            }
        } else {
            while (ring_count() && local_tx_put(ring[r_tail])) {
                ring_pop();
                tx_last_us = time_us_32();
            }
            if (!ring_count() && local_tx_drained()) {
                local_release();
                driving = false;
                local_last_us = time_us_32();   // guard before we trust RX again
            }
        }

        // ---- link watchdog --------------------------------------------------
        if (st.link_up && link_last_us && (now - link_last_us) > LINK_TIMEOUT_US) {
            st.link_up = 0;
            st.link_drops++;
            r_head = r_tail = 0;               // drop stale bytes, hold local idle
        }

        // ---- sticky error flags ---------------------------------------------
        if (local_framing_error()) st.err_frame++;
        if (local_rx_overrun())    st.err_local_ovr++;
        uint32_t ovr; st.err_link += link_take_errors(&ovr); st.err_link_ovr += ovr;

        (void)tx_last_us;
    }
}
