#pragma once

// Colour helpers shared by screens. All return RGB565.

#include <math.h>
#include <stdint.h>

inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

inline float clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }
inline int clampInt(int n, int lo, int hi) { return n < lo ? lo : (n > hi ? hi : n); }

// Green -> yellow -> red, e.g. for up/down or status colouring.
inline uint16_t levelColor(float t) {
  t = clamp01(t);
  uint8_t r = t < 0.5f ? (uint8_t)(t * 2 * 255) : 255;
  uint8_t g = t < 0.5f ? 255 : (uint8_t)((1 - t) * 2 * 255);
  return rgb565(r, g, 0);
}

// HSV with every component in 0..1. Hue wraps, so any value works.
inline uint16_t hsvColor(float h, float s, float v) {
  h = (h - (int)h) * 6;
  if (h < 0) h += 6;
  float f = h - (int)h;
  s = clamp01(s);
  float val = clamp01(v) * 255;
  uint8_t V = (uint8_t)val;
  uint8_t p = (uint8_t)(val * (1 - s));
  uint8_t q = (uint8_t)(val * (1 - s * f));
  uint8_t t = (uint8_t)(val * (1 - s * (1 - f)));
  switch ((int)h) {
    case 0: return rgb565(V, t, p);
    case 1: return rgb565(q, V, p);
    case 2: return rgb565(p, V, t);
    case 3: return rgb565(p, q, V);
    case 4: return rgb565(t, p, V);
    default: return rgb565(V, p, q);
  }
}

// Hue in 0..1 around the colour wheel, full saturation.
inline uint16_t hueColor(float h, float brightness = 1.0f) {
  return hsvColor(h, 1.0f, brightness);
}
