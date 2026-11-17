// Touch gestures from one digital touch pad (TTP223 / HW-763): tap, double tap, long press, hold.
// Pure C++ (no Arduino): feed it the pad state every loop with update(pressed, nowMs),
// then read events with next(). Desktop-tested in test/test_touch.
//
//   tap          press shorter than tapMaxMs, and no second tap within doubleGapMs
//   double tap   two taps, the second starting within doubleGapMs of the first release
//   triple / quad tap  (maxTaps >= 3 / 4) three / four taps in a row, each within doubleGapMs of the last release;
//                the Nth tap of maxTaps is sent at once, fewer are sent when the gap runs out
//   long press   released after longMs, before holdMs          (sent on release)
//   hold         still pressed at holdMs                         (sent while held)
// Presses between tapMaxMs and longMs are ignored (not a tap, not a long press).
// Long press is sent on release so a hold can still turn into a hold: in "set alarm",
// long press = leave, hold = alarm off, and both start the same way.
#pragma once
#include <stdint.h>
#include "events.h"

namespace input {

struct TouchConfig {
  uint16_t debounceMs = 30;     // ignore changes shorter than this
  uint16_t tapMaxMs = 300;
  uint16_t doubleGapMs = 400;
  uint16_t longMs = 1500;
  uint16_t holdMs = 3000;
  uint8_t maxTaps = 2;          // count up to this many taps in a row (2 = tap and double tap only, 4 = up to a quad tap)
};

class TouchGestures {
 public:
  explicit TouchGestures(const TouchConfig& c = TouchConfig()) : cfg_(c) {}
  void setConfig(const TouchConfig& c) { cfg_ = c; }
  // Change the wait for a next tap while running (0 = a tap is sent at once). Touch-only mode counts taps on the home
  // screen only; in a menu every tap must act immediately.
  void setGapMs(uint16_t ms) { cfg_.doubleGapMs = ms; }
  void update(bool pressed, uint32_t nowMs);
  Event next();
  bool pressed() const { return stable_; }

 private:
  TouchConfig cfg_;
  bool raw_ = false, stable_ = false;
  uint32_t rawSince_ = 0;       // when the raw level last changed
  uint32_t downAt_ = 0;         // when the current press started
  bool holdSent_ = false;
  uint8_t taps_ = 0;            // taps seen in a row, waiting to see if another follows
  uint32_t tapUpAt_ = 0;
  Event queue_[4] = {};
  uint8_t qHead_ = 0, qLen_ = 0;
  void push(Event e);
  void flushTaps();
  void onDown(uint32_t now);
  void onUp(uint32_t now);
};

}  // namespace input
