#pragma once

#include "Matrix.h"

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

// Credentials for the setup AP, generated fresh each time it's started
// (see startAp() in Net.cpp). Valid once isApActive().
const char* apSsid();
const char* apPassword();

// Background WiFi scan, refreshed periodically once the connect sequence
// has settled (not while actively trying a saved network, so scanning
// doesn't interfere with that attempt). Results are sorted strongest-first.
// -1 means no scan has completed yet; 0 or more is an actual count.
int scanCount();
const char* scanSsid(int i);
int32_t scanRssi(int i);  // dBm; closer to 0 is stronger
bool scanIsOpen(int i);   // no password needed

// (Re)starts the connect sequence at NetworkStore index i, continuing
// through the rest of the list on failure before falling back to the AP.
void tryIndex(int i);

// "HTTP://<ip>/" for whichever address currently reaches the config portal
// -- the AP's fixed IP before a network is joined, the joined network's
// address afterward. Uppercase scheme (browsers don't care) so the whole
// string stays in the QR code's smaller alphanumeric charset.
const char* configUrl();

// Draws connecting/AP-mode status; call instead of the active screen
// whenever !isConnected().
void draw(Display& display);
}  // namespace Net
