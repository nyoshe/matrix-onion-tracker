#include "Menu.h"

#include <stdio.h>

#include "../config.h"
#include "Color.h"
#include "Nav.h"
#include "NetworkView.h"
#include "Screen.h"

namespace {

enum class Kind { kScreen, kBrightness, kNetwork, kBack };
struct Item {
  const char* label;
  Kind kind;
};
constexpr Item kItems[] = {
    {"Screen", Kind::kScreen},
    {"Brightness", Kind::kBrightness},
    {"Network", Kind::kNetwork},
    {"Back", Kind::kBack},
};
constexpr int kItemCount = sizeof(kItems) / sizeof(kItems[0]);
constexpr int kRowHeight = 8;  // 4 items * 10 would run past the panel's 32px height

enum class State { kClosed, kBrowsing, kEditing };
State state = State::kClosed;
int selected = 0;

// Proper modulo: result always in [0, n), even for negative `value`
// (`value % n` alone keeps the sign of `value` in C++, which would index
// kItems with a negative subscript).
int wrapIndex(int value, int n) { return ((value % n) + n) % n; }

void applyStep(Kind kind, int steps) {
  switch (kind) {
    case Kind::kScreen:
      Nav::selectScreen(Nav::currentScreen() + steps);
      break;
    case Kind::kBrightness:
      Matrix::setBrightness(
          (uint8_t)clampInt((int)Matrix::brightness() + steps * MENU_BRIGHTNESS_STEP, 0, 255));
      break;
    case Kind::kNetwork:  // a view, not an editable value; turning does nothing
      break;
    case Kind::kBack:
      break;
  }
}

void valueText(Kind kind, char* out, size_t outLen) {
  switch (kind) {
    case Kind::kScreen:
      snprintf(out, outLen, "%s", Screens::get(Nav::currentScreen())->name());
      break;
    case Kind::kBrightness:
      snprintf(out, outLen, "%u", Matrix::brightness());
      break;
    case Kind::kNetwork:  // drawn separately by NetworkView::draw(), not via this
    case Kind::kBack:
      out[0] = '\0';
      break;
  }
}

}  // namespace

bool Menu::update(int steps, bool click) {
  switch (state) {
    case State::kClosed:
      if (steps) Nav::selectScreen(Nav::currentScreen() + steps);
      if (click) {
        state = State::kBrowsing;
        selected = 0;
      }
      break;

    case State::kBrowsing:
      if (steps) selected = wrapIndex(selected + steps, kItemCount);
      if (click) {
        Kind kind = kItems[selected].kind;
        if (kind == Kind::kBack) {
          state = State::kClosed;
        } else {
          state = State::kEditing;
          if (kind == Kind::kNetwork) NetworkView::reset();
        }
      }
      break;

    case State::kEditing:
      if (steps) applyStep(kItems[selected].kind, steps);
      if (click) state = State::kBrowsing;
      break;
  }
  return state != State::kClosed;
}

void Menu::draw(Display& display) {
  display.setTextSize(1);
  display.setTextWrap(false);

  if (state == State::kEditing) {
    if (kItems[selected].kind == Kind::kNetwork) {
      NetworkView::draw(display);
      return;
    }
    char value[24];
    valueText(kItems[selected].kind, value, sizeof(value));
    display.setTextColor(0xFFFF);
    display.setCursor(2, 4 + FONT_ASCENT);
    display.print(kItems[selected].label);
    display.setTextColor(hueColor(0.5f));  // cyan: editing this value live
    display.setCursor(2, 18 + FONT_ASCENT);
    display.print(value);
    return;
  }

  for (int i = 0; i < kItemCount; i++) {
    bool isSelected = (i == selected);
    display.setTextColor(isSelected ? hueColor(0.12f) : 0xFFFF);  // amber highlight
    display.setCursor(1, 1 + i * kRowHeight + FONT_ASCENT);
    display.print(isSelected ? "> " : "  ");
    display.print(kItems[i].label);
  }
}
