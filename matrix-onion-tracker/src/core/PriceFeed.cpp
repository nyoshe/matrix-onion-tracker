#include "PriceFeed.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

#include "../config.h"
#include "Net.h"

namespace {

SemaphoreHandle_t mutex;
PriceFeed::Price btcPrice;
PriceFeed::Price xmrPrice;
PriceFeed::History btcHist;
PriceFeed::History xmrHist;

void applyPrice(PriceFeed::Price& slot, double usd, double change24h) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  slot = {true, usd, change24h};
  xSemaphoreGive(mutex);
}

void applyHistory(PriceFeed::History& slot, const PriceFeed::Candle* candles) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  slot.valid = true;
  memcpy(slot.candles, candles, sizeof(slot.candles));
  xSemaphoreGive(mutex);
}

// Skips cert validation: this is public, read-only data, and pinning a root
// CA would need upkeep as CoinGecko's cert rotates.
bool httpGetJson(const String& url, JsonDocument& doc) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.printf("PriceFeed: begin() failed for %s\n", url.c_str());
    return false;
  }

  int code = http.GET();
  if (code != 200) {
    Serial.printf("PriceFeed: HTTP %d for %s\n", code, url.c_str());
    http.end();
    return false;
  }

  // getString(), not getStream(): HTTPClient de-chunks the body here, where
  // a raw stream read can hand deserializeJson() chunk-size markers mixed
  // into the JSON (CoinGecko chunks its responses).
  String body = http.getString();
  http.end();

  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("PriceFeed: JSON parse failed: %s\n", err.c_str());
    return false;
  }
  return true;
}

bool fetchPrices() {
  JsonDocument doc;
  if (!httpGetJson(PRICE_API_URL, doc)) return false;

  double btcUsd = doc["bitcoin"]["usd"] | 0.0;
  double btcChange = doc["bitcoin"]["usd_24h_change"] | 0.0;
  double xmrUsd = doc["monero"]["usd"] | 0.0;
  double xmrChange = doc["monero"]["usd_24h_change"] | 0.0;
  if (btcUsd <= 0 || xmrUsd <= 0) {
    Serial.println("PriceFeed: missing price in response");
    return false;
  }

  applyPrice(btcPrice, btcUsd, btcChange);
  applyPrice(xmrPrice, xmrUsd, xmrChange);
  Serial.printf("PriceFeed: BTC $%.2f (%+.1f%%), XMR $%.2f (%+.1f%%)\n", btcUsd, btcChange, xmrUsd,
                xmrChange);
  return true;
}

// Buckets however many raw [timestamp,open,high,low,close] candles came
// back into a fixed kCandleCount, so chart layout never changes.
void aggregate(JsonArray raw, PriceFeed::Candle* out) {
  int n = raw.size();
  for (int b = 0; b < PriceFeed::kCandleCount; b++) {
    int start = (int)((int64_t)b * n / PriceFeed::kCandleCount);
    int end = (int)((int64_t)(b + 1) * n / PriceFeed::kCandleCount);
    if (end <= start) end = start + 1;
    if (end > n) end = n;

    float open = raw[start][1].as<float>();
    float close = raw[end - 1][4].as<float>();
    float high = raw[start][2].as<float>();
    float low = raw[start][3].as<float>();
    for (int i = start; i < end; i++) {
      high = max(high, raw[i][2].as<float>());
      low = min(low, raw[i][3].as<float>());
    }
    out[b] = {open, high, low, close};
  }
}

bool fetchHistory(const char* coinId, PriceFeed::History& slot) {
  String url = String("https://api.coingecko.com/api/v3/coins/") + coinId +
               "/ohlc?vs_currency=usd&days=1";
  JsonDocument doc;
  if (!httpGetJson(url, doc)) return false;

  JsonArray raw = doc.as<JsonArray>();
  int n = raw.size();
  if (n == 0) {
    Serial.printf("PriceFeed: no OHLC data for %s\n", coinId);
    return false;
  }

  PriceFeed::Candle candles[PriceFeed::kCandleCount];
  aggregate(raw, candles);
  applyHistory(slot, candles);

  float lo = candles[0].low, hi = candles[0].high;
  for (int i = 1; i < PriceFeed::kCandleCount; i++) {
    lo = min(lo, candles[i].low);
    hi = max(hi, candles[i].high);
  }
  Serial.printf("PriceFeed: %s OHLC %d raw candles, lo %.2f hi %.2f\n", coinId, n, lo, hi);
  return true;
}

void taskFn(void*) {
  uint32_t nextPriceMs = 0;
  uint32_t nextHistoryMs = 0;
  for (;;) {
    if (Net::isConnected()) {
      uint32_t now = millis();
      if (now >= nextPriceMs) {
        bool ok = fetchPrices();
        nextPriceMs = now + (ok ? PRICE_POLL_INTERVAL_MS : PRICE_RETRY_DELAY_MS);
      }
      if (now >= nextHistoryMs) {
        bool ok = fetchHistory("bitcoin", btcHist);
        ok = fetchHistory("monero", xmrHist) && ok;
        nextHistoryMs = now + (ok ? PRICE_HISTORY_POLL_INTERVAL_MS : PRICE_RETRY_DELAY_MS);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // check often so a fresh connection fetches right away
  }
}

}  // namespace

void PriceFeed::begin() {
  mutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(taskFn, "PriceFeed", 12288, nullptr, 1, nullptr, 0);
}

PriceFeed::Price PriceFeed::btc() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  Price p = btcPrice;
  xSemaphoreGive(mutex);
  return p;
}

PriceFeed::Price PriceFeed::monero() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  Price p = xmrPrice;
  xSemaphoreGive(mutex);
  return p;
}

PriceFeed::History PriceFeed::btcHistory() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  History h = btcHist;
  xSemaphoreGive(mutex);
  return h;
}

PriceFeed::History PriceFeed::moneroHistory() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  History h = xmrHist;
  xSemaphoreGive(mutex);
  return h;
}
