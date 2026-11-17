// Run: cd firmware && pio test -e native
#include <stdio.h>
#include <string.h>
#include <string>
#include <unity.h>
#include "power/hud.h"
#include "power/power_policy.h"

using namespace power;

void setUp() {}
void tearDown() {}

static bool on(const uint8_t* page, int x, int y) { return page[(y / 8) * hud::W + x] & (1 << (y % 8)); }

static std::string ascii(const uint8_t* page, int x0, int y0, int w, int h) {
  std::string s;
  for (int y = y0; y < y0 + h; y++) { for (int x = x0; x < x0 + w; x++) s += on(page, x, y) ? '#' : '.'; s += '\n'; }
  return s;
}

void test_screen_policy() {
  ScreenConfig c;
  ScreenInput in;
  TEST_ASSERT_EQUAL((int)ScreenMode::Full, (int)decideScreen(c, in));
  in.idleMs = 59999; TEST_ASSERT_EQUAL((int)ScreenMode::Full, (int)decideScreen(c, in));
  in.idleMs = 60000; TEST_ASSERT_EQUAL((int)ScreenMode::Idle, (int)decideScreen(c, in));
  in.idleMs = 0; in.saver = true; TEST_ASSERT_EQUAL((int)ScreenMode::Idle, (int)decideScreen(c, in));
  in = ScreenInput(); in.asleep = true; in.asleepMs = 29999;
  TEST_ASSERT_EQUAL((int)ScreenMode::Night, (int)decideScreen(c, in));
  in.asleepMs = 30000; TEST_ASSERT_EQUAL((int)ScreenMode::Off, (int)decideScreen(c, in));
  c.offAfterMs = 0; TEST_ASSERT_EQUAL((int)ScreenMode::Night, (int)decideScreen(c, in));  // 0 = never off
}

void test_alarm_and_utility_screens_are_never_dimmed_or_off() {
  ScreenConfig c;
  ScreenInput in;
  in.home = false; in.asleep = true; in.asleepMs = 999999; in.idleMs = 999999; in.saver = true;
  TEST_ASSERT_EQUAL((int)ScreenMode::Full, (int)decideScreen(c, in));
}

void test_percent_only_goes_down_on_battery() {
  PercentFilter f;
  TEST_ASSERT_EQUAL(-1, f.value());
  TEST_ASSERT_EQUAL(80, f.update(80, false));
  TEST_ASSERT_EQUAL(78, f.update(78, false));
  TEST_ASSERT_EQUAL(78, f.update(82, false));  // sag then recovery: the number does not bounce
  TEST_ASSERT_EQUAL(90, f.update(90, false));  // a real recovery (charged) is taken
}

void test_percent_after_usb_is_read_fresh() {
  PercentFilter f;
  f.update(30, false);
  TEST_ASSERT_EQUAL(30, f.update(0, true));    // USB: keep the last value, ignore the reading
  TEST_ASSERT_EQUAL(97, f.update(97, false));  // unplugged after charging
}

void test_cutoff_needs_a_steady_low_voltage() {
  CutoffWatch w(3300, 30000, 2500);
  TEST_ASSERT_FALSE(w.update(3290, false, 1000));
  TEST_ASSERT_FALSE(w.update(3290, false, 30999));
  TEST_ASSERT_TRUE(w.update(3290, false, 31000));
  w.update(3400, false, 32000);                    // back above: the count restarts
  TEST_ASSERT_FALSE(w.update(3290, false, 33000));
  TEST_ASSERT_FALSE(w.update(3290, false, 62999));
}

void test_cutoff_ignores_usb_no_battery_and_off() {
  CutoffWatch w(3300, 0, 2500);
  TEST_ASSERT_FALSE(w.update(3000, true, 0));      // USB voltage
  TEST_ASSERT_FALSE(w.update(1200, false, 10));    // no battery connected
  CutoffWatch off(0, 0, 2500);
  TEST_ASSERT_FALSE(off.update(3000, false, 10));  // switched off
  TEST_ASSERT_TRUE(w.update(3000, false, 20));
}

void test_cutoff_wraps_with_millis() {
  CutoffWatch w(3300, 30000, 2500);
  const uint32_t t0 = 4294960000u;
  w.update(3200, false, t0);
  TEST_ASSERT_TRUE(w.update(3200, false, t0 + 30000u));  // crosses the 32-bit wrap
}

void test_bars() {
  TEST_ASSERT_EQUAL(0, hud::bars(0));
  TEST_ASSERT_EQUAL(1, hud::bars(1));
  TEST_ASSERT_EQUAL(1, hud::bars(25));
  TEST_ASSERT_EQUAL(2, hud::bars(26));
  TEST_ASSERT_EQUAL(4, hud::bars(76));
  TEST_ASSERT_EQUAL(4, hud::bars(100));
}

void test_battery_icon_look() {
  static uint8_t page[hud::W * hud::H / 8];
  memset(page, 0, sizeof page);
  hud::State s; s.battery = true; s.percent = 87;
  hud::draw(s, page);
  const std::string got = ascii(page, 0, 0, 16, 15);
  // outline: top and bottom rows full, tip two pixels, four bars of 2 px, digits "87" under it
  for (int x = 1; x <= 13; x++) { TEST_ASSERT_TRUE(on(page, x, 1)); TEST_ASSERT_TRUE(on(page, x, 6)); }
  TEST_ASSERT_TRUE(on(page, 14, 3)); TEST_ASSERT_TRUE(on(page, 14, 4));
  TEST_ASSERT_FALSE(on(page, 14, 2));
  for (int b = 0; b < 4; b++) TEST_ASSERT_TRUE(on(page, 2 + b * 3, 3));
  TEST_ASSERT_FALSE(on(page, 4, 3));  // the gap between bars
  bool digits = false;
  for (int y = 9; y < 14; y++) for (int x = 1; x < 8; x++) digits |= on(page, x, y);
  TEST_ASSERT_TRUE(digits);
  (void)got;
}

void test_charging_shows_a_bolt_and_no_digits() {
  static uint8_t page[hud::W * hud::H / 8];
  memset(page, 0, sizeof page);
  hud::State s; s.battery = true; s.charging = true; s.percent = 50;
  hud::draw(s, page);
  bool digits = false;
  for (int y = 9; y < 14; y++) for (int x = 0; x < 16; x++) digits |= on(page, x, y);
  TEST_ASSERT_FALSE(digits);
  TEST_ASSERT_TRUE(on(page, 8, 2));   // bolt pixel
  TEST_ASSERT_FALSE(on(page, 2, 3));  // no bars
}

void test_icon_and_clock_stay_in_their_corners() {
  static uint8_t page[hud::W * hud::H / 8];
  for (int shift = 0; shift < 9; shift++) {
    memset(page, 0, sizeof page);
    hud::State s; s.battery = true; s.percent = 100; s.clock = true; s.hour = 23; s.minute = 58; s.shift = (uint8_t)shift;
    hud::draw(s, page);
    for (int y = 0; y < hud::H; y++)
      for (int x = 0; x < hud::W; x++)
        if (on(page, x, y)) {
          const bool icon = x <= 14 && y <= 14;      // top-left: icon + up to 3 digits
          const bool clock = x >= 90 && y >= 51;     // bottom-right: 34 x 10 plus the 1 px shift
          TEST_ASSERT_TRUE_MESSAGE(icon || clock, "pixel outside the two corners");
        }
  }
}

void test_clock_digits() {
  static uint8_t page[hud::W * hud::H / 8];
  memset(page, 0, sizeof page);
  hud::State s; s.clock = true; s.hour = 10; s.minute = 24; s.shift = 4;  // shift 4 = centre of the ring
  hud::draw(s, page);
  printf("%s", ascii(page, 90, 51, 38, 12).c_str());
  TEST_ASSERT_TRUE(on(page, hud::CLOCK_X + 2, hud::CLOCK_Y));        // top of the "1" (2x: a 2 px wide stroke)
  TEST_ASSERT_TRUE(on(page, hud::CLOCK_X + 16, hud::CLOCK_Y + 2));  // colon, upper dot
  TEST_ASSERT_TRUE(on(page, hud::CLOCK_X + 17, hud::CLOCK_Y + 7));  // colon, lower dot
  TEST_ASSERT_FALSE(on(page, hud::CLOCK_X + 16, hud::CLOCK_Y + 4)); // nothing between the dots
  TEST_ASSERT_FALSE(on(page, 88, 57));                               // nothing left of the clock
  TEST_ASSERT_TRUE(hud::CLOCK_X + hud::CLOCK_W + 1 <= hud::W);       // the shifted clock stays on the screen
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_screen_policy);
  RUN_TEST(test_alarm_and_utility_screens_are_never_dimmed_or_off);
  RUN_TEST(test_percent_only_goes_down_on_battery);
  RUN_TEST(test_percent_after_usb_is_read_fresh);
  RUN_TEST(test_cutoff_needs_a_steady_low_voltage);
  RUN_TEST(test_cutoff_ignores_usb_no_battery_and_off);
  RUN_TEST(test_cutoff_wraps_with_millis);
  RUN_TEST(test_bars);
  RUN_TEST(test_battery_icon_look);
  RUN_TEST(test_charging_shows_a_bolt_and_no_digits);
  RUN_TEST(test_icon_and_clock_stay_in_their_corners);
  RUN_TEST(test_clock_digits);
  return UNITY_END();
}
