#include "NetworkStore.h"

#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

#include "../config.h"

namespace {

Preferences prefs;
char ssids[WIFI_MAX_SAVED_NETWORKS][33];
char passwords[WIFI_MAX_SAVED_NETWORKS][64];
int entryCount = 0;

void copyTruncated(char* dst, size_t dstLen, const char* src) {
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

void save() {
  prefs.putInt("count", entryCount);
  for (int i = 0; i < entryCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "s%d", i);
    prefs.putString(key, ssids[i]);
    snprintf(key, sizeof(key), "p%d", i);
    prefs.putString(key, passwords[i]);
  }
}

}  // namespace

void NetworkStore::begin() {
  prefs.begin("netstore", false);
  entryCount = prefs.getInt("count", 0);
  if (entryCount < 0) entryCount = 0;
  if (entryCount > WIFI_MAX_SAVED_NETWORKS) entryCount = WIFI_MAX_SAVED_NETWORKS;

  for (int i = 0; i < entryCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "s%d", i);
    copyTruncated(ssids[i], sizeof(ssids[i]), prefs.getString(key, "").c_str());
    snprintf(key, sizeof(key), "p%d", i);
    copyTruncated(passwords[i], sizeof(passwords[i]), prefs.getString(key, "").c_str());
  }
}

int NetworkStore::count() { return entryCount; }

const char* NetworkStore::ssid(int i) { return (i >= 0 && i < entryCount) ? ssids[i] : ""; }

const char* NetworkStore::password(int i) { return (i >= 0 && i < entryCount) ? passwords[i] : ""; }

int NetworkStore::add(const char* newSsid, const char* newPassword) {
  if (!newSsid || !newSsid[0]) return -1;

  int slot = -1;
  for (int i = 0; i < entryCount; i++) {
    if (strcmp(ssids[i], newSsid) == 0) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    if (entryCount >= WIFI_MAX_SAVED_NETWORKS) return -1;
    slot = entryCount++;
  }

  copyTruncated(ssids[slot], sizeof(ssids[slot]), newSsid);
  copyTruncated(passwords[slot], sizeof(passwords[slot]), newPassword ? newPassword : "");
  save();
  return slot;
}

bool NetworkStore::remove(int i) {
  if (i < 0 || i >= entryCount) return false;
  for (int k = i; k < entryCount - 1; k++) {
    memcpy(ssids[k], ssids[k + 1], sizeof(ssids[k]));
    memcpy(passwords[k], passwords[k + 1], sizeof(passwords[k]));
  }
  entryCount--;
  save();
  return true;
}
