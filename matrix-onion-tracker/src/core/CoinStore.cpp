#include "CoinStore.h"

#include <Arduino.h>
#include <Preferences.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {

Preferences prefs;
SemaphoreHandle_t mutex;
CoinStore::Entry entries[MAX_TRACKED_COINS];
int entryCount = 0;

void copyTruncated(char* dst, size_t dstLen, const char* src) {
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

// CoinGecko ids go straight into a URL, so keep them to the character set
// those ids actually use.
bool validId(const char* id) {
  if (!id || !id[0]) return false;
  for (const char* p = id; *p; p++) {
    if (!(islower(*p) || isdigit(*p) || *p == '-')) return false;
  }
  return true;
}

void uppercaseTruncated(char* dst, size_t dstLen, const char* src) {
  size_t i = 0;
  for (; src[i] && i < dstLen - 1; i++) dst[i] = toupper(src[i]);
  dst[i] = '\0';
}

void save() {
  prefs.putInt("count", entryCount);
  for (int i = 0; i < entryCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "i%d", i);
    prefs.putString(key, entries[i].id);
    snprintf(key, sizeof(key), "s%d", i);
    prefs.putString(key, entries[i].symbol);
  }
}

}  // namespace

void CoinStore::begin() {
  mutex = xSemaphoreCreateMutex();
  prefs.begin("coinstore", false);

  // Seed BTC/XMR only on a fresh board (no "count" key yet) -- otherwise
  // deleting every tracked coin would just bring them back on next boot.
  if (!prefs.isKey("count")) {
    entryCount = 2;
    copyTruncated(entries[0].id, sizeof(entries[0].id), "bitcoin");
    copyTruncated(entries[0].symbol, sizeof(entries[0].symbol), "BTC");
    copyTruncated(entries[1].id, sizeof(entries[1].id), "monero");
    copyTruncated(entries[1].symbol, sizeof(entries[1].symbol), "XMR");
    save();
    return;
  }

  entryCount = prefs.getInt("count", 0);
  if (entryCount < 0) entryCount = 0;
  if (entryCount > MAX_TRACKED_COINS) entryCount = MAX_TRACKED_COINS;

  for (int i = 0; i < entryCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "i%d", i);
    copyTruncated(entries[i].id, sizeof(entries[i].id), prefs.getString(key, "").c_str());
    snprintf(key, sizeof(key), "s%d", i);
    copyTruncated(entries[i].symbol, sizeof(entries[i].symbol), prefs.getString(key, "").c_str());
  }
}

CoinStore::Snapshot CoinStore::snapshot() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  Snapshot s;
  s.count = entryCount;
  memcpy(s.entries, entries, sizeof(entries));
  xSemaphoreGive(mutex);
  return s;
}

int CoinStore::count() { return entryCount; }

const char* CoinStore::id(int i) { return (i >= 0 && i < entryCount) ? entries[i].id : ""; }

const char* CoinStore::symbol(int i) { return (i >= 0 && i < entryCount) ? entries[i].symbol : ""; }

int CoinStore::add(const char* newId, const char* newSymbol) {
  if (!validId(newId)) return -1;

  xSemaphoreTake(mutex, portMAX_DELAY);
  int slot = -1;
  for (int i = 0; i < entryCount; i++) {
    if (strcmp(entries[i].id, newId) == 0) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    if (entryCount >= MAX_TRACKED_COINS) {
      xSemaphoreGive(mutex);
      return -1;
    }
    slot = entryCount++;
    copyTruncated(entries[slot].id, sizeof(entries[slot].id), newId);
  }
  // Fall back to the id itself so a symbol always shows, even if left blank.
  uppercaseTruncated(entries[slot].symbol, sizeof(entries[slot].symbol),
                      (newSymbol && newSymbol[0]) ? newSymbol : newId);
  save();
  xSemaphoreGive(mutex);
  return slot;
}

bool CoinStore::remove(int i) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  if (i < 0 || i >= entryCount) {
    xSemaphoreGive(mutex);
    return false;
  }
  for (int k = i; k < entryCount - 1; k++) entries[k] = entries[k + 1];
  entryCount--;
  save();
  xSemaphoreGive(mutex);
  return true;
}
