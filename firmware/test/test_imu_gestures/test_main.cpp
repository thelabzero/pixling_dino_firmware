// Desktop tests for IMU gestures with synthetic samples.
// Run: cd firmware && pio test -e native
#include <unity.h>
#include <math.h>
#include "input/imu_gestures.h"

using namespace input;

static ImuGestures g;
static uint32_t T;
static int counts[(int)Event::Count];

void setUp() { g = ImuGestures(); T = 1000; for (int& c : counts) c = 0; }
void tearDown() {}

// Default config: up = +Z, front = -Y, so right = up x front = +X.
static Vec3 rolled(float deg) {  // top of the dino tilted right by deg
  float r = deg / 57.2957795f;
  return {-sinf(r), 0, cosf(r)};
}
static void feed(Vec3 a, Vec3 gy, uint32_t ms) {
  for (uint32_t e = 0; e < ms; e += 20, T += 20) {
    g.update({a, gy, T});
    for (Event ev; (ev = g.next()) != Event::None;) counts[(int)ev]++;
  }
}
static void rest(uint32_t ms) { feed({0, 0, 1}, {0, 0, 0}, ms); }
static int total() { int s = 0; for (int c : counts) s += c; return s; }

void test_rest_is_quiet() {
  rest(5000);
  TEST_ASSERT_EQUAL(0, total());
  TEST_ASSERT_TRUE(g.state().resting);
}

void test_tilt_right_once() {
  rest(1000);
  feed(rolled(32), {0, 0, 0}, 300);
  rest(500);
  TEST_ASSERT_EQUAL(1, counts[(int)Event::TiltRight]);
  TEST_ASSERT_EQUAL(0, counts[(int)Event::TiltLeft]);
}

void test_tilt_left_repeats_while_held() {
  rest(1000);
  feed(rolled(-35), {0, 0, 0}, 1600);  // fires at 120 ms, then every 700 ms
  TEST_ASSERT_EQUAL(3, counts[(int)Event::TiltLeft]);
}

void test_small_tilt_ignored() {
  rest(1000);
  feed(rolled(15), {0, 0, 0}, 1000);
  TEST_ASSERT_EQUAL(0, total());
}

void test_flip_down_and_up() {
  rest(1000);
  feed({0, 1, 0}, {0, 0, 0}, 900);   // face-down reading = -front = +Y
  TEST_ASSERT_EQUAL(0, counts[(int)Event::FlipDown]);  // not held 1 s yet
  feed({0, 1, 0}, {0, 0, 0}, 300);
  TEST_ASSERT_EQUAL(1, counts[(int)Event::FlipDown]);
  TEST_ASSERT_TRUE(g.state().faceDown);
  rest(500);
  TEST_ASSERT_EQUAL(1, counts[(int)Event::FlipUp]);
}

void test_pick_up_then_put_down() {
  rest(1000);
  feed({0, 0, 1.3f}, {0, 50, 0}, 100);   // lift jolt
  feed({0.1f, 0, 1.05f}, {20, 30, 5}, 400);  // moving in the hand
  TEST_ASSERT_EQUAL(1, counts[(int)Event::PickUp]);
  rest(1200);
  TEST_ASSERT_EQUAL(1, counts[(int)Event::PutDown]);
}

void test_desk_bump_is_not_pick_up() {
  rest(1000);
  feed({0, 0, 1.3f}, {0, 0, 0}, 40);  // one knock, then still again
  rest(1000);
  TEST_ASSERT_EQUAL(0, counts[(int)Event::PickUp]);
}

void test_shake_once_with_cooldown() {
  rest(1000);
  for (int i = 0; i < 8; i++) { feed({2.2f, 0, 1}, {200, 0, 0}, 60); feed({-1.2f, 0, 1}, {-200, 0, 0}, 60); }
  TEST_ASSERT_EQUAL(1, counts[(int)Event::Shake]);  // 8 peaks in ~1 s, but cooldown 1.5 s
}

void test_front_from_face_down() {
  Vec3 f = ImuGestures::frontFromFaceDown({0, 0, 1}, {0.1f, 0.98f, 0.05f});
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.0f, f.z);
  TEST_ASSERT_TRUE(f.y < -0.98f);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_rest_is_quiet);
  RUN_TEST(test_tilt_right_once);
  RUN_TEST(test_tilt_left_repeats_while_held);
  RUN_TEST(test_small_tilt_ignored);
  RUN_TEST(test_flip_down_and_up);
  RUN_TEST(test_pick_up_then_put_down);
  RUN_TEST(test_desk_bump_is_not_pick_up);
  RUN_TEST(test_shake_once_with_cooldown);
  RUN_TEST(test_front_from_face_down);
  return UNITY_END();
}
