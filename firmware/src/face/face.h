// Dino face rig (ported from the design sheet "Deskimon Dino Sheet", art/reference/dino-v1/rig.js).
// Pure C++ (no Arduino calls) so it is checked on the desktop against the design's reference
// frames: see tools/face_check.py.
//
// A frame is a 128x64 1-bit Grid. Everything is drawn by small functions that write into it:
// shape fills (area/ring/edge), strokes, pixel fonts, the parametric face (two eyes, brows,
// nostrils, mouth, effects) and props. The face is described by FaceO (only the fields you
// set) which resolve() applies on top of a named expression preset.
#pragma once
#include <stdint.h>
#include <math.h>
#include "face_assets.h"

#ifndef RIG_REAL
#define RIG_REAL double  // the design computes in double; float also matches (tools/face_check.py checks both)
#endif

namespace face {

typedef RIG_REAL real;  // per-pixel maths (float on the chip)
typedef double dreal;   // parameter maths: same as the design (double), only evaluated a few times per frame
constexpr int W = 128, H = 64;

struct Grid {
  uint8_t d[W * H];
  void clear() { for (int i = 0; i < W * H; i++) d[i] = 0; }
};

// JS Math.round: halves round up (C round() rounds them away from zero).
inline int jr(dreal x) { return (int)floor(x + 0.5); }

// ---- pixels, lines, shapes ----
void px(Grid& g, dreal x, dreal y, uint8_t v = 1);
void line(Grid& g, dreal x0, dreal y0, dreal x1, dreal y1, uint8_t v = 1);
struct B { dreal a, b, c, d; };  // bounds x0,y0,x1,y1 (inclusive)
inline B clampB(dreal a, dreal b, dreal c, dreal d) {
  B o;
  o.a = a < 0 ? 0 : floor(a); o.b = b < 0 ? 0 : floor(b);
  o.c = ceil(c) > W - 1 ? W - 1 : ceil(c); o.d = ceil(d) > H - 1 ? H - 1 : ceil(d);
  return o;
}
inline B bE(dreal cx, dreal cy, dreal rx, dreal ry, dreal p = 1) { return {cx - rx - p, cy - ry - p, cx + rx + p, cy + ry + p}; }

template <class F> void area(Grid& g, F f, B b, uint8_t v = 1) {
  B c = clampB(b.a, b.b, b.c, b.d);
  for (int y = (int)c.b; y <= (int)c.d; y++) for (int x = (int)c.a; x <= (int)c.c; x++) if (f((real)x + (real)0.5, (real)y + (real)0.5)) g.d[y * W + x] = v;
}
// ring/edge test the shape once per pixel (three rolling rows of results) instead of five times:
// the border test needs the neighbours, which are the previous/next rows and columns of the same grid.
// Same pixels as testing f(X+-1,Y), f(X,Y+-1) directly (checked by tools/face_check.py).
template <class F, class Put> void borderPass(F f, B b, Put put) {
  B c = clampB(b.a, b.b, b.c, b.d);
  const int x0 = (int)c.a, x1 = (int)c.c, y0 = (int)c.b, y1 = (int)c.d;
  const int n = x1 - x0 + 3;  // columns x0-1 .. x1+1
  uint8_t rows[3][W + 2];
  auto fill = [&](uint8_t* r, int y) {
    for (int i = 0; i < n; i++) r[i] = f((real)(x0 - 1 + i) + (real)0.5, (real)y + (real)0.5) ? 1 : 0;
  };
  fill(rows[0], y0 - 1); fill(rows[1], y0);
  // rolling rows: prev / cur / next
  uint8_t *prev = rows[0], *cur = rows[1], *next = rows[2];
  for (int y = y0; y <= y1; y++) {
    fill(next, y + 1);
    for (int x = x0; x <= x1; x++) {
      const int i = x - x0 + 1;
      if (cur[i]) put(x, y, !(cur[i + 1] && cur[i - 1] && next[i] && prev[i]));
    }
    uint8_t* t = prev; prev = cur; cur = next; next = t;
  }
}
template <class F> void ring(Grid& g, F f, B b) {  // filled shape, 1 on the border, 0 inside
  borderPass(f, b, [&](int x, int y, bool border) { g.d[y * W + x] = border ? 1 : 0; });
}
template <class F> void edge(Grid& g, F f, B b) {  // only the border pixels
  borderPass(f, b, [&](int x, int y, bool border) { if (border) g.d[y * W + x] = 1; });
}
inline void fillRect(Grid& g, dreal x0, dreal y0, dreal x1, dreal y1, uint8_t v = 1) {
  area(g, [](real, real) { return true; }, B{x0, y0, x1, y1}, v);
}
inline auto ell(dreal cx_, dreal cy_, dreal rx_, dreal ry_) {
  const real cx = (real)cx_, cy = (real)cy_, rx = (real)rx_, ry = (real)ry_;
  return [=](real x, real y) { return ((x - cx) / rx) * ((x - cx) / rx) + ((y - cy) / ry) * ((y - cy) / ry) <= 1; };
}
inline auto rr(dreal cx_, dreal cy_, dreal hw_, dreal hh_, dreal r_) {
  const real cx = (real)cx_, cy = (real)cy_, hw = (real)hw_, hh = (real)hh_, r = (real)r_;
  return [=](real x, real y) {
    real ax = fabs(x - cx), ay = fabs(y - cy);
    if (ax > hw || ay > hh) return false;
    real qx = fmax(ax - (hw - r), 0), qy = fmax(ay - (hh - r), 0);
    return qx * qx + qy * qy <= r * r;
  };
}
void dot(Grid& g, dreal x, dreal y, dreal r, uint8_t v = 1);
void nib(Grid& g, dreal x, dreal y, int th);

struct Pts {  // a polyline (at most 40 points)
  dreal x[40], y[40];
  int n = 0;
  void add(dreal px_, dreal py_) { if (n < 40) { x[n] = px_; y[n] = py_; n++; } }
};
void stroke(Grid& g, const Pts& p, int th);
template <class F> Pts curve(dreal mx, dreal my, dreal hw, F fn, int n = 24) {
  Pts o;
  for (int i = 0; i <= n; i++) { dreal u = -1.0 + 2.0 * i / n; o.add(mx + u * hw, my + fn(u)); }
  return o;
}
void bmp(Grid& g, const char* const* rows, int nrows, dreal x0, dreal y0, uint8_t v = 1);
void plus(Grid& g, dreal x, dreal y);
void zee(Grid& g, dreal x, dreal y, bool big);

// ---- text ----
enum class Font : uint8_t { Big, Small, Txt };  // big 9x13 digits, small 4x7, text 5x7 uppercase
int text(Grid& g, const char* s, dreal x, dreal y, Font f = Font::Txt, int sc = 1, uint8_t v = 1);
int textW(const char* s, Font f = Font::Txt, int sc = 1);
void textC(Grid& g, const char* s, dreal cx, dreal y, Font f = Font::Txt, int sc = 1, uint8_t v = 1);

// ---- the face ----
enum class Shape : uint8_t { Unset, Slab, ArcDown, ArcUp, Chev, X };
enum class Pup : uint8_t { Unset, Round, Tiny, Slit, Heart, Spiral, Star };
enum class Mouth : uint8_t { Unset, None, Line, Smile, Flat, Frown, Grin, Rawr, ChompOpen, O, OSmall, Yawn, Grr, Wobble, Smirk, Tongue, Chomp, Drool };

constexpr dreal UNSET = NAN;
inline bool has(dreal v) { return v == v; }

// Eye overrides: only the fields you set. Chain the setters: EyeO().lt(.3).sl(.5).brow(4)
// (setters change the object in place and return it: a by-value chain made ~350 B temporaries per call and blew the stack)
struct EyeO {
  dreal w_ = UNSET, h_ = UNSET, r_ = UNSET, pr_ = UNSET, py_ = UNSET, px_ = UNSET, lt_ = UNSET, lb_ = UNSET, sl_ = UNSET, sm_ = UNSET;
  Shape shape_ = Shape::Unset;
  Pup pup_ = Pup::Unset;
  bool hasBrow_ = false;
  dreal ba_ = 0, bl_ = 0;
  EyeO& w(dreal v) { w_ = v; return *this; }
  EyeO& h(dreal v) { h_ = v; return *this; }
  EyeO& r(dreal v) { r_ = v; return *this; }
  EyeO& pr(dreal v) { pr_ = v; return *this; }
  EyeO& py(dreal v) { py_ = v; return *this; }
  EyeO& px(dreal v) { px_ = v; return *this; }
  EyeO& lt(dreal v) { lt_ = v; return *this; }
  EyeO& lb(dreal v) { lb_ = v; return *this; }
  EyeO& sl(dreal v) { sl_ = v; return *this; }
  EyeO& sm(dreal v) { sm_ = v; return *this; }
  EyeO& shape(Shape v) { shape_ = v; return *this; }
  EyeO& pup(Pup v) { pup_ = v; return *this; }
  EyeO& brow(dreal a, dreal lift = 0) { hasBrow_ = true; ba_ = a; bl_ = lift; return *this; }
};

// A fully resolved eye (what eye() draws).
struct EyeP {
  dreal w = 28, h = 30, r = 10, pr = 5, py = (dreal)0.12, px = 0, s = 1, lt = 0, lb = 0, sl = 0, sm = 0;
  Shape shape = Shape::Slab;
  Pup pup = Pup::Round;
  bool hasBrow = false;
  dreal ba = 0, bl = 0;
  void apply(const EyeO& o);
};

// Face overrides: only the fields you set. Numbers double as flags (0 = off).
struct FaceO {
  dreal s_ = UNSET, x_ = UNSET, y_ = UNSET, dx_ = UNSET, dy_ = UNSET, sp_ = UNSET, t_ = UNSET;
  dreal lookx_ = UNSET, looky_ = UNSET;
  dreal cheeks_ = UNSET, glasses_ = UNSET, nose_ = UNSET, mx_ = UNSET, my_ = UNSET, mw_ = UNSET;
  dreal z_ = UNSET, bang_ = UNSET, q_ = UNSET, heart_ = UNSET, note_ = UNSET, sweat_ = UNSET, tear_ = UNSET, steam_ = UNSET,
       sparkle_ = UNSET, crown_ = UNSET, dots_ = UNSET;
  Mouth m_ = Mouth::Unset;
  EyeO e_, l_, r_;  // E = both eyes, then L / R on top
  bool hasE_ = false, hasL_ = false, hasR_ = false;
  FaceO& s(dreal v) { s_ = v; return *this; }
  FaceO& x(dreal v) { x_ = v; return *this; }
  FaceO& y(dreal v) { y_ = v; return *this; }
  FaceO& dx(dreal v) { dx_ = v; return *this; }
  FaceO& dy(dreal v) { dy_ = v; return *this; }
  FaceO& t(dreal v) { t_ = v; return *this; }
  FaceO& look(dreal a, dreal b) { lookx_ = a; looky_ = b; return *this; }
  FaceO& cheeks(dreal v = 1) { cheeks_ = v; return *this; }
  FaceO& glasses(dreal v = 1) { glasses_ = v; return *this; }
  FaceO& mx(dreal v) { mx_ = v; return *this; }
  FaceO& mw(dreal v) { mw_ = v; return *this; }
  FaceO& z(dreal v) { z_ = v; return *this; }
  FaceO& bang(dreal v = 1) { bang_ = v; return *this; }
  FaceO& heart(dreal v = 1) { heart_ = v; return *this; }
  FaceO& note(dreal v = 1) { note_ = v; return *this; }
  FaceO& sweat(dreal v = 1) { sweat_ = v; return *this; }
  FaceO& tear(dreal v = 1) { tear_ = v; return *this; }
  FaceO& steam(dreal v = 1) { steam_ = v; return *this; }
  FaceO& sparkle(dreal v = 1) { sparkle_ = v; return *this; }
  FaceO& crown(dreal v = 1) { crown_ = v; return *this; }
  FaceO& dots(dreal v) { dots_ = v; return *this; }
  FaceO& m(Mouth v) { m_ = v; return *this; }
  FaceO& E(const EyeO& v) { e_ = v; hasE_ = true; return *this; }
  FaceO& L(const EyeO& v) { l_ = v; hasL_ = true; return *this; }
  FaceO& R(const EyeO& v) { r_ = v; hasR_ = true; return *this; }
  FaceO over(const FaceO& o) const;  // fields set in `o` win (used for the small-face anchor)
};

struct FaceP {  // fully resolved face
  EyeP L, R;
  dreal s = 1, x = 64, y = 27, dx = 0, dy = 0, sp = 24, t = 0, lookx = 0, looky = 0;
  dreal cheeks = 0, glasses = 0, nose = 1, mx = 0, my = 26, mw = 1;
  dreal z = 0, bang = 0, q = 0, heart = 0, note = 0, sweat = 0, tear = 0, steam = 0, sparkle = 0, crown = 0, dots = 0;
  Mouth m = Mouth::Line;
};

enum class Ex : uint8_t { Idle, Happy, Love, Curious, Sleepy, Asleep, Surprised, Grumpy, Rawr, Dizzy, Shy, Unimpressed, Mischief, Sad,
                          Focus, Excited, Hungry, Chew, Smug, Scared, Determined, Squeeze, Wink, Yawn, Count };
FaceP resolve(Ex preset, const FaceO& over = FaceO());

void eye(Grid& g, dreal cx, dreal cy, const EyeP& e, int side, dreal t);
void mouth(Grid& g, Mouth type, dreal mx, dreal my, dreal s, dreal mw);
void faceDraw(Grid& g, const FaceP& f);

// ---- props and pieces used by the screens ----
using Icon16 = Icon;
void stampIcon(Grid& g, Icon ic, dreal x0, dreal y0, uint8_t v = 1);
void card(Grid& g, int x0, int y0, int w, int h);
void alarmProp(Grid& g, int r, int x = 108, int y = 46);
void batt(Grid& g, int x, int y, int n);
void speed(Grid& g, int x, int y0, int y1);
void shiftG(const Grid& in, Grid& out, int dx, int dy);

}  // namespace face
