#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// The HUB75 panel. It's an Adafruit_GFX, so lines, circles, text etc. work.
using Display = MatrixPanel_I2S_DMA;

// All text uses TomThumb (3x5 glyphs, 4px advance), set in Matrix::begin().
// With a custom GFX font, setCursor's y is the text baseline, not the top.
constexpr int FONT_ADVANCE = 4;
constexpr int FONT_ASCENT  = 5;  // baseline sits this far below a line's top

namespace Matrix {
// Initialises the panel (double buffered) or halts if it can't.
Display& begin();

// Runtime brightness (0-255), adjustable from the on-screen menu.
void setBrightness(uint8_t level);
uint8_t brightness();
}  // namespace Matrix
