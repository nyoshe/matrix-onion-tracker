#pragma once

// WiFi access point + HTTP config portal: lets you add/remove saved
// networks (src/core/NetworkStore) from a browser. The server runs
// regardless of AP/STA mode, so it stays reachable for future settings
// (tracked currencies/sites, etc.) once connected normally too.
namespace ConfigPortal {
void begin();  // call once from setup(), after NetworkStore::begin() and Net::begin()
void poll();   // call every frame
}  // namespace ConfigPortal
