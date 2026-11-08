#include "PriceFeed.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

#include "../config.h"
#include "CoinStore.h"
#include "Net.h"

namespace {

struct Entry {
  char id[CoinStore::kIdLen] = "";
  PriceFeed::Price price;
  PriceFeed::History history;
  bool known = true;
  uint32_t nextHistoryMs = 0;
};

// nextHistoryMs is core0-only (taskFn); everything else crosses to core1
// through the accessors below, guarded by mutex.
SemaphoreHandle_t mutex;
Entry entries[MAX_TRACKED_COINS];
int entryCount = 0;

// Pulls CoinStore's current list into `entries`, keeping each coin's
// existing price/history by id rather than slot position, so a mid-list
// delete doesn't relabel another coin's data until that coin's own next
// fetch. New ids start due for an immediate history fetch.
void syncEntries() {
  CoinStore::Snapshot snap = CoinStore::snapshot();
  static Entry fresh[MAX_TRACKED_COINS];  // off taskFn's stack; only this fn touches it
  for (int i = 0; i < snap.count; i++) {
    int old = -1;
    for (int j = 0; j < entryCount; j++) {
      if (strcmp(entries[j].id, snap.entries[i].id) == 0) {
        old = j;
        break;
      }
    }
    if (old >= 0) {
      fresh[i] = entries[old];
    } else {
      fresh[i] = Entry{};
      strncpy(fresh[i].id, snap.entries[i].id, sizeof(fresh[i].id) - 1);
    }
  }

  xSemaphoreTake(mutex, portMAX_DELAY);
  entryCount = snap.count;
  memcpy(entries, fresh, sizeof(fresh));
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

// One combined call for every tracked coin's price. Each coin's result is
// applied independently, so one bad id (e.g. a typo) doesn't stop the rest
// from updating, and the whole call only counts as failed (for retry
// purposes) if the request itself failed.
bool fetchPrices() {
  int n = entryCount;
  if (n == 0) return true;

  String ids;
  for (int i = 0; i < n; i++) {
    if (i) ids += ",";
    ids += entries[i].id;
  }
  String url = String(PRICE_API_BASE) + "?ids=" + ids + "&vs_currencies=usd&include_24hr_change=true";

  JsonDocument doc;
  if (!httpGetJson(url, doc)) return false;

  for (int i = 0; i < n; i++) {
    JsonVariant coin = doc[entries[i].id];
    double usd = coin["usd"] | 0.0;
    bool ok = usd > 0;

    xSemaphoreTake(mutex, portMAX_DELAY);
    entries[i].known = ok;
    if (ok) entries[i].price = {true, usd, coin["usd_24h_change"] | 0.0};
    xSemaphoreGive(mutex);
  }
  Serial.printf("PriceFeed: priced %d coin(s)\n", n);
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

bool fetchHistory(int i) {
  String url = String("https://api.coingecko.com/api/v3/coins/") + entries[i].id +
               "/ohlc?vs_currency=usd&days=1";
  JsonDocument doc;
  if (!httpGetJson(url, doc)) return false;

  JsonArray raw = doc.as<JsonArray>();
  int n = raw.size();
  if (n == 0) {
    Serial.printf("PriceFeed: no OHLC data for %s\n", entries[i].id);
    return false;
  }

  PriceFeed::Candle candles[PriceFeed::kCandleCount];
  aggregate(raw, candles);

  xSemaphoreTake(mutex, portMAX_DELAY);
  entries[i].history.valid = true;
  memcpy(entries[i].history.candles, candles, sizeof(candles));
  xSemaphoreGive(mutex);

  Serial.printf("PriceFeed: %s OHLC %d raw candles\n", entries[i].id, n);
  return true;
}

void taskFn(void*) {
  uint32_t nextPriceMs = 0;
  uint32_t nextCallMs = 0;  // serializes price + history calls under the rate limit
  int historyTurn = 0;      // round-robins which coin's history is due next
  for (;;) {
    syncEntries();
    uint32_t now = millis();

    if (Net::isConnected() && now >= nextCallMs) {
      if (now >= nextPriceMs) {
        bool ok = fetchPrices();
        nextPriceMs = now + (ok ? PRICE_POLL_INTERVAL_MS : PRICE_RETRY_DELAY_MS);
        nextCallMs = now + PRICE_MIN_CALL_GAP_MS;
      } else if (entryCount > 0) {
        historyTurn %= entryCount;
        int i = historyTurn;
        if (now >= entries[i].nextHistoryMs) {
          bool ok = fetchHistory(i);
          entries[i].nextHistoryMs = now + (ok ? PRICE_HISTORY_POLL_INTERVAL_MS : PRICE_RETRY_DELAY_MS);
          nextCallMs = now + PRICE_MIN_CALL_GAP_MS;
        }
        historyTurn++;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // check often so a fresh connection/new coin fetches right away
  }
}

}  // namespace

void PriceFeed::begin() {
  mutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(taskFn, "PriceFeed", 12288, nullptr, 1, nullptr, 0);
}

PriceFeed::Price PriceFeed::price(const char* id) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  Price p;
  for (int i = 0; i < entryCount; i++) {
    if (strcmp(entries[i].id, id) == 0) {
      p = entries[i].price;
      break;
    }
  }
  xSemaphoreGive(mutex);
  return p;
}

bool PriceFeed::known(const char* id) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  bool k = true;
  for (int i = 0; i < entryCount; i++) {
    if (strcmp(entries[i].id, id) == 0) {
      k = entries[i].known;
      break;
    }
  }
  xSemaphoreGive(mutex);
  return k;
}

PriceFeed::History PriceFeed::history(const char* id) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  History h;
  for (int i = 0; i < entryCount; i++) {
    if (strcmp(entries[i].id, id) == 0) {
      h = entries[i].history;
      break;
    }
  }
  xSemaphoreGive(mutex);
  return h;
}
