#include "CandlestickScreen.h"

class XmrChart : public CandlestickScreen {
 public:
  XmrChart() : CandlestickScreen("XMR Chart", "XMR", PriceFeed::monero, PriceFeed::moneroHistory) {}
};

REGISTER_SCREEN(XmrChart, 11);
