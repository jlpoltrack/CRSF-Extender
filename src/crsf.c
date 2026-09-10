#include "crsf.h"

#define LEN_MIN 2    // type + crc
#define LEN_MAX 62   // 64-byte frame limit minus addr and len

enum { S_ADDR, S_LEN, S_BODY, S_SKIP };

static uint8_t crc_tab[256];

void crsf_init(void) {
    for (unsigned i = 0; i < 256; i++) {
        uint8_t c = (uint8_t)i;
        for (int k = 0; k < 8; k++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0xD5) : (uint8_t)(c << 1);
        crc_tab[i] = c;
    }
}

void crsf_reset(crsf_parser_t *p) { p->state = S_ADDR; }

crsf_result_t crsf_feed(crsf_parser_t *p, uint8_t b) {
    switch (p->state) {
    case S_ADDR:
        p->state = S_LEN;
        return CRSF_NONE;
    case S_LEN:
        if (b < LEN_MIN || b > LEN_MAX) { p->state = S_SKIP; return CRSF_BAD_LEN; }
        p->len = b; p->idx = 0; p->crc = 0; p->state = S_BODY;
        return CRSF_NONE;
    case S_BODY:
        if (++p->idx < p->len) { p->crc = crc_tab[p->crc ^ b]; return CRSF_NONE; }
        p->state = S_ADDR;
        return b == p->crc ? CRSF_OK : CRSF_BAD_CRC;
    default:
        return CRSF_NONE;
    }
}
