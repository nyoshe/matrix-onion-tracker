#pragma once

// WiFi connection management: tries each saved network (NetworkStore) in
// turn, falling back to a WiFi access point if none connect (or none are
// saved). ConfigPortal calls tryIndex() to test a newly added network
// without losing AP access if it fails.
namespace Net {
void begin();  // call once from setup(), after NetworkStore::begin()
void poll();   // call every frame

bool isConnected();
const char* connectedSsid();  // valid when isConnected()
const char* staIp();          // valid when isConnected()

bool isApActive();

// (Re)starts the connect sequence at NetworkStore index i, continuing
// through the rest of the list on failure before falling back to the AP.
void tryIndex(int i);
}  // namespace Net
