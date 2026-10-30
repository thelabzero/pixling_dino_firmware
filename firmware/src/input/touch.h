// Touch gestures from one digital touch pad (TTP223 / HW-763): tap, double tap, long press, hold.
// Pure C++ (no Arduino): feed it the pad state every loop with update(pressed, nowMs),
// then read events with next(). Desktop-tested in test/test_touch.
//
//   tap          press shorter than tapMaxMs, and no second tap within doubleGapMs
//   double tap   two taps, the second starting within doubleGapMs of the first release
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
};

class TouchGestures {
 public:
  explicit TouchGestures(const TouchConfig& c = TouchConfig()) : cfg_(c) {}
  void setConfig(const TouchConfig& c) { cfg_ = c; }
  void update(bool pressed, uint32_t nowMs);
  Event next();
  bool pressed() const { return stable_; }

 private:
  TouchConfig cfg_;
  bool raw_ = false, stable_ = false;
  uint32_t rawSince_ = 0;       // when the raw level last changed
  uint32_t downAt_ = 0;         // when the current press started
  bool holdSent_ = false;
  bool tapPending_ = false;     // one tap seen, waiting to see if a second follows
  uint32_t tapUpAt_ = 0;
  Event queue_[4] = {};
  uint8_t qHead_ = 0, qLen_ = 0;
  void push(Event e);
  void onDown(uint32_t now);
  void onUp(uint32_t now);
};

}  // namespace input
