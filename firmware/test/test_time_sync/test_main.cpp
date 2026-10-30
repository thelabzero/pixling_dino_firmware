// Run: cd firmware && pio test -e native
#include <unity.h>
#include <string.h>
#include "timekeeping/time_sync.h"

using namespace timekeeping;

void setUp() {}
void tearDown() {}

void test_due_at_boot_then_daily() {
  SyncSchedule s(86400000UL);
  s.start(1000);
  TEST_ASSERT_TRUE(s.due(1000));
  s.onSuccess(5000);
  TEST_ASSERT_TRUE(!s.due(5000 + 86399999UL));
  TEST_ASSERT_TRUE(s.due(5000 + 86400000UL));
}

void test_backoff_after_failures() {
  SyncSchedule s;
  s.start(0);
  s.onFailure(0);            // +1 min
  TEST_ASSERT_TRUE(!s.due(59999));
  TEST_ASSERT_TRUE(s.due(60000));
  s.onFailure(60000);        // +5 min
  TEST_ASSERT_TRUE(s.due(60000 + 300000));
  s.onFailure(400000);       // +15 min
  s.onFailure(1300000);      // +60 min
  s.onFailure(4900000);      // stays 60 min
  TEST_ASSERT_TRUE(!s.due(4900000 + 3599999UL));
  TEST_ASSERT_TRUE(s.due(4900000 + 3600000UL));
  s.onSuccess(9000000);
  TEST_ASSERT_EQUAL(0, s.failures());
}

void test_millis_wraparound() {
  SyncSchedule s(1000);
  s.start(0xFFFFFF00UL);
  s.onSuccess(0xFFFFFF00UL);  // next at 0x000002E8 after the wrap
  TEST_ASSERT_TRUE(!s.due(0xFFFFFFF0UL));
  TEST_ASSERT_TRUE(s.due(0x00000300UL));
}

void test_format() {
  char b[6];
  formatHHMM(true, 7, 5, b);   TEST_ASSERT_TRUE(strcmp(b, "07:05") == 0);
  formatHHMM(true, 23, 59, b); TEST_ASSERT_TRUE(strcmp(b, "23:59") == 0);
  formatHHMM(false, 7, 5, b);  TEST_ASSERT_TRUE(strcmp(b, "--:--") == 0);
  formatHHMM(true, 24, 0, b);  TEST_ASSERT_TRUE(strcmp(b, "--:--") == 0);
}

void test_epoch_valid() {
  TEST_ASSERT_TRUE(!epochLooksValid(0));
  TEST_ASSERT_TRUE(!epochLooksValid(86400));
  TEST_ASSERT_TRUE(epochLooksValid(1792400000LL));  // Oct 2026
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_due_at_boot_then_daily);
  RUN_TEST(test_backoff_after_failures);
  RUN_TEST(test_millis_wraparound);
  RUN_TEST(test_format);
  RUN_TEST(test_epoch_valid);
  return UNITY_END();
}
