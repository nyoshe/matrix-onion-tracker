#pragma once

#include <stdio.h>

#include "../core/CoinStore.h"
#include "../core/Screen.h"

// One instance per CoinStore slot (src/screens/CoinChart.cpp); looks up its
// coin by slot index each frame, since CoinStore's list can change under it.
class CandlestickScreen : public Screen {
 public:
  explicit CandlestickScreen(int slot) : slot_(slot) {}

  const char* name() const override {
    snprintf(nameBuf_, sizeof(nameBuf_), "%s Chart", CoinStore::symbol(slot_));
    return nameBuf_;
  }
  bool enabled() const override { return slot_ < CoinStore::count(); }
  void reset() override { scrollX_ = 0; }
  void draw(Display& display) override;

 private:
  int slot_;
  mutable char nameBuf_[16];
  int scrollX_ = 0;  // only moves once the header line is too wide to fit
};
