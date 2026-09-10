#include "stats.h"
#include "config.h"
#include <stdio.h>

static uint32_t or_zero(uint32_t v) { return v == UINT32_MAX ? 0 : v; }

void stats_report(const stats_t *s, uint32_t uptime_s) {
    uint32_t p_avg = s->period_n ? s->period_sum_us / s->period_n : 0;

    printf("[%s %6lus] local: %4lu bursts %6lu B | link: %4lu bursts %6lu B | %s\n",
           SIDE_NAME, (unsigned long)uptime_s,
           (unsigned long)s->local_bursts, (unsigned long)s->local_bytes,
           (unsigned long)s->link_bursts,  (unsigned long)s->link_bytes,
           s->link_up ? "UP" : "DOWN");

    printf("               period %lu/%lu/%lu us  burst_max %lu us  turn %lu/%lu us (n=%lu)\n",
           (unsigned long)or_zero(s->period_min_us), (unsigned long)p_avg,
           (unsigned long)s->period_max_us, (unsigned long)s->burst_max_us,
           (unsigned long)or_zero(s->turn_min_us), (unsigned long)s->turn_max_us,
           (unsigned long)s->turn_n);

    printf("               miss %lu (%lu consec)  err: frm %lu lovr %lu link %lu kovr %lu cont %lu  linkdrop %lu\n",
           (unsigned long)s->miss, (unsigned long)s->miss_consec,
           (unsigned long)s->err_frame, (unsigned long)s->err_local_ovr,
           (unsigned long)s->err_link, (unsigned long)s->err_link_ovr,
           (unsigned long)s->err_contention, (unsigned long)s->link_drops);

    if (s->last_burst_len) {
        printf("               last burst %u B:", (unsigned)s->last_burst_len);
        for (unsigned i = 0; i < s->last_burst_cap; i++) printf(" %02X", s->last_burst[i]);
        printf("%s\n", s->last_burst_cap < s->last_burst_len ? " ..." : "");
    } else {
        printf("               no traffic on the local wire\n");
    }
}
