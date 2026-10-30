// Battery level from the cell voltage. Pure C++ (no Arduino), desktop-tested in test/test_battery.
#pragma once
#include <stdint.h>

namespace power {

// Voltage at the ADC pin (mV) -> battery voltage (mV), for a resistor divider
// Battery+ -> rTop -> pin -> rBottom -> GND, with a calibration factor (1.0 = none).
inline uint32_t batteryMv(uint32_t pinMv, float rTopK, float rBottomK, float cal) {
  return (uint32_t)((float)pinMv * (rTopK + rBottomK) / rBottomK * cal + 0.5f);
}

// Single-cell Li-ion / LiPo at rest -> 0..100 %. A typical curve, not this cell's:
// treat it as an estimate. Under load or while charging the voltage reads higher or lower.
int lipoPercent(uint32_t mv);

// Exponential smoothing for noisy readings: weight = share of the new reading (0..1).
class Smoother {
 public:
  explicit Smoother(float weight = 0.2f) : w_(weight) {}
  float add(float v) { v_ = have_ ? v_ + w_ * (v - v_) : v; have_ = true; return v_; }
  float value() const { return v_; }
  bool ready() const { return have_; }
 private:
  float w_, v_ = 0;
  bool have_ = false;
};

}  // namespace power
