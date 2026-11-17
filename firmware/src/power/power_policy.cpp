#include "power_policy.h"

namespace power {

ScreenMode decideScreen(const ScreenConfig& c, const ScreenInput& in) {
  if (!in.home) return ScreenMode::Full;  // utility screens and alarms: always full brightness, never off
  if (in.asleep) return (c.offAfterMs && in.asleepMs >= c.offAfterMs) ? ScreenMode::Off : ScreenMode::Night;
  if (in.saver || (c.dimAfterMs && in.idleMs >= c.dimAfterMs)) return ScreenMode::Idle;
  return ScreenMode::Full;
}

int PercentFilter::update(int p, bool usb) {
  if (usb) { wasUsb_ = true; return shown_; }  // no battery reading while USB feeds the divider: keep the last one
  if (shown_ < 0 || wasUsb_) shown_ = p;       // first reading, or the first one after unplugging
  else if (p < shown_) shown_ = p;
  else if (p > shown_ + 5) shown_ = p;
  wasUsb_ = false;
  return shown_;
}

bool CutoffWatch::update(uint32_t mv, bool usb, uint32_t now) {
  if (!cut_ || usb || mv < missing_ || mv >= cut_) { below_ = false; return false; }
  if (!below_) { below_ = true; since_ = now; }
  return (int32_t)(now - since_) >= (int32_t)hold_;
}

}  // namespace power
