#include "Screen.h"

namespace {

struct Entry {
  Screen* screen;
  int order;
};

// Plain zero-initialised arrays rather than a container: screens register
// from static constructors in other files, and zero-initialisation is
// guaranteed to have happened before any of those run.
constexpr size_t MAX_SCREENS = 32;
Entry entries[MAX_SCREENS];
size_t entryCount;

}  // namespace

size_t Screens::count() { return entryCount; }

Screen* Screens::get(size_t index) {
  return index < entryCount ? entries[index].screen : nullptr;
}

void Screens::add(Screen* screen, int order) {
  if (entryCount == MAX_SCREENS) return;
  // Insertion sort, so the list is ordered regardless of link order.
  size_t i = entryCount++;
  while (i > 0 && entries[i - 1].order > order) {
    entries[i] = entries[i - 1];
    i--;
  }
  entries[i] = {screen, order};
}
