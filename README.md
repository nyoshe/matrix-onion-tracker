# matrix-onion-tracker

A crypto price + onion-link (.onion) availability board on an ELEGOO
ESP-WROOM-32 board, drawing on a Waveshare 64x32 RGB (HUB75) LED matrix.
Same hardware as [led-matrix-audio-visualizer](../led-matrix-audio-visualizer),
minus the SparkFun Sound Detector -- this one will poll prices and onion
uptime over WiFi instead of listening to a mic.

**Status: scaffold only.** Nothing fetches real data yet. The two screens
below draw hardcoded mock values so the matrix, BOOT-button switching and
serial commands can be demoed before any networking, price-API or
onion-checking logic exists.

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
  3V3   ● │             │ ●  GND   ◄ matrix GND
  EN    ● │             │ ●  23    ◄ A
  VP 36 ● │             │ ●  22    ◄ B
  VN 39 ● │             │ ●  TX
  34    ● │             │ ●  21    ◄ C
  35    ● │             │ ●  19    ◄ D
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
| B1 | 25 | | GND | GND (right header, below 21) |
| R2 | 26 | | G2 | 27 |
| B2 | 14 | | E | — (not used at 32 rows) |
| A | 23 | | B | 22 |
| C | 21 | | D | 19 |
| CLK | 18 | | LAT | 5 |
| OE | 17 | | GND | — |

The colour lines run down the left header and the address/control lines down the right header, in the same order as the HUB75 connector.

Power the matrix from its own 5 V supply rated for at least 2–4 A, and tie its GND to the ESP32's GND.

## Screens (currently mock data)

| # | Name | What it shows |
|---|---|---|
| 0 | Crypto Ticker | Scrolling line of symbol/price pairs, green/red for up/down |
| 1 | Onion Status | Small list of .onion service labels with an up/down dot each |

**Switching:** press the board's **BOOT** button to go to the next screen. Over serial (`make monitor`), send `n` for next, `p` for previous, `l` to list them, or `0`–`9` to pick one. Send `d` to print the measured render frame rate. To rotate through them automatically, set `AUTO_CYCLE_MS` in `src/config.h`.

## Project layout

```
matrix-onion-tracker/
├── matrix-onion-tracker.ino              render loop: draw active screen, button/serial switching
└── src/
    ├── config.h                          pins, panel size, switching settings, WiFi placeholders
    ├── core/
    │   ├── Matrix.{h,cpp}                 HUB75 panel setup
    │   ├── Color.h                        rgb565 / levelColor / hueColor helpers
    │   ├── Screen.h                       base class + REGISTER_SCREEN
    │   └── Registry.cpp                   list of registered screens
    └── screens/                           one file per screen
        ├── CryptoTicker.cpp               mock price ticker
        └── OnionStatus.cpp                mock onion-service status list
```

## What's not implemented yet

- WiFi connection (`WIFI_SSID`/`WIFI_PASSWORD` in `src/config.h` are placeholders, unused)
- Fetching real prices from an exchange/price API
- Checking real .onion link availability (needs a Tor-capable proxy somewhere the ESP32 can reach, since the chip itself can't speak the Tor protocol)
- Any kind of polling, caching or error/timeout handling for either of the above

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
- **Pixels shimmer:** try `cfg.clkphase = false;`.
- **Brightness:** change `PANEL_BRIGHTNESS` in `src/config.h`.
