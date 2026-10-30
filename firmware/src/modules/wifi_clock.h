// Wi-Fi clock: the wall time for the brain. Syncs over Wi-Fi at start-up and once a day,
// with Wi-Fi off in between (drivers/wifi_time.*, ADR 010). Until the first sync the time is
// unknown: the clock shows --:-- and no alarm rings (never a wrong alarm).
#pragma once
#include "../app/module.h"
#include "../drivers/wifi_time.h"

class WifiClock : public Module, public TimeSource {
 public:
  const char* name() const override { return "time"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  bool onKey(App& app, char c) override;
  const char* keyHelp() const override { return "s = sync the time now"; }
  void printInfo(Print& out) const override;
  utility::WallTime wallTime() const override;

 private:
  WifiTime clock_;
};
