#pragma once

// KY-040 rotary encoder: quadrature pins decoded via interrupt, button
// polled once per frame.
namespace Encoder {
void begin();

// Detents turned since the last call (positive = clockwise), then resets to 0.
int takeSteps();

// Must be called exactly once per frame, before takeClick()/takeLongClick().
void pollButton();

// Fires once, on release, for a press shorter than ENCODER_LONG_PRESS_MS.
bool takeClick();

// Fires once, while still held, when the press reaches ENCODER_LONG_PRESS_MS
// (so it doesn't wait for release, and the same press won't also fire a
// short click).
bool takeLongClick();
}  // namespace Encoder
