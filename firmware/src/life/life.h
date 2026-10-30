// Dino's life: what the dino does on the home screen when nobody is using the menu or a timer.
// Pure C++ (no Arduino, no drawing), desktop-tested in test/test_life.
//
// The nine Life clips of the design sheet (group L) are the whole vocabulary. The brain only decides WHICH
// one plays (an Act, looked up in a table: clip key, frames, loop) and WHEN voices fire (a second table).
// Triggers (numbers live in Settings, filled from config.h):
//   touch -> pet; 4th touch within 8 s -> overpet, then 10 s of touches getting only a glance
//   every 2-4 min of idle -> rawr; no input for 5 min -> falls asleep (screen dims once asleep)
//   shake / pick up / tilt (IMU) -> the matching clip; any input while asleep -> wake
// Touch reactions start 120-300 ms after the touch (a pause that reads as noticing).
// The utility brain (alarm, menu, timers) always comes first: Life only plays on the home screen.
// No guilt: the dino never begs for attention.
#pragma once
#include <stdint.h>
#include "../sound/dino_sounds.h"

namespace life {

enum class Act : uint8_t {
  Idle,       // loop: the base (default)
  Glance,     // one frame of the idle clip: the only answer to a touch while over-petted
  Pet, Overpet, Rawr, Shake, PickUp, Tilt,
  FallAsleep, // the sleep clip once, then Asleep
  Asleep,     // loop: the asleep frames of the sleep clip, screen dimmed
  Wake,
  Count
};

// One row per Act: which clip, which of its frames, and whether it loops.
struct ActDef {
  const char* name;
  const char* clip;       // key in scenes::DINO_CLIP_KEYS
  uint8_t first, last;    // frames first..last of that clip (inclusive)
  bool loop;
};
const ActDef& actDef(Act a);
inline const char* actName(Act a) { return actDef(a).name; }

// A voice fires when frame `frame` (index in the full clip) of `act` is shown.
struct Cue { Act act; uint8_t frame; sound::Voice voice; };
const Cue* cues(int& count);

struct Settings {
  uint32_t sleepAfterMs = 300000;                    // no input this long -> falls asleep
  uint32_t rawrMinMs = 120000, rawrMaxMs = 240000;   // idle quirk
  uint8_t overpetTouches = 4;                        // this many touches ...
  uint32_t overpetWindowMs = 8000;                   // ... within this window = over-petted
  uint32_t overpetLockoutMs = 10000;                 // then touches only get a glance for this long
  uint16_t reactMinMs = 120, reactMaxMs = 300;       // pause before reacting to a touch
  uint32_t tiltCooldownMs = 4000;                    // the tilt clip does not repeat faster than this
};

struct View {
  Act act = Act::Idle;
  bool loop = true;        // false = one-shot: the screen calls onClipDone() when it has played
  uint32_t serial = 0;     // changes whenever a new clip must start
  bool dim = false;        // screen at night brightness (asleep)
};

class Brain {
 public:
  void configure(const Settings& s) { s_ = s; }
  void seed(uint32_t s) { rng_ = s ? s : 1; }
  void tick(uint32_t nowMs);                 // call every loop
  void onTouch(uint32_t nowMs);              // a tap while on the home screen
  void onFirmDoubleTap(uint32_t nowMs);      // rawr on demand (not wired: double tap = time on the home screen)
  void onShake(uint32_t nowMs);
  void onPickUp(uint32_t nowMs);
  void onTilt(uint32_t nowMs);
  void onActivity(uint32_t nowMs);           // any other input, or input away from the home screen
  void onClipDone(uint32_t nowMs);           // a one-shot clip finished
  void onAlarmDismissed(uint32_t nowMs);     // the dino was woken by the alarm
  void onHomeResumed(uint32_t nowMs);        // back from a utility screen: start again from the base

  const View& view() const { return v_; }
  bool asleep() const { return asleep_; }
  bool busy() const { return !v_.loop || pending_; }
  void nextNow(uint32_t nowMs) { rawrAt_ = nowMs; }       // test: rawr now
  void playNow(Act a, uint32_t nowMs);                    // test: play this act now (when awake)
  uint32_t msToRawr(uint32_t nowMs) const { return (int32_t)(rawrAt_ - nowMs) > 0 ? rawrAt_ - nowMs : 0; }
  uint32_t msToSleep(uint32_t nowMs) const;

 private:
  Settings s_;
  uint32_t rng_ = 0x9E3779B9u;
  View v_;
  bool started_ = false;
  bool asleep_ = false;
  uint32_t lastInputAt_ = 0, rawrAt_ = 0;
  uint32_t touches_[8] = {};  // times of the latest touches (ring)
  uint8_t nTouches_ = 0;
  uint32_t lockoutUntil_ = 0;
  uint32_t tiltOkAt_ = 0;
  bool overpetPending_ = false;  // 4th touch landed while another clip played
  bool pending_ = false;         // a reaction waits for its pause to end
  Act pendingAct_ = Act::Idle;
  uint32_t pendingAt_ = 0;

  uint32_t rand();
  uint32_t range(uint32_t lo, uint32_t hi) { return hi > lo ? lo + rand() % (hi - lo + 1) : lo; }
  void show(Act a);
  void later(Act a, uint32_t now);        // show after the reaction pause
  void touched(uint32_t now);             // input bookkeeping (resets the sleep timer)
  bool wakeIfAsleep(uint32_t now, bool pause);
  void scheduleRawr(uint32_t now) { rawrAt_ = now + range(s_.rawrMinMs, s_.rawrMaxMs); }
  void base(uint32_t now);
  bool atBase() const { return v_.loop && v_.act == Act::Idle && !pending_; }
};

}  // namespace life
