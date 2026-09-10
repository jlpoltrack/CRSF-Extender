# RS-422 bridge for a 400k inverted half-duplex radio link

Extends the 400 kbaud, inverted, single-wire half-duplex link between an EdgeTX
radio and its external Tx module over 4-wire RS-422. One RP2040 at each end.

Frame budget: 250 Hz / 4 ms. The radio transmits for up to 2 ms, the module
replies in the remainder. Bytes are forwarded **cut-through** (~30 us per hop);
never buffer a whole frame — a 64-byte frame is 1.6 ms and would consume the
entire reply window.

## Build

    export PICO_SDK_PATH=~/pico-sdk
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j8

Produces `build/rs422_radio.uf2` and `build/rs422_module.uf2`. Same sources; the
side is the compile-time constant `BRIDGE_SIDE` (`config.h`), not a strap pin.

Both images are `copy_to_ram`: the whole program runs from SRAM, so there are no
XIP cache misses in the forwarding path and flash is idle at runtime.

## Pins

| GP | Function |
|----|----------|
| 12 / 13 | UART0 TX / RX — RS-422 link at 1 Mbaud, non-inverted, full duplex |
| 9  | Local inverted single-wire half duplex (radio module bay / Tx module) |
| 10 / 11 | Driven low as spare grounds (12 mA max each; signal reference only) |
| 14 | Trace: high while this board drives the local wire |
| 15 | Trace: toggles on each byte from the link |
| 16 | On-board WS2812 (PIO1): 1 Hz blink — green link up, red link down, blue core1 wedged |

Target board is the Waveshare RP2040-Zero (`PICO_BOARD=waveshare_rp2040_zero`).

Inversion is done with the pad's `OUTOVER`/`INOVER` override fields, so the PIO
programs are ordinary idle-high 8N1 UARTs. The differential link stays
non-inverted, which means transceiver failsafe bias (idle = logic high) matches
UART idle — important because the battery-powered far end is often unpowered.

## Design notes

- `local_port.c` — PIO0 SM0 (TX) + SM1 (RX) share GP9. The TX SM owns `pindirs`;
  the RX SM is stopped while we drive, otherwise it just reads our own echo.
- `link_port.c` — UART0, plain full duplex, no direction control needed.
- `bridge.c` — core1 only. Polls both directions, detects burst boundaries by a
  `GUARD_US` (40 us ≈ 1.5 character times) inter-byte gap, and stages link→local
  bytes in a ring so a reply arriving mid-burst waits rather than colliding.
- The link runs at 1 Mbaud while the local wire is 400k, so the link always
  drains faster than it fills and each hop costs 10 us rather than 25. The far
  end regenerates 400k timing on its own wire, so the rates need not match.
- `main.c` — core0 only. USB is initialised **before** core1 launches so
  TinyUSB's IRQ binds to core0 and never perturbs the forwarding loop.
  Core1 never calls `printf`.

## Bring-up, one board only

Flash `rs422_radio.uf2`, connect GP9 to the module bay half-duplex pin, and open
the USB serial port. With no far end attached the report still prints every
second; `link` stays `DOWN` and misses are not counted (a missing reply is
expected when nothing is connected).

    === RS-422 bridge [RADIO ] ===
    [RADIO       7s] local:  250 bursts   6000 B | link:    0 bursts      0 B | DOWN
                   period 3996/4000/4004 us  burst_max 1714 us  turn 0/0 us (n=0)
                   miss 0 (0 consec)  linkdrop 0  cont 0  hold_to 0
                   err: local frm 0 ovr 0 | link frm 0 ovr 0 | dropped tx 0 rx 0
                   last burst 24 B: C8 18 16 E0 03 1F 2B C0 F7 81 0F 7C E0 03 1F F8 ...

What to check, in order:

1. `local bursts` ≈ 250/s and `period` clustered near 4000 us — the wire is alive
   and the gap detector is tuned.
2. `last burst` hex is plausible protocol data (CRSF starts `C8`), and
   `err: frm` stays 0. Garbage plus climbing framing errors means the polarity or
   baud is wrong — check the `OUTOVER`/`INOVER` calls first.
3. `burst_max` under ~2000 us, confirming the radio's share of the frame.

`turn`, `miss` and `linkdrop` only become meaningful once the second board and
the transceivers are in.

You cannot debug PIO timing from a 1 Hz text report — put a logic analyzer on
GP14/GP15 alongside GP9.

## Not yet done

Stage 2 (framed link at 1–2 Mbaud with sync + CRC, plus far-end battery
telemetry) is deliberately not built. Get the transparent pipe measured first.
