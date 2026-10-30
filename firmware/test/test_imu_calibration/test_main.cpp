// Run: cd firmware && pio test -e native
#include <unity.h>
#include <math.h>
#include "input/imu_calibration.h"

using namespace input;

void setUp() {}
void tearDown() {}

static ImuCalibration measured() {  // values measured on the dev unit, 2026-10-18
  ImuCalibration c;
  c.accOffsetG = {0.0319f, -0.0030f, 0.2957f};
  c.accScale = {0.9896f, 0.9902f, 1.0087f};
  c.gyroBiasDps = {-1.4967f, 2.8881f, -6.4404f};
  return c;
}

void test_applies_offset_scale_bias() {
  ImuCorrector k(measured());
  Vec3 a = {0.0319f, -0.0030f, 0.2957f + 1.0087f}, g = {-1.4967f, 2.8881f, -6.4404f};  // standing, still
  k.apply(a, g, 1000);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, a.x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, a.z);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, g.z);
}

void test_learns_new_bias_when_still() {
  ImuCorrector k(measured());
  const Vec3 drifted = {-1.0f, 3.5f, -5.0f};  // chip warmed up: bias moved
  for (uint32_t t = 0; t < 20000; t += 20) {
    Vec3 a = {0.0319f, -0.0030f, 1.3044f}, g = drifted;
    k.apply(a, g, t + 20);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.05f, -5.0f, k.gyroBias().z);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.5f, k.gyroBias().y);
}

void test_does_not_learn_while_turning() {
  ImuCorrector k(measured());
  for (uint32_t t = 0; t < 5000; t += 20) {
    Vec3 a = {0.0319f, -0.0030f, 1.3044f}, g = {-1.4967f, 2.8881f, -6.4404f + 30.0f};  // turning at 30 deg/s
    k.apply(a, g, t + 20);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -6.4404f, k.gyroBias().z);
  TEST_ASSERT_TRUE(!k.learning());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_applies_offset_scale_bias);
  RUN_TEST(test_learns_new_bias_when_still);
  RUN_TEST(test_does_not_learn_while_turning);
  return UNITY_END();
}
