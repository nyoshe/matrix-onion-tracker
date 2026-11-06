#include "CandlestickScreen.h"

#include <stdio.h>

#include "../core/Color.h"

namespace {
constexpr int kCandleWidth = PANEL_WIDTH / PriceFeed::kCandleCount;
constexpr int kHeaderHeight = 9;  // one line: symbol + price + change, chart below it
constexpr int kChartTop = kHeaderHeight;
constexpr int kChartHeight = PANEL_HEIGHT - kHeaderHeight;
}  // namespace

void CandlestickScreen::draw(Display& display) {
  PriceFeed::History hist = history_();
  if (hist.valid) {
    float lo = hist.candles[0].low;
    float hi = hist.candles[0].high;
    for (const auto& c : hist.candles) {
      lo = min(lo, c.low);
      hi = max(hi, c.high);
    }
    float range = (hi - lo) > 0.0001f ? (hi - lo) : 1.0f;

    for (int i = 0; i < PriceFeed::kCandleCount; i++) {
      const PriceFeed::Candle& c = hist.candles[i];
      int x = i * kCandleWidth + kCandleWidth / 2;
      auto toY = [&](float v) {
        return kChartTop + (kChartHeight - 1) - (int)((v - lo) / range * (kChartHeight - 1));
      };
      int yHigh = toY(c.high), yLow = toY(c.low), yOpen = toY(c.open), yClose = toY(c.close);
      uint16_t color = c.close >= c.open ? levelColor(0.0f) : levelColor(1.0f);

      display.drawFastVLine(x, min(yHigh, yLow), abs(yLow - yHigh) + 1, color);

      int bodyTop = min(yOpen, yClose);
      int bodyHeight = max(abs(yClose - yOpen), 1);
      int bodyWidth = max(kCandleWidth - 2, 1);
      display.fillRect(x - bodyWidth / 2, bodyTop, bodyWidth, bodyHeight, color);
    }
  }

  display.setTextSize(1);
  display.setTextWrap(false);

  PriceFeed::Price price = price_();
  char priceBuf[16] = "...";
  char changeBuf[12] = "";
  if (price.valid) {
    snprintf(priceBuf, sizeof(priceBuf), "$%.0f", price.usd);
    snprintf(changeBuf, sizeof(changeBuf), " %+.1f%%", price.change24h);
  }

  char line[48];
  snprintf(line, sizeof(line), "%s %s%s", symbol_, priceBuf, changeBuf);
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
  display.print(symbol_);
  display.print(' ');
  display.print(priceBuf);
  if (price.valid) {
    display.setTextColor(price.change24h >= 0 ? levelColor(0.0f) : levelColor(1.0f));
    display.print(changeBuf);
  }
}
