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
    tapPending_ = false;
    push(Event::Hold);
  }
  // a single tap is only known once the double-tap window has passed
  if (tapPending_ && !stable_ && now - tapUpAt_ > cfg_.doubleGapMs) {
    tapPending_ = false;
    push(Event::Tap);
  }
}

void TouchGestures::onDown(uint32_t now) {
  downAt_ = now;
  holdSent_ = false;
}

void TouchGestures::onUp(uint32_t now) {
  const uint32_t len = now - downAt_;
  if (holdSent_) return;                       // hold already sent
  if (len >= cfg_.longMs) { tapPending_ = false; push(Event::LongPress); return; }
  if (len >= cfg_.tapMaxMs) { tapPending_ = false; return; }  // too long for a tap, too short for long press
  if (tapPending_ && downAt_ - tapUpAt_ <= cfg_.doubleGapMs) {
    tapPending_ = false;
    push(Event::DoubleTap);
    return;
  }
  tapPending_ = true;
  tapUpAt_ = now;
}

}  // namespace input
