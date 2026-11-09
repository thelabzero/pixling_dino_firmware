// Run: cd firmware && pio test -e native
#include <unity.h>
#include <string.h>
#include "soak/soak_record.h"

using namespace soak;

void setUp() {}
void tearDown() {}

static Record sample() {
  Record r;
  r.type = Type::Sample; r.boot = 7; r.uptimeS = 86399; r.epoch = 1794000000u; r.battMv = 3987;
  r.stackLeft = 9792; r.heapFree = 215284; r.heapMin = 201000; r.heapMaxBlock = 110000;
  r.loopWorstMs = 130; r.renderWorstMs = 74; r.imuErrors = 2; r.syncOk = 3; r.syncFail = 1;
  r.driftS = -12; r.state = 4; r.code = 0; r.arg = 0; r.flags = FL_TIME_VALID | FL_HOST_OPEN;
  return r;
}

void test_roundtrip() {
  uint8_t b[RECORD_BYTES];
  const Record a = sample();
  encode(a, b);
  Record c;
  TEST_ASSERT_TRUE(decode(b, c));
  TEST_ASSERT_EQUAL(a.boot, c.boot);
  TEST_ASSERT_EQUAL_UINT32(a.uptimeS, c.uptimeS);
  TEST_ASSERT_EQUAL_UINT32(a.epoch, c.epoch);
  TEST_ASSERT_EQUAL(a.battMv, c.battMv);
  TEST_ASSERT_EQUAL_UINT32(a.heapMin, c.heapMin);
  TEST_ASSERT_EQUAL_UINT32(a.heapMaxBlock, c.heapMaxBlock);
  TEST_ASSERT_EQUAL(-12, c.driftS);
  TEST_ASSERT_EQUAL(a.flags, c.flags);
  TEST_ASSERT_TRUE(c.type == Type::Sample);
}

void test_layout_is_stable() {  // logs already written must stay readable: pin a few bytes
  uint8_t b[RECORD_BYTES];
  encode(sample(), b);
  TEST_ASSERT_EQUAL_HEX8(0xD5, b[0]);
  TEST_ASSERT_EQUAL(2, b[1]);
  TEST_ASSERT_EQUAL(7, b[2]);
  TEST_ASSERT_EQUAL(3987 & 0xFF, b[12]);
  TEST_ASSERT_EQUAL(3987 >> 8, b[13]);
}

void test_torn_or_empty_records_are_rejected() {
  uint8_t b[RECORD_BYTES];
  Record c;
  encode(sample(), b);
  b[20] ^= 0x01;  // one bit flipped
  TEST_ASSERT_FALSE(decode(b, c));
  memset(b, 0xFF, sizeof(b));  // erased flash
  TEST_ASSERT_FALSE(decode(b, c));
  memset(b, 0x00, sizeof(b));  // zero padding
  TEST_ASSERT_FALSE(decode(b, c));
  encode(sample(), b);
  memset(b + 24, 0xFF, 24);  // write cut half way
  TEST_ASSERT_FALSE(decode(b, c));
}

void test_csv_line() {
  char buf[200];
  Record r = sample();
  const size_t n = formatCsv(r, 5, buf, sizeof(buf));
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_EQUAL_STRING("R,5,sample,7,86399,1794000000,3987,215284,201000,110000,9792,130,74,2,3,1,-12,4,0,,0,6", buf);
  r.type = Type::Boot; r.code = 9; r.arg = 4000;
  formatCsv(r, 6, buf, sizeof(buf));
  TEST_ASSERT_NOT_NULL(strstr(buf, ",brownout,4000,"));
  TEST_ASSERT_EQUAL(0, (int)formatCsv(r, 6, buf, 10));  // buffer too small
}

void test_names() {
  TEST_ASSERT_EQUAL_STRING("power_on", resetName(1));
  TEST_ASSERT_EQUAL_STRING("brownout", resetName(9));
  TEST_ASSERT_EQUAL_STRING("?", resetName(200));
  TEST_ASSERT_TRUE(resetIsAbnormal(4));
  TEST_ASSERT_TRUE(resetIsAbnormal(9));
  TEST_ASSERT_FALSE(resetIsAbnormal(1));
  TEST_ASSERT_FALSE(resetIsAbnormal(3));
  TEST_ASSERT_EQUAL_STRING("loop_stall", eventName(EV_LOOP_STALL));
}

void test_sample_interval_speeds_up_when_critical() {
  Policy p;
  TEST_ASSERT_EQUAL_UINT32(60000, sampleInterval(p, 4000));
  TEST_ASSERT_EQUAL_UINT32(60000, sampleInterval(p, 3400));
  TEST_ASSERT_EQUAL_UINT32(15000, sampleInterval(p, 3200));
  TEST_ASSERT_EQUAL_UINT32(60000, sampleInterval(p, 0));  // no reading yet
}

void test_flush_policy() {
  Policy p;
  TEST_ASSERT_FALSE(flushDue(p, 0, true, 999999, 0, 4000));       // nothing to write
  TEST_ASSERT_FALSE(flushDue(p, 3, false, 1000, 0, 4000));        // healthy, recent
  TEST_ASSERT_TRUE(flushDue(p, 1, true, 1000, 0, 4000));          // an event: now
  TEST_ASSERT_TRUE(flushDue(p, 16, false, 1000, 0, 4000));        // buffer full
  TEST_ASSERT_TRUE(flushDue(p, 2, false, 600000, 0, 4000));       // 10 min
  TEST_ASSERT_FALSE(flushDue(p, 2, false, 599999, 0, 4000));
  TEST_ASSERT_TRUE(flushDue(p, 1, false, 1000, 0, 3400));         // low battery: every record
  TEST_ASSERT_TRUE(flushDue(p, 2, false, 600000, 4294967000u, 4000));   // millis() wrapped (600.3 s since the flush)
  TEST_ASSERT_FALSE(flushDue(p, 2, false, 5, 4294967000u, 4000));       // wrapped, only 0.3 s since the flush
}

void test_battery_watch() {
  Policy p;
  BatteryWatch w(p);
  TEST_ASSERT_EQUAL(EV_NONE, w.update(0));
  TEST_ASSERT_EQUAL(EV_NONE, w.update(3900));
  TEST_ASSERT_EQUAL(EV_BATTERY_LOW, w.update(3490));
  TEST_ASSERT_EQUAL(EV_NONE, w.update(3480));       // reported once
  TEST_ASSERT_EQUAL(EV_BATTERY_CRITICAL, w.update(3290));
  TEST_ASSERT_EQUAL(EV_NONE, w.update(3280));
  TEST_ASSERT_EQUAL(EV_NONE, w.update(3350));       // small rise: noise
  TEST_ASSERT_EQUAL(EV_BATTERY_RECOVERED, w.update(3450));  // 170 mV above the lowest: charging
  TEST_ASSERT_EQUAL(EV_NONE, w.update(3460));       // still under the low line: no second "low"
  TEST_ASSERT_EQUAL(EV_NONE, w.update(3900));       // above it: armed again
  TEST_ASSERT_EQUAL(EV_BATTERY_LOW, w.update(3480));
}

void test_battery_watch_goes_straight_to_critical() {
  Policy p;
  BatteryWatch w(p);
  TEST_ASSERT_EQUAL(EV_BATTERY_CRITICAL, w.update(3200));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_roundtrip);
  RUN_TEST(test_layout_is_stable);
  RUN_TEST(test_torn_or_empty_records_are_rejected);
  RUN_TEST(test_csv_line);
  RUN_TEST(test_names);
  RUN_TEST(test_sample_interval_speeds_up_when_critical);
  RUN_TEST(test_flush_policy);
  RUN_TEST(test_battery_watch);
  RUN_TEST(test_battery_watch_goes_straight_to_critical);
  return UNITY_END();
}
