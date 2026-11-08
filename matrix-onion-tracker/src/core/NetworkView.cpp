#include "NetworkView.h"

#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "Color.h"
#include "Net.h"
#include "QrEncode.h"

namespace {
// qrcode_getBufferSize() isn't a constant expression, so this repeats its
// formula (packed 1 bit/module) to size a static buffer for the fixed
// version used below.
constexpr int kQrVersion = 1;
constexpr int kQrSize = kQrVersion * 4 + 17;  // 21x21 modules
constexpr int kQrBufSize = (kQrSize * kQrSize + 7) / 8;
constexpr int kQrMargin = 2;  // quiet zone, so a phone camera can find the edges
constexpr int kBackdrop = kQrSize + 2 * kQrMargin;
constexpr int kTextX = kQrMargin + kQrSize + kQrMargin + 1;

QRCode qr;
uint8_t qrBuf[kQrBufSize];
bool qrGood = false;
char lastUrl[24] = "";
int urlScrollX = PANEL_WIDTH;
int netScrollX = PANEL_WIDTH;

// Same marquee as CandlestickScreen's header line, anchored at kTextX
// instead of the panel's left edge when it fits.
void drawScrollLine(Display& display, const char* text, int y, int& scrollX, uint16_t color) {
  int16_t boundsX, boundsY;
  uint16_t lineWidth, lineHeight;
  display.getTextBounds(text, 0, 0, &boundsX, &boundsY, &lineWidth, &lineHeight);

  int x;
  if ((int)lineWidth <= PANEL_WIDTH - kTextX) {
    x = kTextX;
  } else {
    x = scrollX;
    scrollX = (scrollX <= -(int)lineWidth) ? PANEL_WIDTH : scrollX - 1;
  }

  display.setCursor(x, y + FONT_ASCENT);
  display.setTextColor(color);
  display.print(text);
}
}  // namespace

void NetworkView::reset() {
  urlScrollX = PANEL_WIDTH;  // enter from the right, not mid-scroll
  netScrollX = PANEL_WIDTH;
}

void NetworkView::draw(Display& display) {
  display.setTextSize(1);
  display.setTextWrap(false);

  // Mirrors Net::draw()'s own "no address to show yet" state: the very
  // first connect attempts, before the AP fallback has started.
  if (!Net::isConnected() && !Net::isApActive()) {
    display.setTextColor(0xFFFF);
    display.setCursor(1, 8 + FONT_ASCENT);
    display.print("Connecting...");
    return;
  }

  const char* url = Net::configUrl();
  if (strcmp(url, lastUrl) != 0) {
    strncpy(lastUrl, url, sizeof(lastUrl) - 1);
    lastUrl[sizeof(lastUrl) - 1] = '\0';
    qrGood = qrcode_initText(&qr, qrBuf, kQrVersion, ECC_LOW, url) == 0;
  }

  char netLine[40];
  if (Net::isConnected()) {
    snprintf(netLine, sizeof(netLine), "%s", Net::connectedSsid());
  } else {
    snprintf(netLine, sizeof(netLine), "AP: %s", WIFI_AP_SSID);
  }

  drawScrollLine(display, url, 2, urlScrollX, 0xFFFF);
  drawScrollLine(display, netLine, 14, netScrollX, hueColor(0.5f));

  // Drawn last, over any scrolling text that passed behind it this frame,
  // so the code itself always stays intact and scannable. getModule()
  // returns true for a DARK module, so the quiet zone/background is the
  // lit fill and true modules punch out unlit pixels, not the reverse.
  display.fillRect(0, 0, kBackdrop, kBackdrop, 0xFFFF);
  if (qrGood) {
    for (int y = 0; y < kQrSize; y++) {
      for (int x = 0; x < kQrSize; x++) {
        if (qrcode_getModule(&qr, x, y)) display.drawPixel(kQrMargin + x, kQrMargin + y, 0x0000);
      }
    }
  }
}
