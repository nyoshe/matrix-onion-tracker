#pragma once

#include <stdint.h>

// ESP32 WiFi scan/connect, polled from WifiFlow so neither blocks the render
// loop.
namespace Net {
// Call once from setup(). Starts STA mode and, if a password was saved from
// an earlier connect, reconnects to it in the background.
void begin();

void startScan();

enum class ScanState { kScanning, kDone, kFailed };
ScanState scanState();  // also caches results the first time it reports kDone

int networkCount();  // valid once scanState() == kDone
const char* networkSsid(int i);
bool networkIsOpen(int i);  // true if it needs no password

void connect(const char* ssid, const char* password);

enum class ConnectState { kIdle, kConnecting, kConnected, kFailed };
ConnectState connectState();

bool isConnected();
const char* connectedSsid();  // valid when isConnected(); call once per frame
}  // namespace Net
