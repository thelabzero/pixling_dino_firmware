// Run: cd firmware && pio test -e native
#include <unity.h>
#include "power/battery_level.h"

using namespace power;

void setUp() {}
void tearDown() {}

void test_divider_doubles_for_equal_resistors() {
  TEST_ASSERT_EQUAL(4000, batteryMv(2000, 330, 330, 1.0f));
  TEST_ASSERT_EQUAL(4040, batteryMv(2000, 330, 330, 1.01f));
}

void test_percent_curve() {
  TEST_ASSERT_EQUAL(100, lipoPercent(4250));
  TEST_ASSERT_EQUAL(100, lipoPercent(4200));
  TEST_ASSERT_EQUAL(50, lipoPercent(3840));
  TEST_ASSERT_EQUAL(0, lipoPercent(3200));
  TEST_ASSERT_TRUE(lipoPercent(3900) > lipoPercent(3800));
}

void test_smoother() {
  Smoother s(0.5f);
  TEST_ASSERT_FALSE(s.ready());
  s.add(4000);
  TEST_ASSERT_EQUAL(4000, (int)s.value());
  s.add(3000);
  TEST_ASSERT_EQUAL(3500, (int)s.value());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_divider_doubles_for_equal_resistors);
  RUN_TEST(test_percent_curve);
  RUN_TEST(test_smoother);
  return UNITY_END();
}
