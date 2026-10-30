#include "touch_input.h"
#include "config.h"
#include "../app/app.h"

bool TouchInput::begin(App&) {
  pinMode(PIN_TOUCH, INPUT);  // the module drives the line itself (push-pull output)
  input::TouchConfig c;
  c.debounceMs = TOUCH_DEBOUNCE_MS;
  c.tapMaxMs = TOUCH_TAP_MAX_MS;
  c.doubleGapMs = TOUCH_DOUBLE_GAP_MS;
  c.longMs = TOUCH_LONG_MS;
  c.holdMs = TOUCH_HOLD_MS;
  gestures_.setConfig(c);
  Serial.printf("[touch] pad on GPIO%d, %s now\n", PIN_TOUCH, read() ? "TOUCHED" : "not touched");
  return true;  // a digital pin cannot tell whether a pad is connected: check with "i" or LOG_TOUCH_RAW
}

bool TouchInput::read() const { return digitalRead(PIN_TOUCH) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW); }

void TouchInput::loop(App& app, uint32_t now) {
  const bool p = read();
  if (p != lastPressed_) {
    lastPressed_ = p;
    if (p) { presses_++; downAt_ = now; }
#if LOG_TOUCH_RAW
    if (p) Serial.println("[touch] down");
    else Serial.printf("[touch] up after %lu ms\n", (unsigned long)(now - downAt_));
#endif
  }
  gestures_.update(p, now);
  for (input::Event e; (e = gestures_.next()) != input::Event::None;) app.emit(e);
}

void TouchInput::printInfo(Print& out) const {
  out.printf("[touch] GPIO%d %s, presses since start %lu\n", PIN_TOUCH, read() ? "TOUCHED" : "not touched",
             (unsigned long)presses_);
}
