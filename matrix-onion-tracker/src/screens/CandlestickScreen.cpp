#include "CandlestickScreen.h"

#include <stdio.h>

#include "../core/Color.h"
#include "../core/PriceFeed.h"

namespace {
constexpr int kCandleWidth = PANEL_WIDTH / PriceFeed::kCandleCount;
constexpr int kHeaderHeight = 9;  // one line: symbol + price + change, chart below it
constexpr int kChartTop = kHeaderHeight;
constexpr int kChartHeight = PANEL_HEIGHT - kHeaderHeight;
}  // namespace

void CandlestickScreen::draw(Display& display) {
  if (slot_ >= CoinStore::count()) return;  // coin removed this frame; next frame hides us
  const char* id = CoinStore::id(slot_);
  const char* symbol = CoinStore::symbol(slot_);

  PriceFeed::History hist = PriceFeed::history(id);
  if (hist.valid) {
    float lo = hist.candles[0].low;
    float hi = hist.candles[0].high;
    for (const auto& c : hist.candles) {
      lo = min(lo, c.low);
      hi = max(hi, c.high);
    }
    // Relative floor, not an absolute one: a sub-cent coin's whole daily
    // range can be under 0.0001 in absolute terms without being flat.
    float minRange = (hi > 0 ? hi : 1.0f) * 1e-6f;
    float range = (hi - lo) > minRange ? (hi - lo) : minRange;

    for (int i = 0; i < PriceFeed::kCandleCount; i++) {
      const PriceFeed::Candle& c = hist.candles[i];
      int x = i * kCandleWidth;  // zero gap: each candle fills its whole slot
      auto toY = [&](float v) {
        return kChartTop + (kChartHeight - 1) - (int)((v - lo) / range * (kChartHeight - 1));
      };
      int yHigh = toY(c.high), yLow = toY(c.low), yOpen = toY(c.open), yClose = toY(c.close);
      uint16_t color = c.close >= c.open ? levelColor(0.0f) : levelColor(1.0f);

      // Shadow (high-low) at quarter brightness, drawn first so the body
      // (open-close), at full brightness, overwrites it where they overlap.
      display.fillRect(x, min(yHigh, yLow), kCandleWidth, abs(yLow - yHigh) + 1, dim565(color, 0.25f));

      int bodyTop = min(yOpen, yClose);
      int bodyHeight = max(abs(yClose - yOpen), 1);
      display.fillRect(x, bodyTop, kCandleWidth, bodyHeight, color);
    }
  }

  display.setTextSize(1);
  display.setTextWrap(false);

  PriceFeed::Price price = PriceFeed::price(id);
  char priceBuf[16] = "...";
  char changeBuf[12] = "";
  if (price.valid) {
    // Fixed "$%.0f" reads as $0 for sub-dollar coins (DOGE, SHIB, ...); scale
    // decimals to the price itself instead.
    double v = price.usd;
    const char* fmt = v >= 1000 ? "$%.0f" : v >= 1 ? "$%.2f" : v >= 0.0001 ? "$%.6f" : "$%.8f";
    snprintf(priceBuf, sizeof(priceBuf), fmt, v);
    snprintf(changeBuf, sizeof(changeBuf), " %+.1f%%", price.change24h);
  }

  char line[48];
  snprintf(line, sizeof(line), "%s %s%s", symbol, priceBuf, changeBuf);
  int16_t boundsX, boundsY;
  uint16_t lineWidth, lineHeight;
  display.getTextBounds(line, 0, 0, &boundsX, &boundsY, &lineWidth, &lineHeight);

  int x;
  if ((int)lineWidth <= PANEL_WIDTH) {
    x = 1;  // fits: no need to scroll
  } else {
    x = scrollX_;
    scrollX_ = (scrollX_ <= -(int)lineWidth) ? PANEL_WIDTH : scrollX_ - 1;
  }

  display.setCursor(x, FONT_ASCENT);
  display.setTextColor(0xFFFF);
  display.print(symbol);
  display.print(' ');

  display.setTextColor(0xFFE0);  // yellow
  display.print(priceBuf);
  if (price.valid) {
    display.setTextColor(price.change24h >= 0 ? levelColor(0.0f) : levelColor(1.0f));
    display.print(changeBuf);
  }
}
