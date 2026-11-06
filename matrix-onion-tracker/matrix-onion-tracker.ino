// Crypto price + onion-link status board: ELEGOO ESP-WROOM-32 + Waveshare
// 64x32 HUB75 matrix + KY-040 rotary encoder. Same panel wiring as
// led-matrix-audio-visualizer, minus the sound detector -- this one will
// poll prices and .onion uptime over WiFi instead of listening to a mic.
//
// This is a scaffold only: the screens below draw mock data so the matrix,
// button, encoder menu and screen-switching can be demoed before any
// networking or price/onion-checking logic exists. See README.md.

#include "src/config.h"
#include "src/core/Encoder.h"
#include "src/core/Matrix.h"
#include "src/core/Menu.h"
#include "src/core/Nav.h"
#include "src/core/Screen.h"

Display* display = nullptr;
uint32_t lastSwitchMs = 0;
float measuredRenderFps = 0;
bool encoderDebug = false;

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

  if (Screens::count() == 0) {
    Serial.println("No screens registered");
    while (true) delay(1000);
  }
  Serial.println("Crypto/onion board running (demo data, no networking yet)");
  Nav::begin();
  Nav::listScreens();
}

void loop() {
  handleButton();
  handleSerial();

  int steps = Encoder::takeSteps();
  bool click = Encoder::takeClick();
  if (encoderDebug && (steps || click)) Serial.printf("encoder steps=%d click=%d\n", steps, click);

  bool menuOpen = Menu::update(steps, click);
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
