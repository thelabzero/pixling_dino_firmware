#include "tone_player.h"

namespace sound {

void TonePlayer::start(const Pattern& p, uint32_t now) {
  p_ = p;
  playing_ = p.n > 0;
  step_ = 0; round_ = 0; inGap_ = false;
  stepAt_ = now;
  update(now);
}

void TonePlayer::update(uint32_t now) {
  freq_ = 0; vol_ = 0;
  // Move on past finished steps (a slow loop can skip several short ones at once).
  while (playing_) {
    const uint32_t el = now - stepAt_;
    if (inGap_) {
      if (el < p_.loopGapMs) return;               // silent gap between alarm rounds
      inGap_ = false; stepAt_ += p_.loopGapMs; step_ = 0; round_ = 0;
      continue;
    }
    const Step& s = p_.steps[step_];
    if (el < s.ms) {
      if (s.f0 == 0) return;                       // rest
      const float t = (float)el / (float)s.ms;     // 0..1 through the glide
      freq_ = (uint16_t)((float)s.f0 + ((float)s.f1 - (float)s.f0) * t + 0.5f);
      vol_ = s.vol;
      return;
    }
    stepAt_ += s.ms;
    if (++step_ < p_.n) continue;
    step_ = 0;
    if (++round_ < p_.repeat) continue;
    if (p_.loopGapMs) { inGap_ = true; continue; }
    playing_ = false;
  }
}

}  // namespace sound
