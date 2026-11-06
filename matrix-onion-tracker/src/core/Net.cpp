#include "Net.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "../config.h"
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

void copyTruncated(char* dst, size_t dstLen, const char* src) {
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

void startAp() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  apUp = true;
  mode = Mode::kApOnly;
  Serial.printf("WiFi: no network joined, AP '%s' up at %s\n", WIFI_AP_SSID,
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
  if (mode != Mode::kTrying) return;

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
