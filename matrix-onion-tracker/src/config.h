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
// G/B swapped from the physical 33/25 and 27/14 pin order: green was
// rendering as blue, meaning the G/B wires are crossed on this board.
constexpr int8_t R1 = 32;
constexpr int8_t G1 = 25;
constexpr int8_t B1 = 33;
constexpr int8_t R2 = 26;
constexpr int8_t G2 = 14;
constexpr int8_t B2 = 27;
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

// KY-040 rotary encoder, for the on-screen menu. VN/34/35 are three
// consecutive rows on the left header, so all three wires land next to each
// other. All three are input-only ADC1 pins with no internal pull-up, so
// they rely on the KY-040 module's own onboard pull-up resistors (standard
// on these breakouts).
constexpr int8_t ENCODER_SW  = 39;  // VN
constexpr int8_t ENCODER_CLK = 35;
constexpr int8_t ENCODER_DT  = 34;
}  // namespace Pins

// ---- Rendering ---------------------------------------------------------------
constexpr uint32_t RENDER_FPS = 30;

// ---- Screen switching ---------------------------------------------------------
constexpr uint32_t AUTO_CYCLE_MS = 0;  // switch screen every N ms; 0 = off

// ---- Menu (KY-040 rotary encoder) --------------------------------------------
constexpr uint8_t MENU_BRIGHTNESS_STEP = 8;  // brightness change per detent

// ---- WiFi ---------------------------------------------------------------------
constexpr uint32_t WIFI_PER_NETWORK_TIMEOUT_MS = 8000;  // before trying the next saved network
constexpr int WIFI_MAX_SAVED_NETWORKS = 8;

// Access point started when no saved network connects; the config portal
// (src/core/ConfigPortal.*) is reachable there at 192.168.4.1. Open (no
// password) so it's easy to get to for setup.
constexpr const char* WIFI_AP_SSID = "config";

// ---- Price feed -----------------------------------------------------------------
// CoinGecko's keyless tier shares ~5 requests/min per IP; this budgets for it.
constexpr uint32_t PRICE_POLL_INTERVAL_MS = 60000;           // one combined call, both coins
constexpr uint32_t PRICE_HISTORY_POLL_INTERVAL_MS = 300000;  // one call per coin, so less often
constexpr uint32_t PRICE_RETRY_DELAY_MS = 15000;             // after a failed fetch, not the full interval
constexpr const char* PRICE_API_URL =
    "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,monero&vs_currencies=usd&include_24hr_change=true";
