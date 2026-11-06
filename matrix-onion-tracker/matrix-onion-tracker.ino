// Crypto price + onion-link status board: ELEGOO ESP-WROOM-32 + Waveshare
// 64x32 HUB75 matrix + KY-040 rotary encoder. Same panel wiring as
// led-matrix-audio-visualizer, minus the sound detector -- this one polls
// prices and .onion uptime over WiFi instead of listening to a mic.
//
// The two screens below still draw mock data (price/onion polling isn't
// wired up yet), but the menu's WiFi item makes a real connection. See
// README.md for the full command/menu reference.

#include "src/config.h"
#include "src/core/Encoder.h"
#include "src/core/Matrix.h"
#include "src/core/Menu.h"
#include "src/core/Nav.h"
#include "src/core/Net.h"
#include "src/core/Screen.h"

Display* display = nullptr;
uint32_t lastSwitchMs = 0;
float measuredRenderFps = 0;
bool encoderDebug = false;

// Serial-driven stand-ins for the encoder, for testing the menu without one
// attached or in hand: '<'/'>' turn, 'c' clicks, 'C' long-clicks.
int debugSteps = 0;
bool debugClick = false;
bool debugLongClick = false;

void handleButton() {  // BOOT button: jumps straight to the next screen
  static bool wasPressed = false;
  static uint32_t lastChangeMs = 0;
  bool pressed = digitalRead(Pins::BUTTON) == LOW;
  if (pressed != wasPressed && millis() - lastChangeMs > 30) {  // debounce
    lastChangeMs = millis();
    wasPressed = pressed;
    if (pressed) {
      Nav::selectScreen(Nav::currentScreen() + 1);
      lastSwitchMs = millis();
    }
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'n') { Nav::selectScreen(Nav::currentScreen() + 1); lastSwitchMs = millis(); }
    else if (c == 'p') { Nav::selectScreen(Nav::currentScreen() + (int)Screens::count() - 1); lastSwitchMs = millis(); }
    else if (c == 'l') Nav::listScreens();
    else if (c == 'd') Serial.printf("render %.1f fps\n", measuredRenderFps);
    else if (c == 'e') { encoderDebug = !encoderDebug; Serial.printf("encoder debug %s\n", encoderDebug ? "on" : "off"); }
    else if (c == '<') debugSteps -= 1;
    else if (c == '>') debugSteps += 1;
    else if (c == 'c') debugClick = true;
    else if (c == 'C') debugLongClick = true;
    else if (c >= '0' && c <= '9' && (c - '0') < (int)Screens::count()) {
      Nav::selectScreen(c - '0');
      lastSwitchMs = millis();
    }
  }
}

void measureRenderFps() {
  static uint32_t windowStartMs = millis();
  static uint32_t frames = 0;
  frames++;
  uint32_t elapsed = millis() - windowStartMs;
  if (elapsed >= 1000) {
    measuredRenderFps = frames * 1000.0f / elapsed;
    frames = 0;
    windowStartMs += elapsed;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(Pins::BUTTON, INPUT_PULLUP);

  display = &Matrix::begin();
  Encoder::begin();
  Net::begin();

  if (Screens::count() == 0) {
    Serial.println("No screens registered");
    while (true) delay(1000);
  }
  Serial.println("Crypto/onion board running");
  Nav::begin();
  Nav::listScreens();
}

void loop() {
  handleButton();
  handleSerial();

  Encoder::pollButton();
  int steps = Encoder::takeSteps() + debugSteps;
  bool click = Encoder::takeClick() || debugClick;
  bool longClick = Encoder::takeLongClick() || debugLongClick;
  debugSteps = 0;
  debugClick = false;
  debugLongClick = false;
  if (encoderDebug && (steps || click || longClick)) {
    Serial.printf("encoder steps=%d click=%d longClick=%d\n", steps, click, longClick);
  }

  bool menuOpen = Menu::update(steps, click, longClick);
  if (!menuOpen && AUTO_CYCLE_MS && millis() - lastSwitchMs >= AUTO_CYCLE_MS) {
    Nav::selectScreen(Nav::currentScreen() + 1);
    lastSwitchMs = millis();
  }

  display->fillScreen(0);
  if (menuOpen) Menu::draw(*display);
  else Screens::get(Nav::currentScreen())->draw(*display);
  display->flipDMABuffer();

  measureRenderFps();
  // Hold a steady frame rate.
  static TickType_t lastWake = xTaskGetTickCount();
  vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000 / RENDER_FPS));
}
