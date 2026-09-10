#ifndef RS422_CRSF_H
#define RS422_CRSF_H

#include <stdint.h>

// Passive CRSF frame tracker: bytes are forwarded regardless, this only watches.
// Frame: [addr][len][type][payload...][crc], len = type..crc, CRC8 DVB-S2 over type..payload.
typedef struct {
    uint8_t state, len, idx, crc;
} crsf_parser_t;

typedef enum {
    CRSF_NONE,      // mid-frame
    CRSF_OK,        // frame complete, CRC good
    CRSF_BAD_CRC,   // frame complete, CRC bad
    CRSF_BAD_LEN,   // impossible length: ignore the rest of this burst
} crsf_result_t;

void          crsf_init(void);
void          crsf_reset(crsf_parser_t *p);
crsf_result_t crsf_feed(crsf_parser_t *p, uint8_t b);

#endif
