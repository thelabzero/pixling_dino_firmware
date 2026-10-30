// IMU input: MPU6050 -> calibration -> gestures -> app.emit(TiltLeft / TiltRight / FlipDown / FlipUp).
// Calibration and gesture thresholds come from config.h (sections 4 and 5).
// Pick up, put down and shake are detected but not sent yet: nothing uses them, and they
// misfire until the IMU is rigidly mounted (plan step 2). Add them in kSend[] when needed.
#pragma once
#include "../app/module.h"
#include "../drivers/mpu6050.h"
#include "../input/imu_calibration.h"
#include "../input/imu_gestures.h"

class ImuInput : public Module {
 public:
  const char* name() const override { return "imu"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  void printInfo(Print& out) const override;

 private:
  Mpu6050 imu_;
  input::ImuCorrector corrector_;
  input::ImuGestures gestures_;
  uint32_t nextSample_ = 0;
  uint32_t readErrors_ = 0;
};
