// Power decisions, pure C++ (no Arduino), desktop-tested in test/test_power. Nothing here touches hardware:
// the Screen and Battery modules ask these questions and do the work.
//   - what the screen should do (full / dimmed while idle / night dim / off) from how long nobody touched the dino
//     and how long it has been asleep;
//   - the battery percentage the icon shows (never creeps up while on battery: a loud sound or a screen burst
//     makes the voltage sag and recover, and the number must not bounce);
//   - when the battery is empty enough to shut the dino down and protect the cell.
#pragma once
#include <stdint.h>

namespace power {

enum class ScreenMode : uint8_t { Full, Idle, Night, Off };

struct ScreenConfig {
  uint32_t dimAfterMs = 60000;      // awake on the home screen, nobody touched it: dim
  uint32_t offAfterMs = 30000;      // asleep this long: screen off
};

struct ScreenInput {
  bool home = true;          // the home screen is up (alarms, menu, focus... are never dimmed or switched off)
  bool asleep = false;       // the dino is asleep (home screen only)
  uint32_t idleMs = 0;       // since the last touch / gesture
  uint32_t asleepMs = 0;     // since it fell asleep
  bool saver = false;        // battery is low: the awake home screen runs dimmed
};

ScreenMode decideScreen(const ScreenConfig& c, const ScreenInput& in);

// Battery percentage for the icon. On battery it only goes down (a real recovery of more than 5 points, e.g. a
// charged cell, is accepted). Seeing USB voltage resets it: the percent after a charge is read fresh.
class PercentFilter {
 public:
  int update(int percent, bool usb);
  int value() const { return shown_; }   // -1 = no reading yet
 private:
  int shown_ = -1;
  bool wasUsb_ = false;
};

// Empty-battery shutdown: true once the battery has stayed below cutoffMv for holdMs. USB voltage, no battery,
// and cutoffMv = 0 (switched off) never trigger it. A reading back above the cutoff restarts the count.
class CutoffWatch {
 public:
  CutoffWatch(uint32_t cutoffMv, uint32_t holdMs, uint32_t missingMv) : cut_(cutoffMv), hold_(holdMs), missing_(missingMv) {}
  bool update(uint32_t mv, bool usb, uint32_t nowMs);
 private:
  uint32_t cut_, hold_, missing_;
  uint32_t since_ = 0;
  bool below_ = false;
};

}  // namespace power
