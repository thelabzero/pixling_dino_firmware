// Battery: reads the cell voltage through the divider on PIN_BATTERY_ADC and estimates the
// charge left. Prints it every BATTERY_LOG_S, and with the "b" key. (Not on screen yet:
// that needs a battery icon in the design.)
#pragma once
#include "../app/module.h"
#include "../power/battery_level.h"

class Battery : public Module {
 public:
  const char* name() const override { return "battery"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  bool onKey(App& app, char c) override;
  const char* keyHelp() const override { return "b = battery now"; }
  void printInfo(Print& out) const override;

  uint32_t millivolts() const { return (uint32_t)avg_.value(); }
  int percent() const { return power::lipoPercent(millivolts()); }

 private:
  power::Smoother avg_{0.2f};
  uint32_t nextRead_ = 0, nextLog_ = 0;
  uint32_t lastPinMv_ = 0;
  uint32_t readOnce();
};
