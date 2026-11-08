#pragma once

#include "../config.h"

// Persisted list of coins to track, managed from the web config portal.
// PriceFeed reads this cross-core (core 0), so it uses snapshot() rather
// than the plain accessors below, which assume same-core (portal-side) use.
namespace CoinStore {
void begin();  // call once from setup()

constexpr int kIdLen = 24;     // CoinGecko ids ("shiba-inu", "usd-coin", ...)
constexpr int kSymbolLen = 6;  // display symbol, e.g. "SHIB"

struct Entry {
  char id[kIdLen] = "";
  char symbol[kSymbolLen] = "";
};

struct Snapshot {
  int count = 0;
  Entry entries[MAX_TRACKED_COINS];
};

Snapshot snapshot();  // consistent copy of the full list; safe from any core

int count();
const char* id(int i);
const char* symbol(int i);

// Adds a coin, or updates its symbol if the id is already saved. id must
// match [a-z0-9-]+ (it goes straight into a URL). Returns its index, or -1
// if id is invalid or the list is full (MAX_TRACKED_COINS).
int add(const char* id, const char* symbol);

bool remove(int i);
}  // namespace CoinStore
