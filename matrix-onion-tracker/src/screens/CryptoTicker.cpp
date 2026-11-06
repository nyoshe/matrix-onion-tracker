// Demo screen: scrolls mock ticker symbols and prices across the panel,
// green/red for up/down. Stands in for the real price feed until polling is
// wired up.

#include "../core/Color.h"
#include "../core/Screen.h"

class CryptoTicker : public Screen {
 public:
  const char* name() const override { return "Crypto Ticker"; }

  void reset() override { scrollX_ = PANEL_WIDTH; }

  void draw(Display& display) override {
    display.setTextSize(1);
    display.setTextWrap(false);
    display.setCursor(scrollX_, PANEL_HEIGHT / 2 - 4);

    for (const Entry& e : kMockEntries) {
      display.setTextColor(0xFFFF);
      display.print(e.symbol);
      display.print(' ');
      display.setTextColor(e.up ? levelColor(0.0f) : levelColor(1.0f));
      display.print(e.price);
      display.print("   ");
    }

    if (--scrollX_ < -kLineWidthPx) scrollX_ = PANEL_WIDTH;
  }

 private:
  struct Entry {
    const char* symbol;
    const char* price;
    bool up;
  };

  static constexpr Entry kMockEntries[] = {
      {"BTC", "68,420", true},
      {"ETH", "3,512", false},
      {"SOL", "148.90", true},
  };
  static constexpr int kLineWidthPx = 220;  // rough width of kMockEntries at text size 1

  int scrollX_ = PANEL_WIDTH;
};

REGISTER_SCREEN(CryptoTicker, 10);
