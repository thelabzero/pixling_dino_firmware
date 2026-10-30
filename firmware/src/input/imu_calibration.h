// IMU calibration: fixed accel offset/scale + gyro bias that keeps itself up to date.
// Values come from the HTML calibrator (tools/imu_calibrator/) and live in config.h.
// Pure C++ (no Arduino), unit-tested in env:native.
#pragma once
#include <stdint.h>
#include "imu_gestures.h"  // Vec3

namespace input {

struct ImuCalibration {
  Vec3 accOffsetG{0, 0, 0};   // subtract (g)
  Vec3 accScale{1, 1, 1};     // then divide
  Vec3 gyroBiasDps{0, 0, 0};  // subtract (deg/s); starting value, tracked at run time
};

// Gyro bias drifts with temperature, so it is re-learned whenever the device is still.
struct BiasTracking {
  float stillGyroDps = 3.0f;   // corrected gyro below this...
  float stillAccelG = 0.03f;   // ...and |accel| within this of 1 g...
  uint16_t stillMs = 1500;     // ...for this long = still enough to learn
  float rate = 0.01f;          // share of the error corrected per sample (~2 s time constant at 50 Hz)
};

class ImuCorrector {
 public:
  explicit ImuCorrector(const ImuCalibration& c = ImuCalibration(), const BiasTracking& t = BiasTracking())
      : cal_(c), trk_(t) {}
  // a in g, g in deg/s: corrected in place. Call once per sample.
  void apply(Vec3& a, Vec3& g, uint32_t t_ms);
  Vec3 gyroBias() const { return cal_.gyroBiasDps; }
  bool learning() const { return learning_; }

 private:
  ImuCalibration cal_;
  BiasTracking trk_;
  uint32_t stillSince_ = 0;
  bool learning_ = false;
};

}  // namespace input
