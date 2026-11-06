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

// Fixed display width, whatever the raw candle count was. 64 matches the
// panel's 64px width at 1px/candle; CoinGecko's keyless tier only ever
// returns ~48-50 raw 30min candles for a day, so the thinnest ~16 of these
// repeat their nearest raw candle rather than showing unique data.
constexpr int kCandleCount = 64;

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
