#include "touch.h"

namespace input {

void TouchGestures::push(Event e) {
  if (qLen_ < 4) queue_[(qHead_ + qLen_++) % 4] = e;
}

Event TouchGestures::next() {
  if (!qLen_) return Event::None;
  Event e = queue_[qHead_];
  qHead_ = (qHead_ + 1) % 4;
  qLen_--;
  return e;
}

void TouchGestures::update(bool pressed, uint32_t now) {
  // debounce: the level must stay the same for debounceMs before it counts
  if (pressed != raw_) { raw_ = pressed; rawSince_ = now; }
  if (raw_ != stable_ && now - rawSince_ >= cfg_.debounceMs) {
    stable_ = raw_;
    const uint32_t at = rawSince_;  // when it really changed
    if (stable_) onDown(at); else onUp(at);
  }
  // still pressed long enough = hold (once per press)
  if (stable_ && !holdSent_ && now - downAt_ >= cfg_.holdMs) {
    holdSent_ = true;
    taps_ = 0;
    push(Event::Hold);
  }
  // the number of taps is only known once the window for a next one has passed
  if (taps_ && !stable_ && now - tapUpAt_ > cfg_.doubleGapMs) flushTaps();
}

static Event tapsToEvent(uint8_t n) {
  return n <= 1 ? Event::Tap : n == 2 ? Event::DoubleTap : n == 3 ? Event::TripleTap : Event::QuadTap;
}

void TouchGestures::flushTaps() {
  const uint8_t n = taps_;
  taps_ = 0;
  if (n) push(tapsToEvent(n));
}

void TouchGestures::onDown(uint32_t now) {
  downAt_ = now;
  holdSent_ = false;
}

void TouchGestures::onUp(uint32_t now) {
  const uint32_t len = now - downAt_;
  if (holdSent_) return;                       // hold already sent
  if (len >= cfg_.longMs) { taps_ = 0; push(Event::LongPress); return; }
  if (len >= cfg_.tapMaxMs) { taps_ = 0; return; }  // too long for a tap, too short for long press
  const uint8_t most = cfg_.maxTaps < 2 ? 2 : cfg_.maxTaps > 4 ? 4 : cfg_.maxTaps;
  if (taps_ && downAt_ - tapUpAt_ <= cfg_.doubleGapMs) taps_++;
  else taps_ = 1;
  tapUpAt_ = now;
  if (taps_ >= most) flushTaps();   // the last count there is: no reason to wait
}

}  // namespace input
