// Wi-Fi time sync (ADR 010): turns Wi-Fi on only to ask a time server, then off again.
// Non-blocking: call loop() often. Credentials come from include/secrets.h (git-ignored).
#pragma once
#include <Arduino.h>
#include "config.h"
#include "../timekeeping/time_sync.h"

class WifiTime {
 public:
  enum class State : uint8_t { Idle, Connecting, Syncing, NoCredentials };

  void begin();               // set the time zone and schedule the first sync now
  void loop();
  void syncNow();
  bool timeValid() const;     // false until the first successful sync since power-up
  bool localTime(int& hour, int& minute, int& second) const;
  void printInfo(Print& out) const;
  State state() const { return state_; }

 private:
  void finish(bool ok, const char* why);
  timekeeping::SyncSchedule schedule_{(uint32_t)TIME_SYNC_INTERVAL_H * 3600UL * 1000UL};
  State state_ = State::Idle;
  uint32_t stepStart_ = 0;
  int64_t sysBefore_ = 0;      // system time when the sync started (for drift)
  uint32_t msBefore_ = 0;
  uint32_t syncs_ = 0, fails_ = 0;
  int32_t lastDriftS_ = 0;
  bool driftKnown_ = false;
  uint32_t lastSyncMs_ = 0;
};
