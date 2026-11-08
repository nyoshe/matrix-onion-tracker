#include "NetworkView.h"

#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "Color.h"
#include "Net.h"
#include "QrEncode.h"

namespace {
// Two QR codes are ever shown here: the connected-state portal URL (fits
// comfortably in version 1's alphanumeric capacity) and the AP-state
// WiFi-join payload (forced into byte mode by the required semicolons in
// "WIFI:T:WPA;S:...;P:...;;", so it needs more raw capacity than its
// character count alone suggests -- version 3 gives 55 bytes at ECC_LOW
// against this payload's ~41-byte requirement). qrBuf is sized for the
// larger of the two; margin shrinks for the AP code so its bigger backdrop
// still fits the panel's 32px height.
constexpr int kQrVersionUrl = 1;
constexpr int kQrMarginUrl = 2;
constexpr int kQrVersionAp = 3;
constexpr int kQrMarginAp = 1;
constexpr int kQrSizeMax = kQrVersionAp * 4 + 17;  // 29x29, the larger case
constexpr int kQrBufSize = (kQrSizeMax * kQrSizeMax + 7) / 8;

QRCode qr;
uint8_t qrBuf[kQrBufSize];
bool qrGood = false;
int qrSize = 0;     // modules per side of whatever's currently encoded
int qrMargin = 0;   // quiet zone used for the currently-encoded code

char lastPayload[48] = "";
bool lastWasAp = false;  // forces a regen on switching case, even same string
int urlScrollX = PANEL_WIDTH;
int netScrollX = PANEL_WIDTH;

const char* const kApLines[] = {"Scan to", "setup", "network"};
constexpr int kApLineCount = 3;
constexpr int kApLinePitch = 8;

// Same marquee as CandlestickScreen's header line, anchored at textX instead
// of the panel's left edge when it fits.
void drawScrollLine(Display& display, const char* text, int y, int& scrollX, int textX, uint16_t color) {
  int16_t boundsX, boundsY;
  uint16_t lineWidth, lineHeight;
  display.getTextBounds(text, 0, 0, &boundsX, &boundsY, &lineWidth, &lineHeight);

  int x;
  if ((int)lineWidth <= PANEL_WIDTH - textX) {
    x = textX;
  } else {
    x = scrollX;
    scrollX = (scrollX <= -(int)lineWidth) ? PANEL_WIDTH : scrollX - 1;
  }

  display.setCursor(x, y + FONT_ASCENT);
  display.setTextColor(color);
  display.print(text);
}

// TomThumb is fixed-width (FONT_ADVANCE px/char), so centering is plain
// arithmetic -- no need for a per-string getTextBounds() measurement.
void drawCenteredLines(Display& display, const char* const* lines, int count, int areaX, int areaWidth,
                        int areaHeight, uint16_t color) {
  int blockHeight = count * kApLinePitch;
  int y0 = (areaHeight - blockHeight) / 2;
  display.setTextColor(color);
  for (int i = 0; i < count; i++) {
    int w = (int)strlen(lines[i]) * FONT_ADVANCE;
    int x = areaX + (areaWidth - w) / 2;
    display.setCursor(x, y0 + i * kApLinePitch + FONT_ASCENT);
    display.print(lines[i]);
  }
}

void drawQr(Display& display) {
  int backdrop = qrSize + 2 * qrMargin;
  // Drawn first here (unlike the connected case) since there's no scrolling
  // text to worry about overdrawing -- the centered lines are placed outside
  // the backdrop's width entirely.
  display.fillRect(0, 0, backdrop, backdrop, 0xFFFF);
  if (!qrGood) return;
  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      if (qrcode_getModule(&qr, x, y)) display.drawPixel(qrMargin + x, qrMargin + y, 0x0000);
    }
  }
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

  if (Net::isApActive() && !Net::isConnected()) {
    char payload[48];
    snprintf(payload, sizeof(payload), "WIFI:T:WPA;S:%s;P:%s;;", Net::apSsid(), Net::apPassword());
    if (!lastWasAp || strcmp(payload, lastPayload) != 0) {
      strncpy(lastPayload, payload, sizeof(lastPayload) - 1);
      lastPayload[sizeof(lastPayload) - 1] = '\0';
      lastWasAp = true;
      qrGood = qrcode_initText(&qr, qrBuf, kQrVersionAp, ECC_LOW, payload) == 0;
      qrSize = kQrVersionAp * 4 + 17;
      qrMargin = kQrMarginAp;
    }

    drawQr(display);
    int textX = qrMargin + qrSize + qrMargin + 1;
    drawCenteredLines(display, kApLines, kApLineCount, textX, PANEL_WIDTH - textX, PANEL_HEIGHT, hueColor(0.5f));
    return;
  }

  const char* url = Net::configUrl();
  if (lastWasAp || strcmp(url, lastPayload) != 0) {
    strncpy(lastPayload, url, sizeof(lastPayload) - 1);
    lastPayload[sizeof(lastPayload) - 1] = '\0';
    lastWasAp = false;
    qrGood = qrcode_initText(&qr, qrBuf, kQrVersionUrl, ECC_LOW, url) == 0;
    qrSize = kQrVersionUrl * 4 + 17;
    qrMargin = kQrMarginUrl;
  }

  char netLine[40];
  snprintf(netLine, sizeof(netLine), "%s", Net::connectedSsid());

  int textX = qrMargin + qrSize + qrMargin + 1;
  drawScrollLine(display, url, 2, urlScrollX, textX, 0xFFFF);
  drawScrollLine(display, netLine, 14, netScrollX, textX, hueColor(0.5f));

  // Drawn last, over any scrolling text that passed behind it this frame, so
  // the code itself always stays intact and scannable. getModule() returns
  // true for a DARK module, so the quiet zone/background is the lit fill and
  // true modules punch out unlit pixels, not the reverse.
  drawQr(display);
}
