// IntroPlayer: the clock of the boot film. Pure C++ (no Arduino calls): it only decides WHICH step of a film is
// on screen at a given millis(); drawing is boot::renderStep() and the Screen module shows it.
//
// The schedule is cumulative (each step is due when the previous one's hold has passed, counted from begin()),
// so a slow frame never makes the whole film drift. If a frame is very late, stale steps are skipped, never
// replayed. skip() jumps to the last step (the live face), which then holds for its normal time.
#pragma once
#include <stdint.h>
#include "boot/boot_film.h"

namespace boot {

class IntroPlayer {
 public:
  void begin(const Film& f, uint32_t nowMs) {
    film_ = &f;
    step_ = 0;
    dueAt_ = nowMs + f.steps[0].ms;
    active_ = true;
    changed_ = true;  // step 0 is to be drawn now
  }
  bool active() const { return active_; }
  int step() const { return step_; }
  const Step& current() const { return film_->steps[step_]; }
  bool onLastStep() const { return film_ && step_ == film_->n - 1; }

  // Call every loop. Returns true when a new step must be drawn. active() turns false once the last hold is over.
  bool update(uint32_t nowMs) {
    if (!active_) return false;
    while ((int32_t)(nowMs - dueAt_) >= 0) {
      if (step_ + 1 >= film_->n) { active_ = false; return false; }
      step_++;
      dueAt_ += film_->steps[step_].ms;
      changed_ = true;
    }
    const bool c = changed_;
    changed_ = false;
    return c;
  }

  // A tap: jump to the last step and hold it for its own time.
  void skip(uint32_t nowMs) {
    if (!active_ || onLastStep()) return;
    step_ = film_->n - 1;
    dueAt_ = nowMs + film_->steps[step_].ms;
    changed_ = true;
  }

 private:
  const Film* film_ = nullptr;
  int step_ = 0;
  uint32_t dueAt_ = 0;
  bool active_ = false, changed_ = false;
};

}  // namespace boot
