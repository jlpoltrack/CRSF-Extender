#ifndef RS422_CONFIG_H
#define RS422_CONFIG_H

// ---- Side selection -------------------------------------------------------
// BRIDGE_SIDE is set by CMake per target: build rs422_radio and rs422_module.
#define SIDE_RADIO   1
#define SIDE_MODULE  2

#ifndef BRIDGE_SIDE
#error "BRIDGE_SIDE must be defined as SIDE_RADIO or SIDE_MODULE"
#endif

#if BRIDGE_SIDE == SIDE_RADIO
#define SIDE_NAME "RADIO "
#elif BRIDGE_SIDE == SIDE_MODULE
#define SIDE_NAME "MODULE"
#else
#error "BRIDGE_SIDE must be SIDE_RADIO or SIDE_MODULE"
#endif

// ---- Pins -----------------------------------------------------------------
#define LOCAL_PIN        9    // inverted single-wire half duplex (radio bay / module)
#define LINK_UART        uart0  // GP12/13 must stay on uart0 (TX/RX pin mux)
#define LINK_TX_PIN      12
#define LINK_RX_PIN      13
#define TRACE_DIR_PIN    14   // high while we drive the local wire
#define TRACE_LINK_PIN   15   // pulses on each link byte
#define STATUS_LED_PIN   16   // RP2040-Zero on-board WS2812
// Driven low as spare grounds. Signal reference only: 12 mA max per pin.
#define GND_PIN_A        10
#define GND_PIN_B        11

// ---- Timing ---------------------------------------------------------------
#define LOCAL_BAUD       400000u
// Faster than LOCAL_BAUD on purpose: the link must always drain quicker than the
// local wire fills, and the far end retimes every byte to 400k anyway.
#define LINK_BAUD        1000000u

// Inter-byte gap that marks the end of a burst on the local wire.
// 1.5 character times at 400k = ~37us. Raise if the radio has intra-burst gaps.
#define GUARD_US         40u

// No traffic from the RS-422 link for this long => far end gone; hold local idle.
#define LINK_TIMEOUT_US  20000u

// Hard cap on how long we may drive the local wire, so a misbehaving far end can
// never jam the radio's bus. Comfortably longer than one reply, shorter than a frame.
#define MAX_DRIVE_US     3000u

#define REPORT_INTERVAL_US 1000000u

// 125 MHz is the safe bring-up default. 48000 also works (USB runs from PLL_USB
// regardless) and cuts RP2040 draw; revisit once timing is verified on a scope.
#define SYS_CLK_KHZ      125000

#endif
