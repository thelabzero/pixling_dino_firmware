// When to sync the clock, and how the time is shown. Pure C++ (no Arduino), tested in env:native.
// The Wi-Fi/SNTP side lives in drivers/wifi_time.*.
#pragma once
#include <stdint.h>

namespace timekeeping {

// Decides when the next sync attempt is due: at boot, then every interval after a success,
// and with growing back-off after failures (1, 5, 15, 60 min, then every 60 min).
class SyncSchedule {
 public:
  explicit SyncSchedule(uint32_t intervalMs = 24UL * 3600UL * 1000UL) : intervalMs_(intervalMs) {}
  void start(uint32_t now) { nextAt_ = now; failures_ = 0; }
  bool due(uint32_t now) const { return (int32_t)(now - nextAt_) >= 0; }
  void onSuccess(uint32_t now) { failures_ = 0; nextAt_ = now + intervalMs_; }
  void onFailure(uint32_t now);
  void syncNow(uint32_t now) { nextAt_ = now; }
  uint32_t msUntilNext(uint32_t now) const { return due(now) ? 0 : nextAt_ - now; }
  uint8_t failures() const { return failures_; }

 private:
  uint32_t intervalMs_;
  uint32_t nextAt_ = 0;
  uint8_t failures_ = 0;
};

// "HH:MM" into out[6]; "--:--" when the time is not known yet (never show a wrong time).
void formatHHMM(bool valid, int hour, int minute, char out[6]);

// The system clock counts as set once it is past 2024-01-01 (it starts at 1970 after power-up).
inline bool epochLooksValid(int64_t epochSeconds) { return epochSeconds > 1704067200LL; }

}  // namespace timekeeping
