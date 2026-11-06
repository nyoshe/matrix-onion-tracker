// Demo screen: a small list of mock .onion services with an up/down dot
// each. Stands in for real uptime checks until polling is wired up.

#include "../core/Color.h"
#include "../core/Screen.h"

class OnionStatus : public Screen {
 public:
  const char* name() const override { return "Onion Status"; }

  void draw(Display& display) override {
    display.setTextSize(1);
    display.setTextWrap(false);

    for (int i = 0; i < kCount; i++) {
      int top = 2 + i * 8;
      display.setTextColor(0xFFFF);
      display.setCursor(2, top + FONT_ASCENT);
      display.print(kMockSites[i].label);

      uint16_t dot = kMockSites[i].up ? levelColor(0.0f) : levelColor(1.0f);
      display.fillCircle(PANEL_WIDTH - 6, top + 2, 2, dot);
    }
  }

 private:
  struct Site {
    const char* label;
    bool up;
  };

  static constexpr Site kMockSites[] = {
      {"relay-1", true},
      {"relay-2", true},
      {"market-x", false},
  };
  static constexpr int kCount = sizeof(kMockSites) / sizeof(kMockSites[0]);
};

REGISTER_SCREEN(OnionStatus, 20);
