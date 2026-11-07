// Shard-face check: plays the 12 expressions of the new "Deskimon Shard Face" sheet (v2 rig) on the OLED.
//   pio run -e play_shard -t upload && pio device monitor -e play_shard
//
// The v2 face has no pupils, brows, nostrils or fang: each eye is four corner points plus a corner radius,
// the mouth is a 2-3 px stroke or a small filled shape. Expressions with the same shape type interpolate
// point by point (about 220 ms, ease in-out); hearts, spirals and arches swap during a blink (x1.3 time).
// While an expression rests it blinks, breathes (1 px hop every 1.5 s), hearts pulse and spirals spin,
// like the sheet's morph player.
//
// Controls: touch pad or Serial  n next   p previous   t auto tour on/off   s stats   ? list
//
// Stand-alone on purpose (not part of the face rig in src/face): this is a look-and-feel test of the new
// expressions. Fold it into the rig once the design is chosen. Everything above the ARDUINO line is pure C++
// (no Arduino calls), so it also compiles on the desktop.
//
// Fidelity (fact, measured 2026-11-06 against the sheet's own canvas code run in headless Chrome, resting frame
// at t = 1 s): the sheet keeps pixels whose canvas alpha is above 110/255; here every pixel is sampled 3x3 and
// kept at 4 of 9 or more. 0-18 pixels differ per expression (of 264-878 lit), all on edges; Neutral and Sleepy
// match exactly. Not bit-identical, so do not use it as the reference for a face_check-style diff.
#include <math.h>
#include <stdint.h>
#include <string.h>

namespace shard {

constexpr int W = 128, H = 64;
constexpr int SS = 3;        // sub-samples per pixel side
constexpr int ON_MIN = 4;    // of SS*SS: 4/9 = 0.44 coverage, the sheet's alpha > 110/255 (0.43)
constexpr float MORPH_MS = 220.0f;  // sheet default, "morphMs"
constexpr uint32_t TOUR_MS = 1600;  // sheet's auto tour step

struct Pt { float x, y; };
enum Kind : uint8_t { NONE, FILL, STROKE, HEART, SPIRAL };
struct Part {
  Kind k = NONE;
  Pt p[4] = {};
  float r = 0;      // FILL: corner radius
  float w = 0;      // STROKE: line width
  float sy = 1;     // vertical squash while a blink swap closes / opens the shape
  float sc = 1;     // HEART pulse scale
  float rot = 0;    // SPIRAL angle (radians)
};
struct Face {
  Part L, R, M;
  Pt lc, rc, mc;    // eye and mouth centres
  float oy = 0;     // whole-face vertical offset (breathing)
  float bl = 1;     // blink: eye height factor, 1 open .. 0 closed
};

// ---------------------------------------------------------------- expression data (from the sheet)
static Part mk(Kind k, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float rw) {
  Part q;
  q.k = k;
  q.p[0] = {x0, y0}; q.p[1] = {x1, y1}; q.p[2] = {x2, y2}; q.p[3] = {x3, y3};
  if (k == FILL) q.r = rw; else q.w = rw;
  return q;
}
static Part heart() { Part q; q.k = HEART; return q; }
static Part spiral() { Part q; q.k = SPIRAL; return q; }

enum Id : uint8_t { SLY, NEUTRAL, HAPPY, EXCITED, ANGRY, SAD, SURPRISED, SLEEPY, SUSPICIOUS, LOVE, DIZZY, WINK, LINE, COUNT };
constexpr int TOUR = LINE;  // the tour and the n/p keys cover SLY..WINK; LINE (sleep line) is only a morph target of SLEEPY

static const char* const LABEL[COUNT] = {"Default", "Neutral", "Happy", "Excited", "Angry", "Sad", "Surprised",
                                         "Sleepy", "Suspicious", "Love", "Dizzy", "Mischievous", "Sleep line"};

static Face F[COUNT];
static Pt SPIRAL_PTS[110];  // the spiral polyline, unrotated
constexpr int SPIRAL_N = 110;

static Part mirrored(const Part& a) {
  Part q = a;
  if (a.k == FILL || a.k == STROKE) for (int i = 0; i < 4; i++) q.p[i].x = -a.p[i].x;
  return q;
}

static void defineFace(Id id, Part L, const Part* R, Part M, Pt lc = {40, 26}, Pt rc = {88, 26}) {
  Face& f = F[id];
  f.L = L;
  f.R = R ? *R : mirrored(L);
  f.M = M;
  f.lc = lc; f.rc = rc; f.mc = {64, 47};
}

void init() {
  // mouths: four points, stroke width (or corner radius when filled)
  const Part smirk = mk(STROKE, -9, -3, -6, 1, 6, 1, 9, -3, 3);
  const Part flat = mk(STROKE, -6, 0, -2, 0, 2, 0, 6, 0, 2);
  const Part smile = mk(STROKE, -12, -4, -7, 2, 7, 2, 12, -4, 3);
  const Part smileS = mk(STROKE, -9, -3, -5, 2, 5, 2, 9, -3, 3);
  const Part open = mk(FILL, -9, -3, 9, -3, 6, 5, -6, 5, 3);
  const Part grr = mk(STROKE, -8, 2, -3, 0, 3, 0, 8, 2, 3);
  const Part frown = mk(STROKE, -7, 3, -4, -1, 4, -1, 7, 3, 2);
  const Part o = mk(FILL, -3, -4, 3, -4, 3, 4, -3, 4, 3);
  const Part tiny = mk(STROKE, -3, 0, -1, 0, 1, 0, 3, 0, 2);
  const Part side = mk(STROKE, 1, 1, 5, 1, 8, 0, 11, -2, 2);
  const Part wave = mk(STROKE, -8, 1, -3, -2, 3, 2, 8, -1, 2);
  const Part wink = mk(STROKE, -8, 0, -4, 1, 5, 0, 10, -4, 3);
  const Part slyEye = mk(FILL, -12, -8, 12, 2, 7, 8, -8, 8, 2);

  defineFace(SLY, slyEye, nullptr, smirk);
  defineFace(NEUTRAL, mk(FILL, -10, -10, 10, -10, 10, 10, -10, 10, 4), nullptr, flat);
  defineFace(HAPPY, mk(STROKE, -10, 4, -6, -3, 6, -3, 10, 4, 4), nullptr, smile);
  defineFace(EXCITED, mk(FILL, -8, -13, 8, -13, 8, 13, -8, 13, 6), nullptr, open);
  defineFace(ANGRY, mk(FILL, -12, -4, 12, 6, 11, 10, -12, 10, 1), nullptr, grr, {42, 27}, {86, 27});
  defineFace(SAD, mk(FILL, -12, 0, 11, -8, 11, 8, -10, 8, 2), nullptr, frown);
  defineFace(SURPRISED, mk(FILL, -11, -11, 11, -11, 11, 11, -11, 11, 11), nullptr, o);
  defineFace(SLEEPY, mk(FILL, -11, 0, 11, 0, 10, 6, -10, 6, 2), nullptr, tiny, {40, 30}, {88, 30});
  const Part susR = mk(FILL, -10, -8, 10, -6, 10, 8, -10, 8, 3);  // the one explicit right eye
  defineFace(SUSPICIOUS, mk(FILL, -11, -2, 11, -1, 11, 6, -11, 6, 2), &susR, side);
  defineFace(LOVE, heart(), nullptr, smileS);
  defineFace(DIZZY, spiral(), nullptr, wave);
  const Part winkR = mk(STROKE, -9, 2, -5, -2, 5, -2, 9, 2, 3);
  defineFace(WINK, slyEye, &winkR, wink);
  defineFace(LINE, mk(FILL, -11, 2, 11, 2, 11, 4, -11, 4, 1), nullptr, tiny, {40, 30}, {88, 30});

  for (int i = 0; i < SPIRAL_N; i++) {  // a = 0, .12, .24 ... as in the sheet
    float a = i * 0.12f, rr = 1.0f + a * 0.72f;
    SPIRAL_PTS[i] = {cosf(a) * rr, sinf(a) * rr};
  }
}

// ---------------------------------------------------------------- morphing
static float lp(float a, float b, float t) { return a + (b - a) * t; }
static float ease(float t) { return t < 0.5f ? 2 * t * t : 1 - powf(-2 * t + 2, 2) / 2; }
static Pt lv(Pt a, Pt b, float t) { return {lp(a.x, b.x, t), lp(a.y, b.y, t)}; }
static bool compat(const Part& a, const Part& b) { return (a.k == FILL || a.k == STROKE) && a.k == b.k; }

static Part lerpPart(const Part& a, const Part& b, float t, bool squash) {
  Part o;
  if (compat(a, b)) {  // same shape type: move the four points, radius and width
    o = a;
    for (int i = 0; i < 4; i++) o.p[i] = lv(a.p[i], b.p[i], t);
    o.r = lp(a.r, b.r, t);
    o.w = lp(a.w, b.w, t);
    o.sy = 1;
    return o;
  }
  o = t < 0.5f ? a : b;
  o.sy = 1;
  if (!squash) return o;  // mouths just swap
  o.sy = t < 0.5f ? 1 - t * 2 : (t - 0.5f) * 2;  // eyes close, swap, open: the change is hidden in the blink
  return o;
}

static Face lerpFace(const Face& A, const Face& B, float t) {
  Face f;
  f.L = lerpPart(A.L, B.L, t, true);
  f.R = lerpPart(A.R, B.R, t, true);
  f.M = lerpPart(A.M, B.M, t, false);
  f.lc = lv(A.lc, B.lc, t); f.rc = lv(A.rc, B.rc, t); f.mc = lv(A.mc, B.mc, t);
  return f;
}

static float blink(float t, float period, float at) {
  float u = fmodf(t, period) - at;
  if (u < 0 || u > 0.16f) return 1;
  return fabsf(u - 0.08f) / 0.08f;
}
static float breathe(float t) { return fmodf(t, 3.0f) < 1.5f ? 0.0f : 1.0f; }

// Resting animation of an expression at time t (seconds): hearts pulse twice per beat, spirals spin.
static Face resting(int id, float t) {
  Face f = F[id];
  f.bl = blink(t, 4, 2.6f);
  f.oy = breathe(t);
  if (id == LOVE) {
    float b = fmodf(t, 1.2f);
    float sc = b < 0.12f ? 1.15f : b < 0.24f ? 1.0f : b < 0.36f ? 1.12f : 1.0f;
    f.L.sc = f.R.sc = sc;
  } else if (id == DIZZY) {
    f.L.rot = t * 5;
    f.R.rot = t * 5 + 1.5f;
  }
  return f;
}

// The face `sinceMs` after a switch from `from` to `to`; `t` is the clock in seconds (drives blink and pulses).
Face frameAt(int from, int to, uint32_t sinceMs, float t) {
  const Face &A = F[from], &B = F[to];
  float dur = MORPH_MS * (compat(A.L, B.L) && compat(A.R, B.R) ? 1.0f : 1.3f);
  float u = sinceMs / dur;
  if (u >= 1) return resting(to, t);
  return lerpFace(A, B, ease(u));
}

// ---------------------------------------------------------------- drawing
// One-bit frame in the SSD1306 page layout (byte = 8 vertical pixels), so it is copied straight into the display.
uint8_t fb[W * H / 8];
static inline void setPx(int x, int y) { fb[(y >> 3) * W + x] |= (uint8_t)(1 << (y & 7)); }

struct Poly {  // a flattened rounded polygon in the eye's own coordinates
  Pt v[48];
  int n = 0;
  void add(Pt p) { if (n < 48) v[n++] = p; }
};

// Canvas arcTo(): line to the first tangent point, then a circular arc of radius r to the second one.
static void arcTo(Poly& o, Pt p0, Pt p1, Pt p2, float r) {
  Pt d0 = {p0.x - p1.x, p0.y - p1.y}, d1 = {p2.x - p1.x, p2.y - p1.y};
  float l0 = hypotf(d0.x, d0.y), l1 = hypotf(d1.x, d1.y);
  if (r <= 0 || l0 < 1e-6f || l1 < 1e-6f) { o.add(p1); return; }
  d0.x /= l0; d0.y /= l0; d1.x /= l1; d1.y /= l1;
  float cross = d0.x * d1.y - d0.y * d1.x, dotp = d0.x * d1.x + d0.y * d1.y;
  if (fabsf(cross) < 1e-6f) { o.add(p1); return; }  // straight: no corner to round
  float theta = acosf(fmaxf(-1.0f, fminf(1.0f, dotp)));
  float t = r / tanf(theta / 2);
  Pt t0 = {p1.x + d0.x * t, p1.y + d0.y * t}, t1 = {p1.x + d1.x * t, p1.y + d1.y * t};
  Pt bis = {d0.x + d1.x, d0.y + d1.y};
  float bl = hypotf(bis.x, bis.y);
  float cd = r / sinf(theta / 2);
  Pt c = {p1.x + bis.x / bl * cd, p1.y + bis.y / bl * cd};
  float a0 = atan2f(t0.y - c.y, t0.x - c.x), a1 = atan2f(t1.y - c.y, t1.x - c.x);
  float da = a1 - a0;
  while (da > (float)M_PI) da -= 2 * (float)M_PI;
  while (da < -(float)M_PI) da += 2 * (float)M_PI;
  int steps = (int)ceilf(fabsf(da) / ((float)M_PI / 8));
  if (steps < 1) steps = 1;
  o.add(t0);
  for (int i = 1; i <= steps; i++) {
    float a = a0 + da * i / steps;
    o.add({c.x + cosf(a) * r, c.y + sinf(a) * r});
  }
}

static void flatten(Poly& o, const Part& s) {  // the sheet's rpoly()
  for (int i = 0; i < 4; i++) {
    Pt a = s.p[(i + 3) & 3], b = s.p[i], c = s.p[(i + 1) & 3];
    float rr = fmaxf(0.0f, fminf(s.r, fminf(hypotf(b.x - a.x, b.y - a.y) / 2, hypotf(c.x - b.x, c.y - b.y) / 2)));
    if (i == 0) o.add({(a.x + b.x) / 2, (a.y + b.y) / 2});
    arcTo(o, o.v[o.n - 1], b, c, rr);
  }
}

static float segDist2(Pt p, Pt a, Pt b) {
  float vx = b.x - a.x, vy = b.y - a.y, wx = p.x - a.x, wy = p.y - a.y;
  float L = vx * vx + vy * vy, t = L > 0 ? (wx * vx + wy * vy) / L : 0;
  t = t < 0 ? 0 : t > 1 ? 1 : t;
  float dx = wx - t * vx, dy = wy - t * vy;
  return dx * dx + dy * dy;
}

static const char* const HEART_ROWS[9] = {".111...111.", "11111.11111", "11111111111", "11111111111", ".111111111.",
                                     "..1111111..", "...11111...", "....111....", ".....1....."};

// Is the local point (x, y) inside a stroke, heart or spiral part?
static bool hit(const Part& s, float x, float y) {
  if (s.k == STROKE) {
    float h2 = s.w * s.w / 4;
    for (int i = 0; i < 3; i++) if (segDist2({x, y}, s.p[i], s.p[i + 1]) <= h2) return true;
    return false;
  }
  if (s.k == HEART) {  // 11 x 9 cells of 2 px, scaled by the pulse
    int col = (int)floorf(x / s.sc / 2 + 5.5f), row = (int)floorf(y / s.sc / 2 + 4.5f);
    return col >= 0 && col < 11 && row >= 0 && row < 9 && HEART_ROWS[row][col] == '1';
  }
  // spiral: undo the rotation, then only test the segments whose radius is near this point's radius
  float cs = cosf(s.rot), sn = sinf(s.rot);
  Pt q = {x * cs + y * sn, -x * sn + y * cs};
  float rho = hypotf(q.x, q.y);
  int lo = (int)floorf((rho - 2.5f) / 0.0864f) - 1, hi = (int)ceilf((rho + 0.5f) / 0.0864f) + 1;
  if (lo < 0) lo = 0;
  if (hi > SPIRAL_N - 2) hi = SPIRAL_N - 2;
  for (int i = lo; i <= hi; i++) if (segDist2(q, SPIRAL_PTS[i], SPIRAL_PTS[i + 1]) <= 1.0f) return true;  // width 2
  return false;
}

static float extent(const Part& s) {  // how far the part reaches from its centre, in its own units
  if (s.k == HEART) return 12.0f * s.sc;
  if (s.k == SPIRAL) return 12.0f;
  float m = 0;
  for (int i = 0; i < 4; i++) m = fmaxf(m, fmaxf(fabsf(s.p[i].x), fabsf(s.p[i].y)));
  return m + (s.k == STROKE ? s.w / 2 + 1 : 1);
}

static void drawPart(const Part& s, float cx, float cy, float syIn) {
  if (s.k == NONE) return;
  const float sy = fmaxf(syIn, 0.14f);                 // as the sheet: never thinner than 14%
  const float tx = floorf(cx + 0.5f), ty = floorf(cy + 0.5f);  // the sheet snaps the centre to a pixel
  const float e = extent(s);
  int x0 = (int)floorf(tx - e), x1 = (int)ceilf(tx + e), y0 = (int)floorf(ty - e * sy), y1 = (int)ceilf(ty + e * sy);
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > W - 1) x1 = W - 1;
  if (y1 > H - 1) y1 = H - 1;

  if (s.k == FILL) {  // scanline: edge crossings per sub-row, then count sub-samples per pixel
    Poly poly;
    flatten(poly, s);
    for (int Y = y0; Y <= y1; Y++) {
      uint8_t cov[W] = {};
      for (int j = 0; j < SS; j++) {
        float ly = (Y + (j + 0.5f) / SS - ty) / sy;
        float xs[16];
        int m = 0;
        for (int i = 0; i < poly.n; i++) {
          Pt a = poly.v[i], b = poly.v[(i + 1) % poly.n];
          if ((a.y <= ly) != (b.y <= ly) && m < 16) xs[m++] = tx + a.x + (ly - a.y) * (b.x - a.x) / (b.y - a.y);
        }
        for (int i = 1; i < m; i++) for (int k = i; k > 0 && xs[k] < xs[k - 1]; k--) { float t = xs[k]; xs[k] = xs[k - 1]; xs[k - 1] = t; }
        for (int q = 0; q + 1 < m; q += 2) {
          int ka = (int)ceilf(xs[q] * SS - 0.5f), kb = (int)ceilf(xs[q + 1] * SS - 0.5f) - 1;
          if (ka < 0) ka = 0;
          if (kb > W * SS - 1) kb = W * SS - 1;
          for (int k = ka; k <= kb; k++) cov[k / SS]++;
        }
      }
      for (int X = x0; X <= x1; X++) if (cov[X] >= ON_MIN) setPx(X, Y);
    }
    return;
  }
  for (int Y = y0; Y <= y1; Y++)
    for (int X = x0; X <= x1; X++) {
      int n = 0;
      for (int j = 0; j < SS; j++)
        for (int i = 0; i < SS; i++) n += hit(s, X + (i + 0.5f) / SS - tx, (Y + (j + 0.5f) / SS - ty) / sy);
      if (n >= ON_MIN) setPx(X, Y);
    }
}

void drawFace(const Face& f) {
  memset(fb, 0, sizeof fb);
  drawPart(f.L, f.lc.x, f.lc.y + f.oy, f.L.sy * f.bl);
  drawPart(f.R, f.rc.x, f.rc.y + f.oy, f.R.sy * f.bl);
  drawPart(f.M, f.mc.x, f.mc.y + f.oy, 1);
}

}  // namespace shard

// ---------------------------------------------------------------- hardware (ESP32-C3 + SSD1306)
#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

static Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
constexpr uint32_t FRAME_MS = 33;       // the sheet draws at about 30 fps
constexpr uint32_t TOUCH_LOCK_MS = 300;  // one step per touch

static int cur = shard::SLY, from = shard::SLY;
static uint32_t switchedAt = 0, lastFrame = 0, lastTouch = 0, lastStep = 0;
static bool tour = false, touchWas = false;
// timing of the expression on screen
static uint32_t nFrames, sumRender, worstRender, worstShow, nLate;

static void stats() {
  if (!nFrames) return;
  Serial.printf("  %-12s %lu frames: render avg %lu us, worst %lu us, display() worst %lu us, over %lu ms: %lu\n",
                shard::LABEL[cur], (unsigned long)nFrames, (unsigned long)(sumRender / nFrames), (unsigned long)worstRender,
                (unsigned long)worstShow, (unsigned long)FRAME_MS, (unsigned long)nLate);
  nFrames = sumRender = worstRender = worstShow = nLate = 0;
}

static void go(int id) {
  id = (id + shard::TOUR) % shard::TOUR;
  stats();
  from = cur;
  cur = id;
  switchedAt = lastStep = millis();
  const shard::Face &A = shard::F[from], &B = shard::F[cur];
  Serial.printf("[%d/%d] %s -> %s (%s)\n", cur + 1, shard::TOUR, shard::LABEL[from], shard::LABEL[cur],
                (A.L.k == B.L.k && (A.L.k == shard::FILL || A.L.k == shard::STROKE)) ? "point morph" : (from == cur ? "-" : "blink swap"));
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_FREQ_HZ);
  pinMode(PIN_TOUCH, INPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed - check wiring/address");
    for (;;) delay(1000);
  }
  shard::init();
  Serial.println("shard face player: touch or n = next, p = previous, t = auto tour, s = stats, ? = list");
  Serial.printf("[1/%d] %s\n", shard::TOUR, shard::LABEL[cur]);
}

void loop() {
  uint32_t now = millis();
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == 'n') go(cur + 1);
    else if (c == 'p') go(cur - 1);
    else if (c == 't') { tour = !tour; lastStep = now; Serial.printf("auto tour %s\n", tour ? "on" : "off"); }
    else if (c == 's') stats();
    else if (c == '?') for (int i = 0; i < shard::TOUR; i++) Serial.printf("  %2d %s%s\n", i + 1, shard::LABEL[i], i == cur ? "  <" : "");
  }
  bool touched = digitalRead(PIN_TOUCH) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW);
  if (touched && !touchWas && now - lastTouch > TOUCH_LOCK_MS) { lastTouch = now; go(cur + 1); }
  touchWas = touched;
  if (tour && now - lastStep >= shard::TOUR_MS) go(cur + 1);

  if (now - lastFrame < FRAME_MS) { delay(1); return; }
  lastFrame = now;
  uint32_t t0 = micros();
  shard::drawFace(shard::frameAt(from, cur, now - switchedAt, now / 1000.0f));
  uint32_t render = micros() - t0;
  memcpy(display.getBuffer(), shard::fb, sizeof shard::fb);
  t0 = micros();
  display.display();
  uint32_t show = micros() - t0;
  nFrames++; sumRender += render;
  if (render > worstRender) worstRender = render;
  if (show > worstShow) worstShow = show;
  if ((render + show) / 1000 > FRAME_MS) nLate++;
}
#endif  // ARDUINO
