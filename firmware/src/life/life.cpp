#include "life.h"

namespace life {

// Data, not code: the sheet's Life clips and the frames each Act plays (frame numbers = index in the clip).
static const ActDef DEFS[] = {
    {"idle", "idle", 0, 13, true},
    {"glance", "idle", 5, 5, false},   // "glance right"
    {"pet", "pet", 0, 5, false},
    {"overpet", "overpet", 0, 5, false},
    {"rawr", "rawr", 0, 8, false},
    {"shake", "shake", 0, 11, false},
    {"pickup", "pickup", 0, 5, false},
    {"tilt", "tilt", 0, 4, false},
    {"fall_asleep", "sleep", 0, 4, false},
    {"asleep", "sleep", 3, 4, true},   // the two asleep frames loop
    {"wake", "wake", 0, 7, false},
};
static_assert(sizeof(DEFS) / sizeof(DEFS[0]) == (int)Act::Count, "one row per Act");

const ActDef& actDef(Act a) { return DEFS[a < Act::Count ? (int)a : 0]; }

using sound::Voice;
static const Cue CUES[] = {
    {Act::Pet, 1, Voice::Purr},        // shy
    {Act::Pet, 3, Voice::Trill},       // grin
    {Act::Overpet, 1, Voice::Grumble},
    {Act::Rawr, 2, Voice::Growl},      // the RAWR frame
    {Act::Rawr, 5, Voice::Giggle},
    {Act::Shake, 0, Voice::Yelp},      // jolt
    {Act::Shake, 2, Voice::Dizzy},     // spinning pupils
    {Act::PickUp, 0, Voice::Squeak},   // lifted
    {Act::FallAsleep, 0, Voice::Sigh},
};
const Cue* cues(int& count) { count = sizeof(CUES) / sizeof(CUES[0]); return CUES; }

uint32_t Brain::rand() {  // xorshift32
  rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
  return rng_;
}

void Brain::show(Act a) {
  v_.act = a; v_.loop = actDef(a).loop; v_.serial++;
  v_.dim = a == Act::Asleep;
  pending_ = false;
}

void Brain::later(Act a, uint32_t now) {
  pending_ = true; pendingAct_ = a;
  pendingAt_ = now + range(s_.reactMinMs, s_.reactMaxMs);
}

void Brain::base(uint32_t now) {
  show(Act::Idle);
  scheduleRawr(now);
}

uint32_t Brain::msToSleep(uint32_t now) const {
  const uint32_t due = lastInputAt_ + s_.sleepAfterMs;
  return (int32_t)(due - now) > 0 ? due - now : 0;
}

void Brain::tick(uint32_t now) {
  if (!started_) { started_ = true; lastInputAt_ = now; scheduleRawr(now); }
  if (pending_ && (int32_t)(now - pendingAt_) >= 0) show(pendingAct_);
  if (!atBase() || asleep_) return;
  if ((int32_t)(now - lastInputAt_) >= (int32_t)s_.sleepAfterMs) { show(Act::FallAsleep); return; }
  if ((int32_t)(now - rawrAt_) >= 0) { show(Act::Rawr); scheduleRawr(now); }
}

void Brain::touched(uint32_t now) { lastInputAt_ = now; }

// Any input while asleep wakes the dino (the wake clip). Touches get the reaction pause.
bool Brain::wakeIfAsleep(uint32_t now, bool pause) {
  if (!asleep_) return false;
  asleep_ = false;
  if (pause) later(Act::Wake, now); else show(Act::Wake);
  return true;
}

void Brain::onActivity(uint32_t now) {
  touched(now);
  wakeIfAsleep(now, false);
}

void Brain::onHomeResumed(uint32_t now) {
  touched(now);
  asleep_ = false; overpetPending_ = false;
  base(now);
}

void Brain::onClipDone(uint32_t now) {
  if (v_.loop || pending_) return;
  switch (v_.act) {
    case Act::FallAsleep: asleep_ = true; show(Act::Asleep); break;
    case Act::Wake: touched(now); base(now); break;
    default:
      if (overpetPending_) { overpetPending_ = false; show(Act::Overpet); break; }
      base(now);
      break;
  }
}

void Brain::onTouch(uint32_t now) {
  touched(now);
  if (wakeIfAsleep(now, true)) return;
  // Count every touch, even while a clip plays.
  if (nTouches_ == sizeof(touches_) / sizeof(touches_[0])) {
    for (uint8_t i = 1; i < nTouches_; i++) touches_[i - 1] = touches_[i];
    nTouches_--;
  }
  touches_[nTouches_++] = now;
  if ((int32_t)(lockoutUntil_ - now) > 0) {  // over-petted: it ignores touch, only a glance
    if (atBase()) later(Act::Glance, now);
    return;
  }
  uint8_t inWindow = 0;
  for (uint8_t i = 0; i < nTouches_; i++) if (now - touches_[i] <= s_.overpetWindowMs) inWindow++;
  if (inWindow >= s_.overpetTouches) {
    nTouches_ = 0;
    lockoutUntil_ = now + s_.overpetLockoutMs;
    if (atBase()) later(Act::Overpet, now); else overpetPending_ = true;
    return;
  }
  if (atBase()) later(Act::Pet, now);
}

// Rawr on demand.
void Brain::onFirmDoubleTap(uint32_t now) {
  touched(now);
  if (wakeIfAsleep(now, true)) return;
  if (atBase()) { show(Act::Rawr); scheduleRawr(now); }
}

void Brain::onShake(uint32_t now) {
  touched(now);
  if (wakeIfAsleep(now, false)) return;
  show(Act::Shake);  // a jolt interrupts whatever else plays
  overpetPending_ = false;
}

void Brain::onPickUp(uint32_t now) {
  touched(now);
  if (wakeIfAsleep(now, false)) return;
  if (v_.act == Act::Shake && !v_.loop) return;  // being shaken already
  show(Act::PickUp);
  overpetPending_ = false;
}

void Brain::onTilt(uint32_t now) {
  touched(now);
  if (wakeIfAsleep(now, false)) return;
  if (!atBase() || (int32_t)(tiltOkAt_ - now) > 0) return;
  tiltOkAt_ = now + s_.tiltCooldownMs;
  show(Act::Tilt);
}

void Brain::onAlarmDismissed(uint32_t now) {
  touched(now);
  asleep_ = false;
  show(Act::Wake);
}

void Brain::playNow(Act a, uint32_t now) {
  if (asleep_ || !atBase()) return;
  touched(now);
  show(a);
}

}  // namespace life
