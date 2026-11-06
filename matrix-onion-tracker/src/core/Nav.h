#pragma once

// Which screen is active. Shared by the sketch (BOOT button, serial) and the
// on-screen menu, so both move the same state.
namespace Nav {
// Selects screen 0 and reports it.
void begin();

// Wraps `index` into range (handles negative and multi-step deltas
// correctly), resets the newly active screen, reports it.
void selectScreen(int index);

int currentScreen();

// Prints every registered screen, '*' marking the active one.
void listScreens();
}  // namespace Nav
