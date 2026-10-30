#include "imu_input.h"
#include <Wire.h>
#include "config.h"
#include "../app/app.h"

using input::Event;

// I2C addresses to try: 0x68, or 0x69 if the module's AD0 pin is high.
static const uint8_t kAddrs[] = {0x68, 0x69};
// Gestures passed on to the brain.
static const Event kSend[] = {Event::TiltLeft, Event::TiltRight, Event::FlipDown, Event::FlipUp, Event::Shake, Event::PickUp};  // shake + pick-up: the dino reacts (life brain)

bool ImuInput::begin(App&) {
  for (uint8_t addr : kAddrs) {
    if (!imu_.begin(Wire, addr)) continue;

    input::ImuCalibration c;
    c.accOffsetG = {IMU_ACC_OFFSET_X, IMU_ACC_OFFSET_Y, IMU_ACC_OFFSET_Z};
    c.accScale = {IMU_ACC_SCALE_X, IMU_ACC_SCALE_Y, IMU_ACC_SCALE_Z};
    c.gyroBiasDps = {IMU_GYRO_BIAS_X, IMU_GYRO_BIAS_Y, IMU_GYRO_BIAS_Z};
    corrector_ = input::ImuCorrector(c);

    input::GestureConfig g;
    g.up = {IMU_UP_X, IMU_UP_Y, IMU_UP_Z};
    g.front = {IMU_FRONT_X, IMU_FRONT_Y, IMU_FRONT_Z};
    g.tiltOnDeg = GESTURE_TILT_ON_DEG;
    g.tiltOffDeg = GESTURE_TILT_OFF_DEG;
    g.tiltHoldMs = GESTURE_TILT_HOLD_MS;
    g.tiltRepeatMs = GESTURE_TILT_REPEAT_MS;
    g.flipHoldMs = GESTURE_FLIP_HOLD_MS;
    g.shakePeakG = GESTURE_SHAKE_PEAK_G;
    g.shakePeaks = GESTURE_SHAKE_PEAKS;
    gestures_.setConfig(g);

    Serial.printf("[imu] MPU6050 at 0x%02X\n", addr);
    return true;
  }
  return false;
}

void ImuInput::loop(App& app, uint32_t now) {
  if ((int32_t)(now - nextSample_) < 0) return;
  nextSample_ = now + IMU_SAMPLE_MS;
  input::Vec3 a, g;
  if (!imu_.read(a, g)) { readErrors_++; return; }
  corrector_.apply(a, g, now);
  gestures_.update({a, g, now});
  for (Event e; (e = gestures_.next()) != Event::None;)
    for (Event s : kSend)
      if (e == s) app.emit(e);
}

void ImuInput::printInfo(Print& out) const {
  const input::ImuState& s = gestures_.state();
  out.printf("[imu] roll %.0f deg, pitch %.0f deg, %s%s, gyro bias learning %s, read errors %lu\n", s.rollDeg,
             s.pitchDeg, s.faceDown ? "face-down" : "face-up", s.resting ? ", resting" : "",
             corrector_.learning() ? "yes" : "no", (unsigned long)readErrors_);
}
