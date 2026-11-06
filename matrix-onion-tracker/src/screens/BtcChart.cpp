#include "CandlestickScreen.h"

class BtcChart : public CandlestickScreen {
 public:
  BtcChart() : CandlestickScreen("BTC Chart", "BTC", PriceFeed::btc, PriceFeed::btcHistory) {}
};

REGISTER_SCREEN(BtcChart, 10);
