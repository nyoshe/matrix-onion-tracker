# matrix-onion-tracker

A crypto price + onion-link (.onion) availability board on an ELEGOO
ESP-WROOM-32 board, drawing on a Waveshare 64x32 RGB (HUB75) LED matrix.
Same hardware as [led-matrix-audio-visualizer](../led-matrix-audio-visualizer),
minus the SparkFun Sound Detector -- this one will poll prices and onion
uptime over WiFi instead of listening to a mic.

**Status: partial.** WiFi connects to a saved network automatically, or
starts an access point with a web config page to add one. The two screens
still draw hardcoded mock values -- price and onion-uptime polling aren't
wired up yet.

## Build environment

```sh
nix develop          # or `direnv allow` to load it automatically
make setup           # first time only: installs the ESP32 core and libraries into .arduino/
make upload          # compile and flash to /dev/ttyUSB0 (override with PORT=/dev/ttyUSBx)
make monitor         # serial console at 115200 baud
```

The ESP32 toolchains that arduino-cli downloads are prebuilt binaries for a normal Linux layout, so the flake wraps `arduino-cli` in an FHS environment. Without it they won't run on NixOS.

## Wiring

The pin map targets the **ELEGOO ESP-WROOM-32** dev board, which has the 38-pin ESP32-DevKitC layout. Each group of wires goes to a run of neighbouring header pins. The boot-strapping pins (0, 2, 12, 15) are left free so the board always boots and flashes cleanly. There's no microphone this time, so the audio input pin from the visualizer project (VP/36) is simply unused.

```
              antenna
          ┌─────────────┐
  3V3   ● │ ◄ ENC +     │ ●  GND   ◄ matrix GND, ENC GND
  EN    ● │             │ ●  23    ◄ A
  VP 36 ● │             │ ●  22    ◄ B
  VN 39 ● │ ◄ ENC SW    │ ●  TX
  34    ● │ ◄ ENC DT    │ ●  21    ◄ C
  35    ● │ ◄ ENC CLK   │ ●  19    ◄ D
  32    ● │ ◄ R1        │ ●  18    ◄ CLK
  33    ● │ ◄ G1        │ ●  5     ◄ LAT
  25    ● │ ◄ B1        │ ●  17    ◄ OE
  26    ● │ ◄ R2        │ ●  16
  27    ● │ ◄ G2        │ ●  4
  14    ● │ ◄ B2        │ ●  0
  12    ● │             │ ●  2
  GND   ● │             │ ●  15
  13    ● │             │ ●  SD1
  SD2   ● │             │ ●  SD0
  SD3   ● │             │ ●  CLK
  CMD   ● │             │
  5V    ● │             │
          └────[USB]────┘
```

| HUB75 (2×8 ribbon) | ESP32 | | HUB75 | ESP32 |
|---|---|---|---|---|
| R1 | 32 | | G1 | 33 |
| B1 | 25 | | GND | GND (top of right header) |
| R2 | 26 | | G2 | 27 |
| B2 | 14 | | E | — (not used at 32 rows) |
| A | 23 | | B | 22 |
| C | 21 | | D | 19 |
| CLK | 18 | | LAT | 5 |
| OE | 17 | | GND | GND (same pin is fine) |

The colour lines run down the left header and the address/control lines down the right header, in the same order as the HUB75 connector.

Wire **both** HUB75 GND pins. They can land on the same ESP32 GND pin, since all the ESP32's GND pins are one net. With only one ground wire, ground bounce corrupted bits whenever many data lines switched at once.

Power the matrix from its own 5 V supply rated for at least 2–4 A, and tie its GND to the ESP32's GND.

| KY-040 | ESP32 |
|---|---|
| SW | VN / 39 |
| CLK | 35 |
| DT | 34 |
| + | 3V3 |
| GND | GND |

SW, CLK and DT land on three consecutive rows of the left header (VN, 34, 35), so all three wires come off together. All three are input-only ADC1 pins with no internal pull resistor, so they rely on the KY-040 module's own onboard pull-ups (standard on these breakout boards).

## Screens (currently mock data)

| # | Name | What it shows |
|---|---|---|
| 0 | Crypto Ticker | Scrolling line of symbol/price pairs, green/red for up/down |
| 1 | Onion Status | Small list of .onion service labels with an up/down dot each |

**Switching:** press the board's **BOOT** button to go to the next screen. Over serial (`make monitor`), send `n` for next, `p` for previous, `l` to list them, or `0`–`9` to pick one. Send `d` to print the measured render frame rate. To rotate through them automatically, set `AUTO_CYCLE_MS` in `src/config.h`.

## Menu (KY-040)

Press the encoder to open a small menu over whatever screen is showing: **Screen**, **Brightness**, **Back**. Turn to move the highlight, press to open that item, turn to change its value, press again to confirm and return to the list. Selecting **Back** (or pressing while it's highlighted) closes the menu and returns to the active screen. State lives in `src/core/Menu.cpp`; the encoder itself is read in `src/core/Encoder.cpp`.

Without an encoder in hand, drive the menu over serial (`make monitor`): `<`/`>` turn, `c` clicks.

## WiFi

At boot, each saved network (`src/core/NetworkStore`) is tried in turn (`WIFI_PER_NETWORK_TIMEOUT_MS` each, see `src/config.h`), and the panel shows **Connecting...** plus which network it's currently trying. If one connects, that's it -- the board is on your network, and the panel moves on to its normal screens. If none do, or none are saved, it starts an access point instead and the panel shows that state persistently (not just at boot) until a network connects:

- **SSID:** `WIFI_AP_SSID` in `src/config.h` (`config` by default)
- **Password:** none -- it's an open network, for easy access during setup
- **Config page:** connect to that AP and visit `http://192.168.4.1` (shown on the panel too)

The panel's AP-mode screen is drawn in `Net::draw()` (`src/core/Net.cpp`); if `WIFI_AP_SSID` is long, it can run past the panel's 64px width at the current font, since it's static text rather than a scrolling ticker.

The page lists saved networks with a **Delete** button each, and a form to add one by SSID and password. Adding a network saves it and immediately tries connecting to it, while keeping the access point up (dual AP+STA mode) so the page stays reachable if it fails. On success, the access point drops and the page is reachable at the new network's IP instead (printed over serial; the panel doesn't show the STA IP once connected).

This same page is meant to grow into the place for other settings later (tracked currencies/sites), not just WiFi -- see `src/core/ConfigPortal.cpp`.

## Project layout

```
matrix-onion-tracker/
├── matrix-onion-tracker.ino              render loop: draw active screen or menu, button/serial/encoder input
└── src/
    ├── config.h                          pins, panel size, switching settings, timings
    ├── core/
    │   ├── Matrix.{h,cpp}                 HUB75 panel setup, runtime brightness
    │   ├── Color.h                        rgb565 / levelColor / hueColor helpers
    │   ├── Screen.h                       base class + REGISTER_SCREEN
    │   ├── Registry.cpp                   list of registered screens
    │   ├── Nav.{h,cpp}                    which screen is active (shared by sketch + menu)
    │   ├── Encoder.{h,cpp}                KY-040 quadrature + button reading
    │   ├── Menu.{h,cpp}                   on-screen menu state machine + drawing
    │   ├── NetworkStore.{h,cpp}           persisted list of saved WiFi networks (NVS)
    │   ├── Net.{h,cpp}                    tries saved networks, falls back to an AP
    │   └── ConfigPortal.{h,cpp}           web page to add/remove saved networks
    └── screens/                           one file per screen
        ├── CryptoTicker.cpp               mock price ticker
        └── OnionStatus.cpp                mock onion-service status list
```

## What's not implemented yet

- Fetching real prices from an exchange/price API
- Checking real .onion link availability (needs a Tor-capable proxy somewhere the ESP32 can reach, since the chip itself can't speak the Tor protocol)
- Any kind of polling, caching or error/timeout handling for either of the above
- On-screen status once connected (STA IP) -- serial-only for now; AP mode and connecting are shown
- Any config page content beyond WiFi networks (the planned currencies/sites settings)

## Adding a screen

Create one file in `src/screens/`. Nothing else needs editing, because the file registers itself:

```cpp
// src/screens/Hello.cpp
#include "../core/Screen.h"

class Hello : public Screen {
 public:
  const char* name() const override { return "Hello"; }

  void draw(Display& display) override {
    display.setCursor(2, 2);
    display.setTextColor(0xFFFF);
    display.print("hi");
  }
};

REGISTER_SCREEN(Hello, 30);  // the number sets its position in the rotation
```

The screen is cleared before `draw()` and shown afterwards. Draw the whole picture every frame and keep any history (scroll position, etc.) in member variables. Reset that history in `reset()`, which runs whenever the screen is selected. `Display` is an Adafruit_GFX, so lines, circles, rectangles and text all work.

## Tuning

- **Panel stays dark or shows garbage:** some Waveshare panels use an FM6126A driver. Uncomment `cfg.driver = HUB75_I2S_CFG::FM6126A;` in `src/core/Matrix.cpp`.
- **Pixels shimmer or show the wrong colour in fixed spots:** `cfg.clkphase` in `src/core/Matrix.cpp` is set to `false` because this panel misread R/B bits on the default edge (white showed as green). If a different panel misbehaves, try setting it back to `true`. The clock is already at the library's minimum of 8 MHz.
- **Brightness:** change `PANEL_BRIGHTNESS` in `src/config.h` (startup default) or use the menu's **Brightness** item at runtime.
- **Encoder turns the wrong way:** swap `ENCODER_CLK`/`ENCODER_DT` in `src/config.h`.
- **Encoder skips or double-counts steps:** `src/core/Encoder.cpp` decodes CLK/DT with a quadrature state machine (interrupts on both pins, no time-based debounce -- skipping an edge to "debounce" it would desync the table from the pins' real state). If it still misbehaves, it's more likely a wiring/contact issue than a timing one.
- **Button is flaky, especially once WiFi is in use:** GPIO39 (the `SW` pin) gets brief spurious pull-downs from the radio's ADC; `Encoder::pollButton()` requires two consecutive polls to agree before trusting a level change. Loosen or tighten that in `src/core/Encoder.cpp` if presses feel laggy or still bounce.
