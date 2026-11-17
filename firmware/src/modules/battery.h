// Battery: reads the cell voltage through the divider on PIN_BATTERY_ADC and estimates the
// charge left. Prints it every BATTERY_LOG_S, and with the "b" key. The Screen draws it as an icon on the home
// screen (BatteryInfo). When the cell stays below BATTERY_CUTOFF_MV it logs, switches the panel and the IMU off and
// goes into deep sleep to protect the cell; it wakes by itself when USB is plugged in or the voltage recovers
// (guardAtBoot(), called first in setup()).
#pragma once
#include "config.h"
#include "../app/module.h"
#include "../power/battery_level.h"
#include "../power/power_policy.h"

class Battery : public Module, public BatteryInfo {
 public:
  Battery();
  const char* name() const override { return "battery"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  bool onKey(App& app, char c) override;
  const char* keyHelp() const override { return "b = battery now"; }
  void printInfo(Print& out) const override;
  void soakFill(soak::Record& r) const override;

  uint32_t millivolts() const { return (uint32_t)avg_.value(); }
  int rawPercent() const { return power::lipoPercent(millivolts()); }   // straight from the voltage

  // BatteryInfo
  bool present() const override { const uint32_t mv = millivolts(); return mv >= BATTERY_MISSING_MV && mv <= BATTERY_MAX_MV; }
  bool usb() const override { return millivolts() > BATTERY_MAX_MV; }
  int percent() const override { return pct_.value() < 0 ? rawPercent() : pct_.value(); }

  // First thing in setup(): after a low-battery deep sleep, decide between waking up and sleeping again.
  static void guardAtBoot();
 private:
  power::Smoother avg_{0.2f};
  power::PercentFilter pct_;
  power::CutoffWatch cut_;
  void shutdown(App& app);
  uint32_t nextRead_ = 0, nextLog_ = 0;
  uint32_t lastPinMv_ = 0;
  uint32_t readOnce();
};
