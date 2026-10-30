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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_tap_after_double_window);
  RUN_TEST(test_double_tap);
  RUN_TEST(test_long_press_sent_on_release);
  RUN_TEST(test_hold_sent_while_held_and_nothing_on_release);
  RUN_TEST(test_middle_length_press_ignored);
  RUN_TEST(test_bounce_ignored);
  return UNITY_END();
}
