#pragma once

#include "Matrix.h"

// Drawing for the menu's "Network" item: a QR code for the config portal's
// current URL, the URL as text, and whichever network reaches it. Lives
// here rather than as a Screen since it's reached from the menu (so it
// still works in AP mode, before any screen is dispatched -- see loop()'s
// !Net::isConnected() gate), not from the screen rotation.
namespace NetworkView {
void reset();  // call when the menu enters this item, resets scroll position
void draw(Display& display);
}  // namespace NetworkView
