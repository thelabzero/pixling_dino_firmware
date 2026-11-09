// Run: cd firmware && pio test -e native
// IntroPlayer: the boot film's clock (cumulative schedule, stale steps skipped, tap skips to the live face, ends once).
#include <unity.h>
#include "boot/intro_player.h"

using namespace boot;

void setUp() {}
void tearDown() {}

static const Film& zoomFilm() { return film(V_ZOOM); }

void test_starts_on_step_zero_and_asks_for_a_draw_once() {
  IntroPlayer p;
  p.begin(zoomFilm(), 1000);
  TEST_ASSERT_TRUE(p.active());
  TEST_ASSERT_EQUAL(0, p.step());
  TEST_ASSERT_TRUE(p.update(1000));   // draw step 0
  TEST_ASSERT_FALSE(p.update(1001));  // nothing new
}

void test_steps_follow_the_cumulative_schedule() {
  IntroPlayer p;
  const Film& f = zoomFilm();
  p.begin(f, 0);
  p.update(0);
  uint32_t t = 0;
  for (int i = 1; i < f.n; i++) {
    t += f.steps[i - 1].ms;
    TEST_ASSERT_FALSE(p.update(t - 1));  // one ms early: still the old step
    TEST_ASSERT_TRUE(p.update(t));       // exactly due
    TEST_ASSERT_EQUAL(i, p.step());
  }
  t += f.steps[f.n - 1].ms;
  TEST_ASSERT_TRUE(p.active());
  p.update(t);
  TEST_ASSERT_FALSE(p.active());  // total duration reached
  TEST_ASSERT_EQUAL(totalMs(f), t);
}

void test_a_late_loop_skips_stale_steps_but_never_goes_back() {
  IntroPlayer p;
  const Film& f = zoomFilm();
  p.begin(f, 0);
  p.update(0);
  TEST_ASSERT_TRUE(p.update(700));  // many steps overdue: draw only the latest
  int s = p.step();
  TEST_ASSERT_TRUE(s > 2);
  p.update(710);
  TEST_ASSERT_TRUE(p.step() >= s);
}

void test_skip_jumps_to_the_live_face_and_holds_it() {
  IntroPlayer p;
  const Film& f = zoomFilm();
  p.begin(f, 0);
  p.update(0);
  p.skip(300);
  TEST_ASSERT_TRUE(p.update(300));
  TEST_ASSERT_TRUE(p.onLastStep());
  TEST_ASSERT_EQUAL((int)Op::Live, (int)p.current().op);
  TEST_ASSERT_FALSE(p.update(300 + f.steps[f.n - 1].ms - 1));
  TEST_ASSERT_TRUE(p.active());
  p.update(300 + f.steps[f.n - 1].ms);
  TEST_ASSERT_FALSE(p.active());
}

void test_skip_on_the_last_step_does_not_extend_it() {
  IntroPlayer p;
  const Film& f = zoomFilm();
  p.begin(f, 0);
  p.update(0);
  p.skip(10);
  p.update(10);
  p.skip(400);  // a second tap during the last hold
  p.update(10 + f.steps[f.n - 1].ms);
  TEST_ASSERT_FALSE(p.active());
}

void test_clock_wrap_is_safe() {
  IntroPlayer p;
  p.begin(zoomFilm(), 0xFFFFFF00u);  // millis() about to wrap
  p.update(0xFFFFFF00u);
  TEST_ASSERT_TRUE(p.update(0xFFFFFF00u + 200));
  TEST_ASSERT_TRUE(p.step() >= 1);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_on_step_zero_and_asks_for_a_draw_once);
  RUN_TEST(test_steps_follow_the_cumulative_schedule);
  RUN_TEST(test_a_late_loop_skips_stale_steps_but_never_goes_back);
  RUN_TEST(test_skip_jumps_to_the_live_face_and_holds_it);
  RUN_TEST(test_skip_on_the_last_step_does_not_extend_it);
  RUN_TEST(test_clock_wrap_is_safe);
  return UNITY_END();
}
