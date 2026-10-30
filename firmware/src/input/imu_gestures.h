// IMU gestures from MPU6050 samples: tilt left/right, flip face-down/up, pick up,
// put down, shake. Pure C++ (no Arduino), unit-tested in env:native.
//
// Orientation is relative to how the IMU sits in the enclosure, given by two unit
// vectors in sensor axes: `up` (what the accelerometer reads when the dino stands
// normally) and `front` (out of the screen, towards the user). play_inputs has a
// Serial calibration (c, then f) that measures both and prints config lines.
#pragma once
#include <stdint.h>
#include "events.h"

namespace input {

struct Vec3 { float x, y, z; };

struct ImuSample {
  Vec3 accel_g;    // g
  Vec3 gyro_dps;   // degrees per second
  uint32_t t_ms;
};

// Thresholds are starting values (interaction-spec.md: [Open] until tested in the enclosure).
struct GestureConfig {
  Vec3 up{0, 0, 1};
  Vec3 front{0, -1, 0};
  // tilt (roll around the front axis)
  float tiltOnDeg = 25, tiltOffDeg = 12;
  uint16_t tiltHoldMs = 120;     // must stay tilted this long
  uint16_t tiltRepeatMs = 700;   // repeats while held (0 = never)
  // flip
  float faceDownDot = -0.75f;    // accel . front below this = screen facing the desk
  float faceUpDot = -0.45f;      // back above this = flipped back
  uint16_t flipHoldMs = 1000;    // spec: held 1 s
  // still / pick up / put down
  float stillGyroDps = 8, stillAccelG = 0.06f;
  uint16_t restMs = 800;         // still this long = resting on the desk
  float liftAccelG = 0.18f;      // |accel| - 1 g jump that starts a pick-up
  uint16_t liftConfirmMs = 250;  // still moving this long after the jump = picked up
  uint16_t putDownMs = 1000;     // still this long while held = put down
  // shake
  float shakePeakG = 1.7f;       // |accel| peak
  uint8_t shakePeaks = 3;        // peaks within the window
  uint16_t shakeWindowMs = 1000, shakeCooldownMs = 1500;
};

struct ImuState {
  float rollDeg = 0, pitchDeg = 0, accelMag = 1;
  bool still = false, resting = false, held = false, faceDown = false;
};

class ImuGestures {
 public:
  explicit ImuGestures(const GestureConfig& c = GestureConfig()) { setConfig(c); }
  void setConfig(const GestureConfig& c);
  const GestureConfig& config() const { return cfg_; }
  // Feed one sample (every ~20 ms), then read events with next() until it returns None.
  void update(const ImuSample& s);
  Event next();
  const ImuState& state() const { return st_; }

  // Calibration helpers: turn averaged accel readings into unit vectors.
  // front = -(face-down reading), made perpendicular to up.
  static Vec3 unit(Vec3 v);
  static Vec3 frontFromFaceDown(Vec3 up, Vec3 faceDownReading);

 private:
  GestureConfig cfg_;
  Vec3 right_{1, 0, 0};
  ImuState st_;
  // timers (0 = not running)
  uint32_t stillSince_ = 0, tiltSince_ = 0, tiltLastFire_ = 0, downSince_ = 0, upSince_ = 0;
  uint32_t liftAt_ = 0, shakeStart_ = 0, shakeCooldownUntil_ = 0;
  int8_t tiltDir_ = 0;        // -1 left, +1 right, 0 none
  bool tiltLatched_ = false;
  bool aboveShake_ = false;
  uint8_t shakeCount_ = 0;
  bool started_ = false;
  Event queue_[4] = {};
  uint8_t qHead_ = 0, qLen_ = 0;
  void push(Event e);
};

}  // namespace input
