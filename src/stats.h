#ifndef RS422_STATS_H
#define RS422_STATS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // Volume
    uint32_t local_bytes;     // bytes received from the local half-duplex wire
    uint32_t link_bytes;      // bytes received from the RS-422 link
    uint32_t local_bursts;    // bursts seen on the local wire
    uint32_t link_bursts;     // bursts seen from the link

    // Frame cadence, measured local-burst-start to local-burst-start
    uint32_t period_min_us, period_max_us, period_sum_us, period_n;
    uint32_t burst_max_us;    // longest local burst

    // Turnaround: end of local burst -> first byte back from the link
    uint32_t turn_min_us, turn_max_us, turn_n;

    // Health
    uint32_t miss;            // frames where no reply came back before the next one
    uint32_t miss_consec;     // current consecutive-miss run
    uint32_t err_frame;       // PIO framing errors on the local wire
    uint32_t err_local_ovr;   // PIO RX FIFO overrun
    uint32_t err_link;        // UART framing/parity/break on the link
    uint32_t err_link_ovr;    // UART RX overrun on the link
    uint32_t err_link_drop;   // link TX FIFO full: a byte from the local wire was lost
    uint32_t err_ring_ovf;    // link->local staging ring full: a reply byte was lost
    uint32_t err_drive_timeout; // held the local wire too long; forced release
    uint32_t err_contention;  // link data ready while the local wire was busy
    uint32_t link_drops;      // link-timeout events
    uint32_t link_up;         // 1 if link seen recently

    // Bring-up aid: head of the most recent local burst. Wrong baud or wrong
    // polarity shows up here immediately as garbage / framing errors.
    uint8_t  last_burst[16];
    uint8_t  last_burst_cap;  // bytes captured in last_burst
    uint16_t last_burst_len;  // full length of that burst
} stats_t;

// Core0 requests a snapshot; core1 copies and clears the windowed fields.
extern volatile bool  g_snap_req;
extern stats_t        g_snap;

void stats_report(const stats_t *s, uint32_t uptime_s);

#endif
