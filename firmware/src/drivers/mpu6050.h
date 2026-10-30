// Minimal MPU6050 driver (no library): wake, set ranges, read accel + gyro.
// Range: accel +-4 g, gyro +-500 deg/s, on-chip low-pass ~44 Hz.
#pragma once
#include <Wire.h>
#include "../input/imu_gestures.h"

class Mpu6050 {
 public:
  // Returns false if nothing answers at addr.
  bool begin(TwoWire& w, uint8_t addr);
  // WHO_AM_I register. Genuine MPU6050 = 0x68; clones and MPU6500/9250 report other values.
  uint8_t whoAmI();
  bool read(input::Vec3& accel_g, input::Vec3& gyro_dps);
  uint8_t address() const { return addr_; }

 private:
  TwoWire* w_ = nullptr;
  uint8_t addr_ = 0x68;
  bool write(uint8_t reg, uint8_t val);
};
