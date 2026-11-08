#pragma once

#include <stddef.h>

#include "../config.h"
#include "Matrix.h"

// Base class for every screen. To add one, subclass it in a new
// src/screens/*.cpp file ending with REGISTER_SCREEN(YourClass, order).
class Screen {
 public:
  virtual ~Screen() = default;

  // Shown on the serial console when the screen is selected.
  virtual const char* name() const = 0;

  // False hides this screen from Screens::count()/get() (and so from Nav,
  // the menu and the serial picker too), e.g. an unused coin-chart slot.
  virtual bool enabled() const { return true; }

  // Called each time this screen becomes active; clear any history here.
  virtual void reset() {}

  // Draw one frame; screen is pre-cleared, flipped after. Draw everything
  // each call -- keep persistence (scroll position, history) in members.
  virtual void draw(Display& display) = 0;
};

namespace Screens {
// Registered screens, sorted by their `order`.
size_t count();
Screen* get(size_t index);
void add(Screen* screen, int order);
}  // namespace Screens

struct ScreenRegistrar {
  ScreenRegistrar(Screen* screen, int order) { Screens::add(screen, order); }
};

// Creates a single static instance of `Type` and adds it to the rotation.
// Lower `order` values come first.
#define REGISTER_SCREEN(Type, order) \
  static Type Type##Instance;        \
  static ScreenRegistrar Type##Registrar(&Type##Instance, order)
