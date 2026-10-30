// Tone player: plays a Pattern (a list of pitch glides and rests) over time. Pure C++
// (no Arduino): call update(now) every loop and send freq()/volume() to the buzzer.
// Desktop-tested in test/test_sound; tools/sound_preview renders the same patterns to .wav.
#pragma once
#include <stdint.h>

namespace sound {

// One step: glide from f0 to f1 Hz over ms, at volume 0-100. f0 = 0 means silence (a rest).
struct Step {
  uint16_t f0, f1;
  uint16_t ms;
  uint8_t vol;
};

struct Pattern {
  const Step* steps;
  uint8_t n;          // number of steps
  uint8_t repeat;     // play the steps this many times (1 = once)
  uint16_t loopGapMs; // > 0: loop forever with this rest between rounds (alarm), until stop()
};

class TonePlayer {
 public:
  void start(const Pattern& p, uint32_t nowMs);
  void stop() { playing_ = false; freq_ = 0; vol_ = 0; }
  void update(uint32_t nowMs);
  bool playing() const { return playing_; }
  bool looping() const { return playing_ && p_.loopGapMs > 0; }
  uint16_t freq() const { return freq_; }   // 0 = silent
  uint8_t volume() const { return vol_; }   // 0-100

 private:
  Pattern p_ = {nullptr, 0, 1, 0};
  bool playing_ = false;
  uint32_t stepAt_ = 0;   // when the current step started
  uint8_t step_ = 0, round_ = 0;
  bool inGap_ = false;
  uint16_t freq_ = 0;
  uint8_t vol_ = 0;
};

}  // namespace sound
