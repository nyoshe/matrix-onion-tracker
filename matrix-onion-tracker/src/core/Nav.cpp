#include "Nav.h"

#include "Screen.h"

namespace {
int current = 0;

// Proper modulo: result always in [0, n), even for negative `value`
// (`value % n` alone keeps the sign of `value` in C++).
int wrapIndex(int value, int n) { return ((value % n) + n) % n; }
}  // namespace

void Nav::begin() { selectScreen(0); }

void Nav::selectScreen(int index) {
  int n = (int)Screens::count();
  if (n == 0) return;
  current = wrapIndex(index, n);
  Screens::get(current)->reset();
  Serial.printf("[%d] %s\n", current, Screens::get(current)->name());
}

int Nav::currentScreen() { return current; }

void Nav::listScreens() {
  for (int i = 0; i < (int)Screens::count(); i++) {
    Serial.printf("%c %d: %s\n", i == current ? '*' : ' ', i, Screens::get(i)->name());
  }
}
