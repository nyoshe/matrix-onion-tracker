#include "Net.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "../config.h"

namespace {

constexpr int kMaxNetworks = 16;
char ssidBuf[kMaxNetworks][33];
bool openBuf[kMaxNetworks];
int networkCountCached = 0;
bool resultsCached = false;

char connectedSsidBuf[33] = "";

Net::ConnectState connectStateValue = Net::ConnectState::kIdle;
uint32_t connectStartMs = 0;

bool wifiStarted = false;

void ensureStarted() {
  if (wifiStarted) return;
  wifiStarted = true;
  WiFi.mode(WIFI_STA);
}

void copyTruncated(char* dst, size_t dstLen, const char* src) {
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

}  // namespace

void Net::begin() {
  ensureStarted();
  Serial.println("WiFi: reconnecting with saved credentials (if any)...");
  WiFi.begin();  // "connect to SDK config": reconnects using NVS-saved credentials
}

void Net::startScan() {
  ensureStarted();
  resultsCached = false;
  networkCountCached = 0;
  WiFi.scanNetworks(true);
}

Net::ScanState Net::scanState() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return ScanState::kScanning;
  if (n == WIFI_SCAN_FAILED) return ScanState::kFailed;

  if (!resultsCached) {
    // Skip hidden (empty-SSID) networks and dedupe mesh APs that repeat the
    // same SSID on every node -- otherwise the connected-marker would match
    // more than one row.
    for (int i = 0; i < n && networkCountCached < kMaxNetworks; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;
      bool duplicate = false;
      for (int j = 0; j < networkCountCached; j++) {
        if (strcmp(ssidBuf[j], ssid.c_str()) == 0) { duplicate = true; break; }
      }
      if (duplicate) continue;
      copyTruncated(ssidBuf[networkCountCached], sizeof(ssidBuf[0]), ssid.c_str());
      openBuf[networkCountCached] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
      networkCountCached++;
    }
    resultsCached = true;
  }
  return ScanState::kDone;
}

int Net::networkCount() { return networkCountCached; }

const char* Net::networkSsid(int i) {
  return (i >= 0 && i < networkCountCached) ? ssidBuf[i] : "";
}

bool Net::networkIsOpen(int i) {
  return (i >= 0 && i < networkCountCached) && openBuf[i];
}

void Net::connect(const char* ssid, const char* password) {
  ensureStarted();
  connectStateValue = ConnectState::kConnecting;
  connectStartMs = millis();
  WiFi.begin(ssid, (password && password[0]) ? password : nullptr);
}

Net::ConnectState Net::connectState() {
  if (connectStateValue != ConnectState::kConnecting) return connectStateValue;

  if (WiFi.status() == WL_CONNECTED) {
    connectStateValue = ConnectState::kConnected;
  } else if (millis() - connectStartMs > WIFI_CONNECT_TIMEOUT_MS ||
             WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
    WiFi.disconnect();  // otherwise the station keeps retrying in the background
    connectStateValue = ConnectState::kFailed;
  }
  return connectStateValue;
}

bool Net::isConnected() { return WiFi.status() == WL_CONNECTED; }

const char* Net::connectedSsid() {
  if (WiFi.status() == WL_CONNECTED) {
    copyTruncated(connectedSsidBuf, sizeof(connectedSsidBuf), WiFi.SSID().c_str());
  }
  return connectedSsidBuf;
}
