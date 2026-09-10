#ifndef RS422_LINK_PORT_H
#define RS422_LINK_PORT_H

#include <stdint.h>
#include <stdbool.h>

// Full-duplex RS-422 link. Plain hardware UART, non-inverted, no direction control.
void link_port_init(void);
bool link_rx_get(uint8_t *b);       // non-blocking
bool link_tx_put(uint8_t b);        // non-blocking; false if the TX FIFO is full
uint32_t link_take_errors(uint32_t *overruns);  // reads and clears sticky flags

#endif
