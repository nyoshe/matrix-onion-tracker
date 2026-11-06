#pragma once

#include "../core/PriceFeed.h"
#include "../core/Screen.h"

// Shared candlestick-chart draw logic; BtcChart/XmrChart subclass with a
// default ctor since REGISTER_SCREEN needs one.
class CandlestickScreen : public Screen {
 public:
  const char* name() const override { return name_; }
  void reset() override { scrollX_ = 0; }
  void draw(Display& display) override;

 protected:
  CandlestickScreen(const char* name, const char* symbol, PriceFeed::Price (*price)(),
                     PriceFeed::History (*history)())
      : name_(name), symbol_(symbol), price_(price), history_(history) {}

 private:
  const char* name_;
  const char* symbol_;
  PriceFeed::Price (*price_)();
  PriceFeed::History (*history_)();
  int scrollX_ = 0;  // only moves once the header line is too wide to fit
};
