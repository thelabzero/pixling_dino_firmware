// Run: cd firmware && pio test -e native
#include <unity.h>
#include "life/life.h"

using namespace life;
using sound::Voice;

static Brain b;
static uint32_t T;

static void run(uint32_t ms) { for (uint32_t e = 0; e < ms; e += 100, T += 100) b.tick(T); }
static Act act() { return b.view().act; }
static void finish() { b.onClipDone(T); }
static Voice voiceOf(Act a, int frame) {
  int n; const Cue* c = cues(n);
  for (int i = 0; i < n; i++) if (c[i].act == a && c[i].frame == frame) return c[i].voice;
  return Voice::None;
}

void setUp() { b = Brain(); b.seed(12345); T = 1000; run(100); }
void tearDown() {}

void test_starts_idle_loop_not_dim() {
  TEST_ASSERT_TRUE(act() == Act::Idle);
  TEST_ASSERT_TRUE(b.view().loop);
  TEST_ASSERT_FALSE(b.view().dim);
}

void test_touch_pets_after_a_pause() {
  const uint32_t s0 = b.view().serial;
  b.onTouch(T);
  TEST_ASSERT_TRUE(act() == Act::Idle);  // pausing: noticing
  run(100);
  TEST_ASSERT_TRUE(act() == Act::Idle || act() == Act::Pet);
  run(300);
  TEST_ASSERT_TRUE(act() == Act::Pet);
  TEST_ASSERT_TRUE(b.view().serial != s0);
  TEST_ASSERT_FALSE(b.view().loop);
  finish();
  TEST_ASSERT_TRUE(act() == Act::Idle);
}

void test_pause_is_120_to_300_ms() {
  for (int i = 0; i < 50; i++) {
    setUp();
    b.onTouch(T);
    uint32_t t = 0;
    while (act() != Act::Pet) { b.tick(T + t); t++; }
    TEST_ASSERT_TRUE(t >= 120 && t <= 300);
  }
}

void test_fourth_touch_in_8s_is_overpet_then_10s_lockout() {
  for (int i = 0; i < 3; i++) { b.onTouch(T); run(1000); finish(); run(100); }
  b.onTouch(T); run(400);
  TEST_ASSERT_TRUE(act() == Act::Overpet);
  finish();
  // lockout: 10 s of touches only get a glance
  run(1000);
  b.onTouch(T); run(400);
  TEST_ASSERT_TRUE(act() == Act::Glance);
  finish(); run(8000);
  b.onTouch(T); run(400);
  TEST_ASSERT_TRUE(act() == Act::Glance);
  finish(); run(2000);  // > 10 s since the over-pet
  b.onTouch(T); run(400);
  TEST_ASSERT_TRUE(act() == Act::Pet);
}

void test_slow_touches_never_overpet() {
  for (int i = 0; i < 12; i++) {
    b.onTouch(T); run(400);
    TEST_ASSERT_TRUE(act() == Act::Pet);
    finish(); run(3000);
  }
}

void test_overpet_waits_for_a_playing_clip() {
  b.onTouch(T); run(400);  // pet
  b.onTouch(T); b.onTouch(T); b.onTouch(T);  // 4th touch inside 8 s, pet still playing
  TEST_ASSERT_TRUE(act() == Act::Pet);
  finish();
  TEST_ASSERT_TRUE(act() == Act::Overpet);
}

void test_rawr_between_2_and_4_minutes() {
  run(119000);
  TEST_ASSERT_TRUE(act() == Act::Idle);
  run(121000);  // 240 s total
  TEST_ASSERT_TRUE(act() == Act::Rawr);
  finish();
  TEST_ASSERT_TRUE(act() == Act::Idle);
}

void test_sleeps_after_5_min_dims_once_asleep_then_wakes_on_touch() {
  // input at 4:00 keeps it awake past 5:00 from the start
  run(240000); b.onActivity(T);
  run(290000);
  TEST_ASSERT_FALSE(act() == Act::FallAsleep || act() == Act::Asleep);
  run(20000);
  TEST_ASSERT_TRUE(act() == Act::FallAsleep || act() == Act::Rawr || act() == Act::Asleep);
}

void test_sleep_sequence() {
  b.playNow(Act::Idle, T);
  b.onActivity(T);
  run(300000 + 100);
  if (act() == Act::Rawr) { finish(); run(300000); }
  TEST_ASSERT_TRUE(act() == Act::FallAsleep);
  TEST_ASSERT_FALSE(b.view().dim);  // dims once asleep
  finish();
  TEST_ASSERT_TRUE(act() == Act::Asleep);
  TEST_ASSERT_TRUE(b.view().dim);
  TEST_ASSERT_TRUE(b.view().loop);
  run(60000);  // stays asleep, no rawr
  TEST_ASSERT_TRUE(act() == Act::Asleep);
  b.onTouch(T); run(400);
  TEST_ASSERT_TRUE(act() == Act::Wake);   // the waking touch does not also pet
  TEST_ASSERT_FALSE(b.view().dim);
  finish();
  TEST_ASSERT_TRUE(act() == Act::Idle);
}

void test_motion_wakes_a_sleeping_dino() {
  b.onActivity(T); run(300100);
  if (act() == Act::Rawr) { finish(); run(300000); }
  finish();
  TEST_ASSERT_TRUE(act() == Act::Asleep);
  b.onShake(T);
  TEST_ASSERT_TRUE(act() == Act::Wake);
}

void test_alarm_dismiss_wakes_the_dino() {
  b.onActivity(T); run(300100);
  if (act() == Act::Rawr) { finish(); run(300000); }
  finish();
  TEST_ASSERT_TRUE(act() == Act::Asleep);
  b.onAlarmDismissed(T);
  TEST_ASSERT_TRUE(act() == Act::Wake);
  TEST_ASSERT_FALSE(b.view().dim);
  finish();
  TEST_ASSERT_TRUE(act() == Act::Idle);
}

void test_motion_reactions() {
  b.onShake(T);
  TEST_ASSERT_TRUE(act() == Act::Shake);
  finish();
  b.onPickUp(T);
  TEST_ASSERT_TRUE(act() == Act::PickUp);
  finish();
  b.onTilt(T);
  TEST_ASSERT_TRUE(act() == Act::Tilt);
  finish();
  b.onTilt(T);  // repeats of a held tilt do not replay it at once
  TEST_ASSERT_TRUE(act() == Act::Idle);
  run(4100);
  b.onTilt(T);
  TEST_ASSERT_TRUE(act() == Act::Tilt);
}

void test_shake_interrupts_pet() {
  b.onTouch(T); run(400);
  TEST_ASSERT_TRUE(act() == Act::Pet);
  b.onShake(T);
  TEST_ASSERT_TRUE(act() == Act::Shake);
}

void test_home_resumed_starts_from_idle() {
  b.onShake(T);
  b.onHomeResumed(T);
  TEST_ASSERT_TRUE(act() == Act::Idle);
  TEST_ASSERT_TRUE(b.view().loop);
}

void test_voices_per_clip() {
  TEST_ASSERT_TRUE(voiceOf(Act::Pet, 1) == Voice::Purr);       // shy
  TEST_ASSERT_TRUE(voiceOf(Act::Pet, 3) == Voice::Trill);      // grin
  TEST_ASSERT_TRUE(voiceOf(Act::Overpet, 1) == Voice::Grumble);
  TEST_ASSERT_TRUE(voiceOf(Act::Rawr, 2) == Voice::Growl);
  TEST_ASSERT_TRUE(voiceOf(Act::Rawr, 5) == Voice::Giggle);
  TEST_ASSERT_TRUE(voiceOf(Act::Shake, 0) == Voice::Yelp);
  TEST_ASSERT_TRUE(voiceOf(Act::Shake, 2) == Voice::Dizzy);
  TEST_ASSERT_TRUE(voiceOf(Act::PickUp, 0) == Voice::Squeak);
  TEST_ASSERT_TRUE(voiceOf(Act::FallAsleep, 0) == Voice::Sigh);
  TEST_ASSERT_TRUE(voiceOf(Act::Idle, 0) == Voice::None);
}

void test_every_cue_frame_is_inside_its_act() {
  int n; const Cue* c = cues(n);
  for (int i = 0; i < n; i++) {
    const ActDef& d = actDef(c[i].act);
    TEST_ASSERT_TRUE(c[i].frame >= d.first && c[i].frame <= d.last);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_idle_loop_not_dim);
  RUN_TEST(test_touch_pets_after_a_pause);
  RUN_TEST(test_pause_is_120_to_300_ms);
  RUN_TEST(test_fourth_touch_in_8s_is_overpet_then_10s_lockout);
  RUN_TEST(test_slow_touches_never_overpet);
  RUN_TEST(test_overpet_waits_for_a_playing_clip);
  RUN_TEST(test_rawr_between_2_and_4_minutes);
  RUN_TEST(test_sleeps_after_5_min_dims_once_asleep_then_wakes_on_touch);
  RUN_TEST(test_sleep_sequence);
  RUN_TEST(test_motion_wakes_a_sleeping_dino);
  RUN_TEST(test_alarm_dismiss_wakes_the_dino);
  RUN_TEST(test_motion_reactions);
  RUN_TEST(test_shake_interrupts_pet);
  RUN_TEST(test_home_resumed_starts_from_idle);
  RUN_TEST(test_voices_per_clip);
  RUN_TEST(test_every_cue_frame_is_inside_its_act);
  return UNITY_END();
}
