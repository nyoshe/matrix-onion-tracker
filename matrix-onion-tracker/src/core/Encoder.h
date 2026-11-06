#pragma once

// KY-040 rotary encoder: quadrature pins decoded via interrupt, button
// debounced by polling.
namespace Encoder {
void begin();

// Detents turned since the last call (positive = clockwise), then resets to 0.
int takeSteps();

// True once per completed button press since the last call.
bool takeClick();
}  // namespace Encoder
