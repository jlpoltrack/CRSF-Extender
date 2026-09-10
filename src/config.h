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
#define LOCAL_PIN        2    // inverted single-wire half duplex (radio bay / module)
#define LINK_UART        uart0
#define LINK_TX_PIN      0
#define LINK_RX_PIN      1
#define TRACE_DIR_PIN    14   // high while we drive the local wire
#define TRACE_LINK_PIN   15   // pulses on each link byte

// ---- Timing ---------------------------------------------------------------
#define LOCAL_BAUD       400000u
#define LINK_BAUD        400000u   // stage 1: transparent, same rate

// Inter-byte gap that marks the end of a burst on the local wire.
// 1.5 character times at 400k = ~37us. Raise if the radio has intra-burst gaps.
#define GUARD_US         40u

// No traffic from the RS-422 link for this long => far end gone; hold local idle.
#define LINK_TIMEOUT_US  20000u

#define REPORT_INTERVAL_US 1000000u

// 125 MHz is the safe bring-up default. 48000 also works (USB runs from PLL_USB
// regardless) and cuts RP2040 draw; revisit once timing is verified on a scope.
#define SYS_CLK_KHZ      125000

#endif
