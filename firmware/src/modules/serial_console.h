// Serial console: one-letter keys typed in the Serial monitor.
//   t tap   d double tap   l long press   h hold      <- touch, until the TTP223 is wired
//   ,  tilt left   .  tilt right   f flip down   u flip up   <- when the IMU is not handy
//   a  test: alarm at the next minute    i  info    ?  list all keys
// Any other key is offered to the other modules (onKey), e.g. "s" = sync time.
#pragma once
#include "../app/module.h"

class SerialConsole : public Module {
 public:
  const char* name() const override { return "console"; }
  bool begin(App&) override { return true; }
  void loop(App& app, uint32_t now) override;
  const char* keyHelp() const override {
    return "t d l h = tap / double / long / hold, , . f u = tilt / flip, a = test alarm, i = info, ? = keys";
  }

 private:
  void testAlarm(App& app);
};
