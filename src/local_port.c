#include "local_port.h"
#include "config.h"

#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "uart_tx.pio.h"
#include "uart_rx.pio.h"

#define PIO_LOCAL   pio0
#define SM_TX       0
#define SM_RX       1

// uart_rx.pio raises `irq 4 rel`, which for SM n resolves to IRQ 4 | ((4+n) & 3).
#define IRQ_FRAMING (4u | ((4u + SM_RX) & 3u))

static uint off_tx, off_rx;

void local_port_init(void) {
    off_tx = pio_add_program(PIO_LOCAL, &uart_tx_program);
    off_rx = pio_add_program(PIO_LOCAL, &uart_rx_program);

    pio_gpio_init(PIO_LOCAL, LOCAL_PIN);

    // The protocol is inverted; do it in the pad so the PIO programs stay normal.
    gpio_set_outover(LOCAL_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(LOCAL_PIN, GPIO_OVERRIDE_INVERT);

    // Physical low == inverted idle, so a pulldown reads as UART idle if the
    // wire is briefly undriven by both ends.
    gpio_pull_down(LOCAL_PIN);

    uart_tx_program_init(PIO_LOCAL, SM_TX, off_tx, LOCAL_PIN, LOCAL_BAUD);
    uart_rx_program_init(PIO_LOCAL, SM_RX, off_rx, LOCAL_PIN, LOCAL_BAUD);

    pio_sm_set_consecutive_pindirs(PIO_LOCAL, SM_TX, LOCAL_PIN, 1, false); // input
}

bool local_rx_get(uint8_t *b) {
    if (pio_sm_is_rx_fifo_empty(PIO_LOCAL, SM_RX)) return false;
    // Shift-right leaves the last 8 bits sampled in the MSBs of the word.
    *b = (uint8_t)(PIO_LOCAL->rxf[SM_RX] >> 24);
    return true;
}

bool local_rx_overrun(void) {
    uint32_t bit = 1u << (PIO_FDEBUG_RXSTALL_LSB + SM_RX);
    if (!(PIO_LOCAL->fdebug & bit)) return false;
    PIO_LOCAL->fdebug = bit;
    return true;
}

bool local_framing_error(void) {
    uint32_t bit = 1u << IRQ_FRAMING;
    if (!(PIO_LOCAL->irq & bit)) return false;
    PIO_LOCAL->irq = bit;
    return true;
}

void local_drive(void) {
    // Stop RX first: while we drive the wire the RX SM would just read our echo.
    pio_sm_set_enabled(PIO_LOCAL, SM_RX, false);
    pio_sm_clear_fifos(PIO_LOCAL, SM_RX);

    PIO_LOCAL->fdebug = 1u << (PIO_FDEBUG_TXSTALL_LSB + SM_TX);
    pio_sm_set_consecutive_pindirs(PIO_LOCAL, SM_TX, LOCAL_PIN, 1, true);
    gpio_put(TRACE_DIR_PIN, 1);
}

bool local_tx_put(uint8_t b) {
    if (pio_sm_is_tx_fifo_full(PIO_LOCAL, SM_TX)) return false;
    PIO_LOCAL->fdebug = 1u << (PIO_FDEBUG_TXSTALL_LSB + SM_TX);
    PIO_LOCAL->txf[SM_TX] = b;
    return true;
}

bool local_tx_drained(void) {
    return (PIO_LOCAL->fdebug & (1u << (PIO_FDEBUG_TXSTALL_LSB + SM_TX))) != 0;
}

void local_release(void) {
    // TXSTALL asserts as the SM re-enters `pull`, i.e. at the start of the stop
    // bit; drive it for one full bit plus margin before letting go.
    busy_wait_us_32(1000000u / LOCAL_BAUD + 1);

    pio_sm_set_consecutive_pindirs(PIO_LOCAL, SM_TX, LOCAL_PIN, 1, false);
    gpio_put(TRACE_DIR_PIN, 0);

    pio_sm_clear_fifos(PIO_LOCAL, SM_RX);
    pio_sm_restart(PIO_LOCAL, SM_RX);
    pio_sm_exec(PIO_LOCAL, SM_RX, pio_encode_jmp(off_rx));
    pio_sm_set_enabled(PIO_LOCAL, SM_RX, true);
}
