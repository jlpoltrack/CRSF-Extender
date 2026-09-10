#ifndef RS422_LOCAL_PORT_H
#define RS422_LOCAL_PORT_H

#include <stdint.h>
#include <stdbool.h>

// Inverted single-wire half-duplex port on LOCAL_PIN.
// Idles as an input; the wire is only driven between local_drive()/local_release().
void local_port_init(void);

bool local_rx_get(uint8_t *b);      // non-blocking
bool local_rx_overrun(void);        // reads and clears the sticky PIO flag
bool local_framing_error(void);     // reads and clears the sticky PIO flag

void local_drive(void);             // stop RX, take the wire as an output
bool local_tx_put(uint8_t b);       // non-blocking; false if the TX FIFO is full
bool local_tx_drained(void);        // last bit shifted out
void local_release(void);           // give the wire back, restart RX

#endif
