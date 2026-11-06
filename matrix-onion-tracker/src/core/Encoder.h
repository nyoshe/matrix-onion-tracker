#pragma once

// KY-040 rotary encoder: quadrature pins decoded via interrupt, button
// polled once per frame.
namespace Encoder {
void begin();

// Detents turned since the last call (positive = clockwise), then resets to 0.
int takeSteps();

// Must be called exactly once per frame, before takeClick().
void pollButton();

// Fires once per press.
bool takeClick();
}  // namespace Encoder
