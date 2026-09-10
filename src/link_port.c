#include "link_port.h"
#include "config.h"

#include "hardware/uart.h"
#include "hardware/gpio.h"

void link_port_init(void) {
    uart_init(LINK_UART, LINK_BAUD);
    gpio_set_function(LINK_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LINK_RX_PIN, GPIO_FUNC_UART);

    // Solo bring-up: with no transceiver fitted this pin floats and noise would
    // be decoded as link traffic, which we would then drive onto the radio's wire.
    gpio_pull_up(LINK_RX_PIN);
    uart_set_format(LINK_UART, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(LINK_UART, false, false);
    uart_set_fifo_enabled(LINK_UART, true);
}

bool link_rx_get(uint8_t *b) {
    if (!uart_is_readable(LINK_UART)) return false;
    *b = (uint8_t)uart_get_hw(LINK_UART)->dr;
    return true;
}

bool link_tx_put(uint8_t b) {
    if (!uart_is_writable(LINK_UART)) return false;
    uart_get_hw(LINK_UART)->dr = b;
    return true;
}

uint32_t link_take_errors(uint32_t *overruns) {
    uart_hw_t *hw = uart_get_hw(LINK_UART);
    uint32_t rsr = hw->rsr & 0xfu;
    if (!rsr) { *overruns = 0; return 0; }
    hw->rsr = 0;   // any write clears
    *overruns = (rsr & UART_UARTRSR_OE_BITS) ? 1 : 0;
    return (rsr & ~UART_UARTRSR_OE_BITS) ? 1 : 0;
}
