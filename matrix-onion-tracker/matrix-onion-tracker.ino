// Crypto price + onion-link status board: ELEGOO ESP-WROOM-32 + Waveshare
// 64x32 HUB75 matrix. Same panel wiring as led-matrix-audio-visualizer, minus
// the sound detector -- this one will poll prices and .onion uptime over
// WiFi instead of listening to a mic.
//
// This is a scaffold only: the screens below draw mock data so the matrix,
// button and screen-switching can be demoed before any networking or
// price/onion-checking logic exists. See README.md.

#include "src/config.h"
#include "src/core/Matrix.h"
#include "src/core/Screen.h"

Display* display = nullptr;
size_t current = 0;
uint32_t lastSwitchMs = 0;
float measuredRenderFps = 0;

void selectScreen(size_t index) {
  size_t n = Screens::count();
  current = (index + n) % n;
  Screens::get(current)->reset();
  lastSwitchMs = millis();
  Serial.printf("[%u] %s\n", (unsigned)current, Screens::get(current)->name());
}

void listScreens() {
  for (size_t i = 0; i < Screens::count(); i++) {
    Serial.printf("%c %u: %s\n", i == current ? '*' : ' ', (unsigned)i, Screens::get(i)->name());
  }
}

void handleButton() {
  static bool wasPressed = false;
  static uint32_t lastChangeMs = 0;
  bool pressed = digitalRead(Pins::BUTTON) == LOW;
  if (pressed != wasPressed && millis() - lastChangeMs > 30) {  // debounce
    lastChangeMs = millis();
    wasPressed = pressed;
    if (pressed) selectScreen(current + 1);
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'n') selectScreen(current + 1);
    else if (c == 'p') selectScreen(current + Screens::count() - 1);
    else if (c == 'l') listScreens();
    else if (c == 'd') Serial.printf("render %.1f fps\n", measuredRenderFps);
    else if (c >= '0' && c <= '9' && (size_t)(c - '0') < Screens::count()) selectScreen(c - '0');
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

  if (Screens::count() == 0) {
    Serial.println("No screens registered");
    while (true) delay(1000);
  }
  Serial.println("Crypto/onion board running (demo data, no networking yet)");
  listScreens();
  selectScreen(0);
}

void loop() {
  handleButton();
  handleSerial();
  if (AUTO_CYCLE_MS && millis() - lastSwitchMs >= AUTO_CYCLE_MS) selectScreen(current + 1);

  display->fillScreen(0);
  Screens::get(current)->draw(*display);
  display->flipDMABuffer();

  measureRenderFps();
  // Hold a steady frame rate.
  static TickType_t lastWake = xTaskGetTickCount();
  vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000 / RENDER_FPS));
}
