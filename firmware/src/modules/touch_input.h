// Touch input: the HW-763 / TTP223 pad on PIN_TOUCH -> tap, double tap, long press, hold.
// The gesture timing lives in input/touch.* (pure, tested); this module only reads the pin.
// The Serial keys t d l h keep working alongside it.
#pragma once
#include "../app/module.h"
#include "../input/touch.h"

class TouchInput : public Module {
 public:
  const char* name() const override { return "touch"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  void printInfo(Print& out) const override;

 private:
  input::TouchGestures gestures_;
  bool lastPressed_ = false;
  uint32_t presses_ = 0, downAt_ = 0;
  bool read() const;
};
