#pragma once

// Persisted list of WiFi networks to try connecting to, managed through the
// web config portal (src/core/ConfigPortal.*). Backed by NVS (Preferences).
namespace NetworkStore {
void begin();  // call once from setup(), before Net::begin()

int count();
const char* ssid(int i);
const char* password(int i);

// Adds a network, or updates its password if the SSID is already saved.
// Returns its index, or -1 if the list is full (WIFI_MAX_SAVED_NETWORKS) or
// ssid is empty.
int add(const char* ssid, const char* password);

bool remove(int i);
}  // namespace NetworkStore
