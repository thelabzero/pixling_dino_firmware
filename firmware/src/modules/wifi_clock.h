// Wi-Fi clock: the wall time for the brain. Syncs over Wi-Fi at start-up and once a day,
// with Wi-Fi off in between (drivers/wifi_time.*, ADR 010). Until the first sync the time is
// unknown: the clock shows --:-- and no alarm rings (never a wrong alarm).
//
// Also runs "connect your dino" (ADR 005): when the brain asks (menu > Wi-Fi) it opens dino's own temporary network and
// setup page, tells the brain what is happening, and on success saves the credentials and syncs the time. It lives here
// because it owns the same radio; the pairing rules are in wifi/wifi_setup.* (pure, tested).
#pragma once
#include "../app/module.h"
#include "../drivers/wifi_portal.h"
#include "../drivers/wifi_time.h"
#include "../wifi/wifi_setup.h"

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
  void startSetup(App& app, uint32_t now);
  void endSetup();
  void runSetup(App& app, uint32_t now);

  WifiTime clock_;
  WifiPortal portal_;
  wifisetup::Session session_;
  bool setup_ = false;        // the setup network is up (or lingering after a success)
  uint32_t closeAtMs_ = 0;    // after a success: take the network down at this time (0 = not scheduled)
};
