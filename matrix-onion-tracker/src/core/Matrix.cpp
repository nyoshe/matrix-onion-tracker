#include "Matrix.h"

#include <Fonts/TomThumb.h>

#include "../config.h"

namespace {
Display* activeDisplay = nullptr;
uint8_t currentBrightness = PANEL_BRIGHTNESS;
}  // namespace

Display& Matrix::begin() {
  HUB75_I2S_CFG::i2s_pins pins = {Pins::R1, Pins::G1, Pins::B1, Pins::R2, Pins::G2, Pins::B2,
                                  Pins::A,  Pins::B,  Pins::C,  Pins::D,  Pins::E,
                                  Pins::LAT, Pins::OE, Pins::CLK};
  HUB75_I2S_CFG cfg(PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN, pins);
  cfg.double_buff = true;  // draw off-screen, then flip: no flicker
  // Some Waveshare panels use an FM6126A driver; if yours stays dark, try:
  // cfg.driver = HUB75_I2S_CFG::FM6126A;
  // Latch data on the other clock edge; with the default phase, white pixels
  // intermittently lost R/B (showed green) when many lines switched at once.
  cfg.clkphase = false;

  Display* display = new Display(cfg);
  if (!display->begin()) {
    Serial.println("Matrix init failed (not enough DMA memory?)");
    while (true) delay(1000);
  }
  activeDisplay = display;
  display->setFont(&TomThumb);
  Matrix::setBrightness(PANEL_BRIGHTNESS);
  display->clearScreen();
  return *display;
}

void Matrix::setBrightness(uint8_t level) {
  currentBrightness = level;
  if (activeDisplay) activeDisplay->setBrightness8(level);
}

uint8_t Matrix::brightness() { return currentBrightness; }
