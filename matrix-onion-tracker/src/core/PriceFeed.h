#pragma once

// Polls CoinGecko's keyless API for BTC/XMR price + OHLC history on a
// background task (core 0), so the blocking HTTPS calls never stall rendering.
namespace PriceFeed {
void begin();  // call once from setup()

struct Price {
  bool valid = false;  // false until the first successful fetch
  double usd = 0;
  double change24h = 0;  // percent
};

Price btc();
Price monero();

constexpr int kCandleCount = 16;  // fixed display width, whatever the raw candle count was

struct Candle {
  float open = 0, high = 0, low = 0, close = 0;
};

struct History {
  bool valid = false;
  Candle candles[kCandleCount];
};

History btcHistory();
History moneroHistory();
}  // namespace PriceFeed
