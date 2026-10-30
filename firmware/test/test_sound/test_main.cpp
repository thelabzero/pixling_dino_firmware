// Run: cd firmware && pio test -e native
#include <unity.h>
#include "sound/dino_sounds.h"
#include "sound/tone_player.h"

using namespace sound;

static const Step GLIDE[] = {{1000, 2000, 100, 50}, {0, 0, 50, 0}};

void setUp() {}
void tearDown() {}

void test_glide_rest_and_end() {
  TonePlayer p;
  p.start(Pattern{GLIDE, 2, 1, 0}, 0);
  TEST_ASSERT_EQUAL(1000, p.freq());
  p.update(50);
  TEST_ASSERT_EQUAL(1500, p.freq());        // halfway through the glide
  TEST_ASSERT_EQUAL(50, p.volume());
  p.update(120);
  TEST_ASSERT_EQUAL(0, p.freq());           // rest
  TEST_ASSERT_TRUE(p.playing());
  p.update(160);
  TEST_ASSERT_FALSE(p.playing());           // 150 ms total
}

void test_repeat_and_loop() {
  TonePlayer p;
  p.start(Pattern{GLIDE, 2, 2, 0}, 0);
  p.update(160);
  TEST_ASSERT_EQUAL(1100, p.freq());        // second round, 10 ms in
  p.start(Pattern{GLIDE, 2, 1, 1000}, 0);   // loop with a 1 s gap
  p.update(500);
  TEST_ASSERT_EQUAL(0, p.freq());
  TEST_ASSERT_TRUE(p.looping());
  p.update(1150);
  TEST_ASSERT_EQUAL(1000, p.freq());        // next round started at 1150
  p.stop();
  TEST_ASSERT_FALSE(p.playing());
}

void test_every_brain_sound_has_a_valid_pattern() {
  for (int i = 1; i < (int)utility::Sound::Count; i++) {
    Pattern pt = patternFor((utility::Sound)i);
    TEST_ASSERT_TRUE(pt.n > 0 && pt.steps != nullptr && pt.repeat >= 1);
    for (int k = 0; k < pt.n; k++) {
      TEST_ASSERT_TRUE(pt.steps[k].ms > 0 && pt.steps[k].vol <= 100);
      TEST_ASSERT_TRUE(pt.steps[k].f0 == 0 || (pt.steps[k].f0 >= 100 && pt.steps[k].f1 <= 8000));
    }
  }
}

static uint32_t totalMs(const Pattern& p) {
  uint32_t t = 0;
  for (int k = 0; k < p.n; k++) t += p.steps[k].ms;
  return t * p.repeat;
}

void test_every_dino_voice_has_a_valid_short_pattern() {
  for (int i = 1; i < (int)Voice::Count; i++) {
    Pattern pt = patternFor((Voice)i);
    TEST_ASSERT_TRUE(pt.n > 0 && pt.steps != nullptr && pt.repeat >= 1);
    TEST_ASSERT_EQUAL(0, pt.loopGapMs);  // voices play once; only alarms loop
    TEST_ASSERT_TRUE(totalMs(pt) <= 2500);
    for (int k = 0; k < pt.n; k++) {
      TEST_ASSERT_TRUE(pt.steps[k].ms > 0 && pt.steps[k].vol <= 100);
      TEST_ASSERT_TRUE(pt.steps[k].f0 == 0 || (pt.steps[k].f0 >= 1000 && pt.steps[k].f1 <= 4500));  // piezo's useful range
    }
  }
}

void test_alarm_stages_loop_and_other_sounds_do_not() {
  TEST_ASSERT_TRUE(patternFor(utility::Sound::AlarmStage1).loopGapMs > 0);
  TEST_ASSERT_TRUE(patternFor(utility::Sound::AlarmStage2).loopGapMs > 0);
  TEST_ASSERT_TRUE(patternFor(utility::Sound::AlarmStage3).loopGapMs > 0);
  TEST_ASSERT_EQUAL(0, patternFor(utility::Sound::MenuSelect).loopGapMs);
  TEST_ASSERT_EQUAL(0, patternFor(utility::Sound::Snooze).loopGapMs);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_glide_rest_and_end);
  RUN_TEST(test_repeat_and_loop);
  RUN_TEST(test_every_brain_sound_has_a_valid_pattern);
  RUN_TEST(test_every_dino_voice_has_a_valid_short_pattern);
  RUN_TEST(test_alarm_stages_loop_and_other_sounds_do_not);
  return UNITY_END();
}
