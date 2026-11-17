// Run: cd firmware && pio test -e native
#include <unity.h>
#include "input/touch.h"

using namespace input;

static TouchGestures g;
static uint32_t T;

// hold the pad at `pressed` for ms, feeding every 5 ms like the real loop
static void feed(bool pressed, uint32_t ms) { for (uint32_t e = 0; e < ms; e += 5, T += 5) g.update(pressed, T); }
static Event one() { Event e = g.next(); return e; }

void setUp() { g = TouchGestures(); T = 1000; feed(false, 100); }
void tearDown() {}

void test_tap_after_double_window() {
  feed(true, 100); feed(false, 200);
  TEST_ASSERT_TRUE(one() == Event::None);   // still waiting for a possible second tap
  feed(false, 300);
  TEST_ASSERT_TRUE(one() == Event::Tap);
  TEST_ASSERT_TRUE(one() == Event::None);
}

void test_double_tap() {
  feed(true, 100); feed(false, 150); feed(true, 100); feed(false, 600);
  TEST_ASSERT_TRUE(one() == Event::DoubleTap);
  TEST_ASSERT_TRUE(one() == Event::None);
}

void test_long_press_sent_on_release() {
  feed(true, 2000);
  TEST_ASSERT_TRUE(one() == Event::None);
  feed(false, 100);
  TEST_ASSERT_TRUE(one() == Event::LongPress);
}

void test_hold_sent_while_held_and_nothing_on_release() {
  feed(true, 3100);
  TEST_ASSERT_TRUE(one() == Event::Hold);
  feed(true, 2000); feed(false, 600);
  TEST_ASSERT_TRUE(one() == Event::None);
}

void test_middle_length_press_ignored() {
  feed(true, 800); feed(false, 600);
  TEST_ASSERT_TRUE(one() == Event::None);
}

void test_bounce_ignored() {
  feed(true, 10); feed(false, 600);         // 10 ms glitch < 30 ms debounce
  TEST_ASSERT_TRUE(one() == Event::None);
}

// touch-only timings (config.h with TOUCH_ONLY): no double tap to wait for, no hold; a long tap is any length past longMs
static void touchOnlyTimings() {
  TouchConfig c; c.doubleGapMs = 0; c.longMs = 700; c.holdMs = 60000; g.setConfig(c);
}
void test_touch_only_tap_is_instant() {
  touchOnlyTimings();
  feed(true, 100); feed(false, 40);              // 40 ms: past the 30 ms debounce
  TEST_ASSERT_TRUE(one() == Event::Tap);        // sent right after release, not after a 400 ms window
  TEST_ASSERT_TRUE(one() == Event::None);
}
void test_touch_only_two_quick_taps_are_two_taps() {
  touchOnlyTimings();
  feed(true, 100); feed(false, 100); feed(true, 100); feed(false, 100);
  TEST_ASSERT_TRUE(one() == Event::Tap);
  TEST_ASSERT_TRUE(one() == Event::Tap);
  TEST_ASSERT_TRUE(one() == Event::None);
}
void test_touch_only_long_tap_never_becomes_a_hold() {
  touchOnlyTimings();
  feed(true, 5000);                              // way past the old 3 s hold
  TEST_ASSERT_TRUE(one() == Event::None);
  feed(false, 100);
  TEST_ASSERT_TRUE(one() == Event::LongPress);
  TEST_ASSERT_TRUE(one() == Event::None);
}

// multi-tap (touch-only mode, home screen): the gap is 350 ms, up to 4 taps
static void multiTap() { TouchConfig c; c.doubleGapMs = 350; c.longMs = 700; c.holdMs = 60000; c.maxTaps = 4; g.setConfig(c); }
static void tapN(int n) { for (int i = 0; i < n; i++) { feed(true, 80); feed(false, 100); } }
void test_multi_tap_counts_two_three_and_four() {
  multiTap(); tapN(1); feed(false, 400);
  TEST_ASSERT_TRUE(one() == Event::Tap); TEST_ASSERT_TRUE(one() == Event::None);
  multiTap(); tapN(2); feed(false, 400);
  TEST_ASSERT_TRUE(one() == Event::DoubleTap); TEST_ASSERT_TRUE(one() == Event::None);
  multiTap(); tapN(3); feed(false, 400);
  TEST_ASSERT_TRUE(one() == Event::TripleTap); TEST_ASSERT_TRUE(one() == Event::None);
  multiTap(); tapN(4); feed(false, 40);                  // the 4th is the last count there is: sent without waiting
  TEST_ASSERT_TRUE(one() == Event::QuadTap); TEST_ASSERT_TRUE(one() == Event::None);
}
void test_multi_tap_waits_for_the_gap_and_a_fifth_tap_starts_over() {
  multiTap(); tapN(2); feed(false, 100);
  TEST_ASSERT_TRUE(one() == Event::None);                // still waiting: a third tap may follow
  tapN(2); feed(false, 40);                              // 2 + 2 more within the gap = 4
  TEST_ASSERT_TRUE(one() == Event::QuadTap);
  tapN(1); feed(false, 400);
  TEST_ASSERT_TRUE(one() == Event::Tap);                 // a new count
}
void test_multi_tap_gap_can_change_while_running() {     // home screen counts, a menu does not wait
  multiTap(); g.setGapMs(0);
  tapN(1); feed(false, 40);
  TEST_ASSERT_TRUE(one() == Event::Tap);
  tapN(2); feed(false, 40);
  TEST_ASSERT_TRUE(one() == Event::Tap); TEST_ASSERT_TRUE(one() == Event::Tap); TEST_ASSERT_TRUE(one() == Event::None);
}
void test_default_config_still_stops_at_double_tap() {
  tapN(3); feed(false, 600);                             // maxTaps 2: two taps = DoubleTap at once, the third is a new tap
  TEST_ASSERT_TRUE(one() == Event::DoubleTap); TEST_ASSERT_TRUE(one() == Event::Tap); TEST_ASSERT_TRUE(one() == Event::None);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_tap_after_double_window);
  RUN_TEST(test_double_tap);
  RUN_TEST(test_long_press_sent_on_release);
  RUN_TEST(test_hold_sent_while_held_and_nothing_on_release);
  RUN_TEST(test_middle_length_press_ignored);
  RUN_TEST(test_bounce_ignored);
  RUN_TEST(test_touch_only_tap_is_instant);
  RUN_TEST(test_touch_only_two_quick_taps_are_two_taps);
  RUN_TEST(test_touch_only_long_tap_never_becomes_a_hold);
  RUN_TEST(test_multi_tap_counts_two_three_and_four);
  RUN_TEST(test_multi_tap_waits_for_the_gap_and_a_fifth_tap_starts_over);
  RUN_TEST(test_multi_tap_gap_can_change_while_running);
  RUN_TEST(test_default_config_still_stops_at_double_tap);
  return UNITY_END();
}
