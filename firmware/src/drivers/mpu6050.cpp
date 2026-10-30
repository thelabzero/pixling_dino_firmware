#include <Arduino.h>
#include "mpu6050.h"

static const uint8_t REG_CONFIG = 0x1A, REG_GYRO_CFG = 0x1B, REG_ACCEL_CFG = 0x1C;
static const uint8_t REG_ACCEL_XOUT_H = 0x3B, REG_PWR_MGMT_1 = 0x6B, REG_WHO_AM_I = 0x75;
static const float ACCEL_LSB_PER_G = 8192.0f;   // +-4 g
static const float GYRO_LSB_PER_DPS = 65.5f;    // +-500 deg/s

bool Mpu6050::write(uint8_t reg, uint8_t val) {
  w_->beginTransmission(addr_);
  w_->write(reg);
  w_->write(val);
  return w_->endTransmission() == 0;
}

bool Mpu6050::begin(TwoWire& w, uint8_t addr) {
  w_ = &w; addr_ = addr;
  if (!write(REG_PWR_MGMT_1, 0x01)) return false;  // wake, clock from gyro X PLL
  delay(10);
  write(REG_CONFIG, 0x03);       // DLPF ~44 Hz
  write(REG_GYRO_CFG, 0x08);     // +-500 deg/s
  write(REG_ACCEL_CFG, 0x08);    // +-4 g
  return true;
}

uint8_t Mpu6050::whoAmI() {
  w_->beginTransmission(addr_);
  w_->write(REG_WHO_AM_I);
  if (w_->endTransmission(false) != 0) return 0;
  if (w_->requestFrom(addr_, (uint8_t)1) != 1) return 0;
  return w_->read();
}

bool Mpu6050::read(input::Vec3& a, input::Vec3& g) {
  w_->beginTransmission(addr_);
  w_->write(REG_ACCEL_XOUT_H);
  if (w_->endTransmission(false) != 0) return false;
  if (w_->requestFrom(addr_, (uint8_t)14) != 14) return false;
  int16_t v[7];
  for (int i = 0; i < 7; i++) { int hi = w_->read(); int lo = w_->read(); v[i] = (int16_t)((hi << 8) | lo); }
  a = {v[0] / ACCEL_LSB_PER_G, v[1] / ACCEL_LSB_PER_G, v[2] / ACCEL_LSB_PER_G};
  // v[3] is temperature
  g = {v[4] / GYRO_LSB_PER_DPS, v[5] / GYRO_LSB_PER_DPS, v[6] / GYRO_LSB_PER_DPS};
  return true;
}
