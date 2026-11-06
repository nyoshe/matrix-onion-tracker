#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// The HUB75 panel. It's an Adafruit_GFX, so lines, circles, text etc. work.
using Display = MatrixPanel_I2S_DMA;

namespace Matrix {
// Initialises the panel (double buffered) or halts if it can't.
Display& begin();

// Runtime brightness (0-255), adjustable from the on-screen menu.
void setBrightness(uint8_t level);
uint8_t brightness();
}  // namespace Matrix
