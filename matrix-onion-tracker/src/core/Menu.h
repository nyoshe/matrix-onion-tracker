#pragma once

#include "Matrix.h"

// On-screen menu driven by the KY-040: a press opens it, turning moves the
// highlight (or edits the highlighted item's value), and a press picks the
// highlighted item (or confirms the value and returns to the list).
namespace Menu {
// Feeds this frame's encoder input. Returns true while the menu should be
// drawn instead of the active screen.
bool update(int steps, bool click);

// Draws the menu; call instead of the active screen's draw() while open.
void draw(Display& display);
}  // namespace Menu
