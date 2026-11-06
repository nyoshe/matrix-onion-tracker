#pragma once

#include "Matrix.h"

// WiFi setup flow: scan -> pick a network -> (enter password) -> connect ->
// show result -> back to the list. Delegated into from Menu's "WiFi" item.
namespace WifiFlow {
void begin();  // starts a scan; call when entering this flow from the menu

// Feeds this frame's encoder input. Returns true while this flow should be
// drawn instead of the menu's item list; false once "Back" is chosen.
bool update(int steps, bool click, bool longClick);

void draw(Display& display);
}  // namespace WifiFlow
