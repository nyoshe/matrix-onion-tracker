#pragma once

// All hardware and tuning constants in one place.

#include <stdint.h>

// ---- Matrix -----------------------------------------------------------------
constexpr int PANEL_WIDTH  = 64;
constexpr int PANEL_HEIGHT = 32;
constexpr int PANEL_CHAIN  = 1;
constexpr uint8_t PANEL_BRIGHTNESS = 96;  // 0-255

// Pin map for the ELEGOO ESP-WROOM-32 (38-pin DevKitC layout), chosen so each
// group of wires lands on a consecutive run of header pins and the boot
// strapping pins (0, 2, 12, 15) are never driven. Same matrix wiring as
// led-matrix-audio-visualizer, minus the sound detector.
namespace Pins {
// HUB75 colour lines: left header, six pins in a row (32 33 25 26 27 14).
constexpr int8_t R1 = 32;
constexpr int8_t G1 = 33;
constexpr int8_t B1 = 25;
constexpr int8_t R2 = 26;
constexpr int8_t G2 = 27;
constexpr int8_t B2 = 14;
// HUB75 address/control: right header, a run from 23 down to 17
// (skipping TX/RX and the GND pin, which the matrix GND can use).
constexpr int8_t A   = 23;
constexpr int8_t B   = 22;
constexpr int8_t C   = 21;
constexpr int8_t D   = 19;
constexpr int8_t CLK = 18;
constexpr int8_t LAT = 5;
constexpr int8_t OE  = 17;
constexpr int8_t E   = -1;  // unused on a 1/16 scan 32-row panel

// The board's BOOT button (GPIO0 has an on-board pull-up and is only a
// strapping pin at reset, so it's free to read afterwards).
constexpr int8_t BUTTON = 0;
}  // namespace Pins

// ---- Rendering ---------------------------------------------------------------
constexpr uint32_t RENDER_FPS = 30;

// ---- Screen switching ---------------------------------------------------------
constexpr uint32_t AUTO_CYCLE_MS = 0;  // switch screen every N ms; 0 = off

// ---- Networking (not wired up yet) --------------------------------------------
// Placeholders for when price/onion polling gets added; nothing reads these yet.
constexpr const char* WIFI_SSID     = "";
constexpr const char* WIFI_PASSWORD = "";
