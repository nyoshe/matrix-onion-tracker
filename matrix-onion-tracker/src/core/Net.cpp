#include "Net.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_random.h>
#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "Color.h"
#include "NetworkStore.h"

namespace {

enum class Mode { kTrying, kConnected, kApOnly };
Mode mode = Mode::kTrying;
bool apUp = false;
int currentIndex = 0;
int attemptsRemaining = 0;
uint32_t attemptStartMs = 0;

char connectedSsidBuf[33] = "";
char staIpBuf[16] = "";

// Setup-AP credentials, regenerated each time startAp() runs (once per
// boot). Random rather than fixed so joining requires scanning the QR
// NetworkView draws, rather than anyone in range joining an open network
// by name alone.
char apSsidBuf[16] = "";
char apPasswordBuf[11] = "";

// Fills dst[0..len) with random characters from charset, then NUL-terminates
// at dst[len]. esp_random() is the hardware TRNG -- appropriate here since
// this is a security-relevant secret, unlike the software-seeded random().
void fillRandom(char* dst, int len, const char* charset, int charsetLen) {
  for (int i = 0; i < len; i++) dst[i] = charset[esp_random() % charsetLen];
  dst[len] = '\0';
}

constexpr int kMaxScanResults = 12;
constexpr uint32_t kScanIntervalMs = 15000;  // how often to refresh once settled

struct ScanEntry {
  char ssid[33];
  int32_t rssi;
  bool open;
};
ScanEntry scanResults[kMaxScanResults];
int scanResultCount = -1;  // -1: no scan has completed yet
uint32_t nextScanMs = 0;   // 0 so the first scan fires as soon as polling starts

// softAP() below is called with no explicit IP config, which defaults to
// this. Shared with configUrl() so the two can't drift apart.
constexpr const char* kApIp = "192.168.4.1";

void copyTruncated(char* dst, size_t dstLen, const char* src) {
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

void startAp() {
  strcpy(apSsidBuf, "Matrix-");
  fillRandom(apSsidBuf + 7, 4, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", 36);
  fillRandom(apPasswordBuf, 10,
             "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 62);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsidBuf, apPasswordBuf);
  apUp = true;
  mode = Mode::kApOnly;
  Serial.printf("WiFi: no network joined, AP '%s' up at %s\n", apSsidBuf,
                WiFi.softAPIP().toString().c_str());
}

void startAttempt(int index) {
  currentIndex = index;
  attemptStartMs = millis();
  WiFi.mode(apUp ? WIFI_AP_STA : WIFI_STA);
  Serial.printf("WiFi: trying %s\n", NetworkStore::ssid(index));
  WiFi.begin(NetworkStore::ssid(index), NetworkStore::password(index));
  mode = Mode::kTrying;
}

// scanNetworks() calls WiFi.enableSTA(true) internally rather than replacing
// the current mode, so running this while the AP is up doesn't drop it.
// Only called once the connect sequence has settled (see Net::poll()), so a
// scan never competes with an in-progress WiFi.begin() attempt.
void pollScan() {
  int16_t n = WiFi.scanComplete();
  if (n >= 0) {
    int count = n < kMaxScanResults ? n : kMaxScanResults;
    for (int i = 0; i < count; i++) {
      copyTruncated(scanResults[i].ssid, sizeof(scanResults[i].ssid), WiFi.SSID(i).c_str());
      scanResults[i].rssi = WiFi.RSSI(i);
      scanResults[i].open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();

    // Strongest signal first, so the list reads top-down by usefulness.
    for (int i = 1; i < count; i++) {
      ScanEntry key = scanResults[i];
      int j = i - 1;
      while (j >= 0 && scanResults[j].rssi < key.rssi) {
        scanResults[j + 1] = scanResults[j];
        j--;
      }
      scanResults[j + 1] = key;
    }

    scanResultCount = count;
    nextScanMs = millis() + kScanIntervalMs;
  } else if (n == WIFI_SCAN_FAILED && millis() >= nextScanMs) {
    WiFi.scanNetworks(true);  // async; a later pollScan() picks up the result
  }
  // n == WIFI_SCAN_RUNNING: still working, nothing to do yet.
}

}  // namespace

void Net::begin() {
  int n = NetworkStore::count();
  if (n == 0) {
    startAp();
    return;
  }
  attemptsRemaining = n;
  startAttempt(0);
}

void Net::tryIndex(int i) {
  if (i < 0 || i >= NetworkStore::count()) return;
  attemptsRemaining = NetworkStore::count();
  startAttempt(i);
}

void Net::poll() {
  if (mode != Mode::kTrying) {
    pollScan();
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    copyTruncated(connectedSsidBuf, sizeof(connectedSsidBuf), NetworkStore::ssid(currentIndex));
    copyTruncated(staIpBuf, sizeof(staIpBuf), WiFi.localIP().toString().c_str());
    Serial.printf("WiFi: connected to %s, IP %s\n", connectedSsidBuf, staIpBuf);
    apUp = false;
    WiFi.mode(WIFI_STA);  // drop the AP now that a real connection works
    mode = Mode::kConnected;
    return;
  }

  if (millis() - attemptStartMs > WIFI_PER_NETWORK_TIMEOUT_MS) {
    attemptsRemaining--;
    if (attemptsRemaining <= 0) {
      startAp();
      return;
    }
    startAttempt((currentIndex + 1) % NetworkStore::count());
  }
}

bool Net::isConnected() { return mode == Mode::kConnected; }

const char* Net::connectedSsid() { return connectedSsidBuf; }

const char* Net::staIp() { return staIpBuf; }

bool Net::isApActive() { return apUp; }

const char* Net::apSsid() { return apSsidBuf; }

const char* Net::apPassword() { return apPasswordBuf; }

int Net::scanCount() { return scanResultCount; }

const char* Net::scanSsid(int i) {
  return (i >= 0 && i < scanResultCount) ? scanResults[i].ssid : "";
}

int32_t Net::scanRssi(int i) {
  return (i >= 0 && i < scanResultCount) ? scanResults[i].rssi : 0;
}

bool Net::scanIsOpen(int i) {
  return (i >= 0 && i < scanResultCount) ? scanResults[i].open : false;
}

const char* Net::configUrl() {
  static char buf[24];
  snprintf(buf, sizeof(buf), "HTTP://%s/", isConnected() ? staIpBuf : kApIp);
  return buf;
}

void Net::draw(Display& display) {
  display.setTextSize(1);
  display.setTextWrap(false);

  if (apUp) {
    display.setTextColor(0xFFFF);
    display.setCursor(1, 2 + FONT_ASCENT);
    display.print("Connect to:");
    display.setCursor(1, 12 + FONT_ASCENT);
    display.print(apSsidBuf);
    display.setTextColor(hueColor(0.5f));  // cyan: the actionable step
    display.setCursor(1, 22 + FONT_ASCENT);
    display.print(kApIp);
  } else {
    display.setTextColor(0xFFFF);
    display.setCursor(1, 8 + FONT_ASCENT);
    display.print("Connecting...");
    display.setTextColor(hueColor(0.5f));
    display.setCursor(1, 18 + FONT_ASCENT);
    display.print(NetworkStore::ssid(currentIndex));
  }
}
