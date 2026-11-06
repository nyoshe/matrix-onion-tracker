#include "Encoder.h"

#include <Arduino.h>

#include "../config.h"

namespace {

// Quadrature state machine (Ben Buxton's "full-step" rotary decode table).
// Walks a graph of valid CLK/DT states and only emits a step once a
// complete transition sequence finishes; an edge sampled mid-settle (e.g.
// from RC-slowed signals on a breakout with debounce caps) just lands on an
// intermediate state instead of emitting a wrong-direction step. This is why
// there's no time-based debounce here: skipping an edge to "debounce" it
// would desync the table from the pins' real state.
constexpr uint8_t R_START     = 0x0;
constexpr uint8_t R_CW_FINAL  = 0x1;
constexpr uint8_t R_CW_BEGIN  = 0x2;
constexpr uint8_t R_CW_NEXT   = 0x3;
constexpr uint8_t R_CCW_BEGIN = 0x4;
constexpr uint8_t R_CCW_FINAL = 0x5;
constexpr uint8_t R_CCW_NEXT  = 0x6;
constexpr uint8_t DIR_CW  = 0x10;
constexpr uint8_t DIR_CCW = 0x20;

// Row = current state (low nibble of the previous entry); column =
// (CLK << 1 | DT). If turning the knob one way reports the wrong direction,
// that's a calibration thing, not a bug -- swap ENCODER_CLK/ENCODER_DT in
// config.h, same as before.
constexpr uint8_t kTransitionTable[7][4] = {
    {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
    {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
    {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
    {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
    {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

volatile uint8_t ringState = R_START;
volatile int stepAccumulator = 0;

void IRAM_ATTR onEncoderEdge() {
  uint8_t pinState = (digitalRead(Pins::ENCODER_CLK) << 1) | digitalRead(Pins::ENCODER_DT);
  ringState = kTransitionTable[ringState & 0xf][pinState];
  if (ringState & DIR_CW) stepAccumulator++;
  else if (ringState & DIR_CCW) stepAccumulator--;
}

}  // namespace

void Encoder::begin() {
  pinMode(Pins::ENCODER_CLK, INPUT);
  pinMode(Pins::ENCODER_DT, INPUT);
  pinMode(Pins::ENCODER_SW, INPUT);  // GPIO39 has no internal pull-up; module supplies its own
  attachInterrupt(Pins::ENCODER_CLK, onEncoderEdge, CHANGE);
  attachInterrupt(Pins::ENCODER_DT, onEncoderEdge, CHANGE);
}

int Encoder::takeSteps() {
  noInterrupts();
  int steps = stepAccumulator;
  stepAccumulator = 0;
  interrupts();
  return steps;
}

namespace {
bool pendingClick = false;
bool pendingLongClick = false;
}  // namespace

void Encoder::pollButton() {
  static bool wasPressed = false;
  static bool candidate = false;
  static uint8_t agreeCount = 0;
  static uint32_t pressStartMs = 0;
  static bool longFired = false;

  bool raw = digitalRead(Pins::ENCODER_SW) == LOW;

  // GPIO39 gets brief spurious pull-downs when the radio's ADC is active;
  // require two consecutive polls to agree before trusting a level change.
  if (raw == candidate) {
    if (agreeCount < 2) agreeCount++;
  } else {
    candidate = raw;
    agreeCount = 1;
  }
  bool pressed = (agreeCount >= 2) ? candidate : wasPressed;

  uint32_t now = millis();
  if (pressed && !wasPressed) {
    pressStartMs = now;
    longFired = false;
  } else if (pressed && wasPressed && !longFired && now - pressStartMs >= ENCODER_LONG_PRESS_MS) {
    longFired = true;
    pendingLongClick = true;
  } else if (!pressed && wasPressed && !longFired) {
    pendingClick = true;  // short press, released before the long threshold
  }
  wasPressed = pressed;
}

bool Encoder::takeClick() {
  bool c = pendingClick;
  pendingClick = false;
  return c;
}

bool Encoder::takeLongClick() {
  bool c = pendingLongClick;
  pendingLongClick = false;
  return c;
}
