#include "WifiFlow.h"

#include <string.h>

#include "../config.h"
#include "Color.h"
#include "Net.h"

namespace {

enum class State { kScanning, kList, kPassword, kConnecting, kResult };
State state = State::kScanning;

constexpr int kRowHeight = 7;
constexpr int kVisibleRows = 4;

int listSelected = 0;

// Password chars, most-likely-first so common passwords need less scrolling.
constexpr char kCharset[] =
    "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*-_.";
constexpr int kCharsetLen = sizeof(kCharset) - 1;
constexpr int kMaxPasswordLen = 32;

char password[kMaxPasswordLen + 1] = "";
int passwordLen = 0;
int charIndex = 0;
bool charChosen = false;  // has the knob been turned for the current slot?
int pendingNetwork = -1;

bool lastConnectOk = false;
char resultSsid[33] = "";

int wrapIndex(int value, int n) { return ((value % n) + n) % n; }

void enterList() {
  state = State::kList;
  listSelected = 0;
}

void startPasswordEntry(int networkIndex) {
  pendingNetwork = networkIndex;
  password[0] = '\0';
  passwordLen = 0;
  charIndex = 0;
  charChosen = false;
  state = State::kPassword;
}

void beginConnect(const char* ssid, const char* pass) {
  Serial.printf("WiFi: connecting to %s\n", ssid);
  strncpy(resultSsid, ssid, sizeof(resultSsid) - 1);
  resultSsid[sizeof(resultSsid) - 1] = '\0';
  Net::connect(ssid, pass);
  state = State::kConnecting;
}

}  // namespace

void WifiFlow::begin() {
  Serial.println("WiFi: scanning...");
  Net::startScan();
  state = State::kScanning;
}

bool WifiFlow::update(int steps, bool click, bool longClick) {
  switch (state) {
    case State::kScanning: {
      Net::ScanState s = Net::scanState();
      if (s != Net::ScanState::kScanning) {
        Serial.printf("WiFi: found %d network(s)\n", Net::networkCount());
        for (int i = 0; i < Net::networkCount(); i++) {
          Serial.printf("  %d: %s%s\n", i, Net::networkSsid(i), Net::networkIsOpen(i) ? " (open)" : "");
        }
        enterList();
      }
      break;
    }

    case State::kList: {
      int rows = Net::networkCount() + 2;  // + Rescan, Back
      if (steps) listSelected = wrapIndex(listSelected + steps, rows);
      if (click) {
        if (listSelected == Net::networkCount()) {
          Serial.println("WiFi: rescanning...");
          Net::startScan();
          state = State::kScanning;
        } else if (listSelected == Net::networkCount() + 1) {
          return false;  // Back
        } else if (Net::networkIsOpen(listSelected)) {
          beginConnect(Net::networkSsid(listSelected), "");
        } else {
          startPasswordEntry(listSelected);
        }
      }
      break;
    }

    case State::kPassword: {
      if (steps) {
        charChosen = true;
        charIndex = wrapIndex(charIndex + steps, kCharsetLen);
      }
      if (click && charChosen && passwordLen < kMaxPasswordLen) {
        password[passwordLen++] = kCharset[charIndex];
        password[passwordLen] = '\0';
        charChosen = false;
        charIndex = 0;
      }
      if (longClick) {
        if (charChosen && passwordLen < kMaxPasswordLen) {
          password[passwordLen++] = kCharset[charIndex];
          password[passwordLen] = '\0';
        }
        beginConnect(Net::networkSsid(pendingNetwork), password);
      }
      break;
    }

    case State::kConnecting: {
      Net::ConnectState s = Net::connectState();
      if (s == Net::ConnectState::kConnected) {
        Serial.printf("WiFi: connected to %s\n", resultSsid);
        lastConnectOk = true;
        state = State::kResult;
      } else if (s == Net::ConnectState::kFailed) {
        Serial.printf("WiFi: failed to connect to %s\n", resultSsid);
        lastConnectOk = false;
        state = State::kResult;
      }
      break;
    }

    case State::kResult:
      if (click || longClick) enterList();
      break;
  }
  return true;
}

void WifiFlow::draw(Display& display) {
  display.setTextWrap(false);

  switch (state) {
    case State::kScanning:
      display.setTextColor(0xFFFF);
      display.setCursor(2, 10 + FONT_ASCENT);
      display.print("Scanning...");
      break;

    case State::kList: {
      int count = Net::networkCount();
      int rows = count + 2;
      const char* connected = Net::isConnected() ? Net::connectedSsid() : "";

      int firstVisible = clampInt(listSelected - 1, 0, max(rows - kVisibleRows, 0));
      for (int row = 0; row < kVisibleRows && firstVisible + row < rows; row++) {
        int i = firstVisible + row;
        bool isSelected = (i == listSelected);
        int y = 1 + row * kRowHeight;
        display.setTextColor(isSelected ? hueColor(0.12f) : 0xFFFF);
        display.setCursor(1, y + FONT_ASCENT);
        display.print(isSelected ? "> " : "  ");
        if (i < count) {
          display.print(Net::networkSsid(i));
          if (connected[0] && strcmp(connected, Net::networkSsid(i)) == 0) {
            display.fillCircle(PANEL_WIDTH - 3, y + 2, 2, levelColor(0.0f));
          }
        } else if (i == count) {
          display.print("Rescan");
        } else {
          display.print("Back");
        }
      }
      break;
    }

    case State::kPassword:
      display.setTextColor(0xFFFF);
      display.setCursor(2, FONT_ASCENT);
      display.print(Net::networkSsid(pendingNetwork));

      display.setCursor(2, 10 + FONT_ASCENT);
      for (int i = 0; i < passwordLen; i++) display.print('*');
      display.setTextColor(hueColor(0.5f));  // cyan: live, uncommitted character
      display.print(charChosen ? kCharset[charIndex] : '_');
      break;

    case State::kConnecting:
      display.setTextColor(0xFFFF);
      display.setCursor(2, 10 + FONT_ASCENT);
      display.print("Connecting...");
      break;

    case State::kResult:
      display.setTextColor(lastConnectOk ? levelColor(0.0f) : levelColor(1.0f));
      display.setCursor(2, 5 + FONT_ASCENT);
      display.print(lastConnectOk ? "Connected" : "Failed");
      display.setTextColor(0xFFFF);
      display.setCursor(2, 15 + FONT_ASCENT);
      display.print(resultSsid);
      break;
  }
}
