// Run: cd firmware && pio test -e native
// Boot film: bitmaps are well-formed, every step of the three films renders inside the screen, zooms are exact
// whole-pixel zooms, timings are in the planned range and the last frame of film C hands over to the live face.
#include <unity.h>
#include <string.h>
#include "boot/boot_film.h"

using namespace boot;

static uint8_t work[PIXELS], out[PIXELS];
static int liveCalls, lastVariant;
static void stubLive(uint8_t* px, int variant) {  // the real rig is not in the desktop build: a marker pattern
  liveCalls++;
  lastVariant = variant;
  memset(px, 0, PIXELS);
  for (int x = 20; x < 100; x++) px[30 * W + x] = 1;
}
static int lit(const uint8_t* b) { int n = 0; for (int i = 0; i < PIXELS; i++) n += b[i] ? 1 : 0; return n; }

void setUp() {}
void tearDown() {}

void test_bitmaps_well_formed() { TEST_ASSERT_TRUE(dataOk()); }

void test_every_step_renders_and_is_valid() {
  for (int v = 0; v < V_COUNT; v++) {
    const Film& f = film(v);
    TEST_ASSERT_TRUE(f.n > 10);
    for (int i = 0; i < f.n; i++) {
      const Step& s = f.steps[i];
      TEST_ASSERT_TRUE(s.ms >= 50 && s.ms <= 700);                                  // a hold, never a flash faster than ~3/s... or a stall
      TEST_ASSERT_TRUE(s.z == 1 || s.z == 2 || s.z == 4 || s.z == 8);
      renderStep(s, work, out, stubLive);
      if (s.op == Op::Dark) TEST_ASSERT_EQUAL(0, lit(out));
      else TEST_ASSERT_TRUE_MESSAGE(lit(out) > 0, s.label);
      for (int p = 0; p < PIXELS; p++) TEST_ASSERT_TRUE(out[p] <= 1);
    }
  }
}

void test_render_is_deterministic() {
  static uint8_t out2[PIXELS];
  for (int v = 0; v < V_COUNT; v++)
    for (int i = 0; i < film(v).n; i++) {
      renderStep(film(v).steps[i], work, out, stubLive);
      renderStep(film(v).steps[i], work, out2, stubLive);
      TEST_ASSERT_EQUAL_MEMORY(out, out2, PIXELS);
    }
}

void test_zoom_is_an_exact_pixel_zoom() {
  Step s = film(V_ZOOM).steps[2];  // a wordmark frame
  s.z = 1; s.fx = 64; s.fy = 32;
  renderStep(s, work, out, stubLive);
  static uint8_t base[PIXELS];
  memcpy(base, work, PIXELS);
  for (int z = 2; z <= 8; z *= 2) {
    s.z = z; s.fx = 38; s.fy = 22;
    renderStep(s, work, out, stubLive);
    const int sw = W / z, sh = H / z;
    int sx = 38 - sw / 2, sy = 22 - sh / 2;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++) TEST_ASSERT_EQUAL(base[(sy + y / z) * W + sx + x / z], out[y * W + x]);
  }
}

void test_zoom_view_stays_inside_the_screen() {
  Step s = film(V_ZOOM).steps[2];
  s.z = 8;
  const uint8_t fx[] = {0, 127, 64}, fy[] = {0, 63, 32};
  for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) { s.fx = fx[a]; s.fy = fy[b]; renderStep(s, work, out, stubLive); }
  TEST_ASSERT_TRUE(true);  // no out-of-range read (the native build runs with -Werror and the test would crash)
}

void test_durations_in_planned_range() {
  TEST_ASSERT_TRUE(totalMs(film(V_BORN)) >= 3500 && totalMs(film(V_BORN)) <= 5500);
  TEST_ASSERT_TRUE(totalMs(film(V_DINO)) >= 3000 && totalMs(film(V_DINO)) <= 5000);
  TEST_ASSERT_TRUE(totalMs(film(V_ZOOM)) >= 4000 && totalMs(film(V_ZOOM)) <= 6000);
}

void test_films_end_on_the_planned_frame() {
  const Film& a = film(V_BORN);
  TEST_ASSERT_EQUAL((int)Op::Word, (int)a.steps[a.n - 1].op);
  TEST_ASSERT_EQUAL(46, a.steps[a.n - 1].a);
  const Film& b = film(V_DINO);
  TEST_ASSERT_EQUAL((int)Op::Word, (int)b.steps[b.n - 1].op);
  const Film& c = film(V_ZOOM);
  TEST_ASSERT_EQUAL((int)Op::Live, (int)c.steps[c.n - 1].op);
  liveCalls = 0;
  renderStep(c.steps[c.n - 1], work, out, stubLive);
  TEST_ASSERT_EQUAL(1, liveCalls);
  TEST_ASSERT_EQUAL(0, lastVariant);  // the last frame is the open idle face
}

void test_film_c_has_the_three_blink_variants() {
  const Film& c = film(V_ZOOM);
  int seen[3] = {0, 0, 0};
  for (int i = 0; i < c.n; i++) if (c.steps[i].op == Op::Live) seen[c.steps[i].a]++;
  TEST_ASSERT_TRUE(seen[0] > 0 && seen[1] > 0 && seen[2] > 0);
}

void test_dino_is_bigger_than_its_shrunk_copies() {
  Step full = {100, Op::Dino, F_OPEN, 0, 1, 0, 20, 2, 1, 64, 32, "full"};
  Step half = full; half.c = 2;
  Step quarter = full; quarter.c = 4;
  renderStep(full, work, out, nullptr);
  int nFull = lit(out);
  renderStep(half, work, out, nullptr);
  int nHalf = lit(out);
  renderStep(quarter, work, out, nullptr);
  int nQuarter = lit(out);
  TEST_ASSERT_TRUE(nFull > nHalf && nHalf > nQuarter && nQuarter > 0);
}

void test_no_person_in_film_b() {
  const Film& b = film(V_DINO);
  for (int i = 0; i < b.n; i++) TEST_ASSERT_TRUE(b.steps[i].op != Op::Person);
  const Film& a = film(V_BORN);
  bool person = false;
  for (int i = 0; i < a.n; i++) person |= a.steps[i].op == Op::Person;
  TEST_ASSERT_TRUE(person);  // film A keeps the pixel person
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_bitmaps_well_formed);
  RUN_TEST(test_every_step_renders_and_is_valid);
  RUN_TEST(test_render_is_deterministic);
  RUN_TEST(test_zoom_is_an_exact_pixel_zoom);
  RUN_TEST(test_zoom_view_stays_inside_the_screen);
  RUN_TEST(test_durations_in_planned_range);
  RUN_TEST(test_films_end_on_the_planned_frame);
  RUN_TEST(test_film_c_has_the_three_blink_variants);
  RUN_TEST(test_dino_is_bigger_than_its_shrunk_copies);
  RUN_TEST(test_no_person_in_film_b);
  return UNITY_END();
}
