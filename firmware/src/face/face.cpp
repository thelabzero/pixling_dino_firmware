// Dino face rig: see face.h. A line-by-line port of the design's rig.js (art/reference/dino-v1/).
// Keep it identical to the design: tools/face_check.py compares every frame pixel by pixel.
#include "face.h"

namespace face {

static const char* const HEART7[] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
static const char* const DROP[] = {".#.", ".#.", "###", "###", ".#."};
static const char* const NOTE[] = {"..##", "..#.", "..#.", ".##.", "###.", ".#.."};
static const char* const CROWN[] = {"#..#..#", "##.#.##", "#######", "#######"};

// ---------- pixels, lines ----------
void px(Grid& g, dreal x, dreal y, uint8_t v) {
  int ix = jr(x), iy = jr(y);
  if (ix >= 0 && ix < W && iy >= 0 && iy < H) g.d[iy * W + ix] = v;
}

void line(Grid& g, dreal fx0, dreal fy0, dreal fx1, dreal fy1, uint8_t v) {
  int x0 = jr(fx0), y0 = jr(fy0), x1 = jr(fx1), y1 = jr(fy1);
  int dx = abs(x1 - x0), dy = -abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int e = dx + dy;
  for (;;) {
    px(g, x0, y0, v);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * e;
    if (e2 >= dy) { e += dy; x0 += sx; }
    if (e2 <= dx) { e += dx; y0 += sy; }
  }
}

void dot(Grid& g, dreal x, dreal y, dreal r, uint8_t v) {
  int R = (int)ceil(r);
  for (int dy = -R; dy <= R; dy++) for (int dx = -R; dx <= R; dx++) if (dx * dx + dy * dy <= r * r + 0.3) px(g, x + dx, y + dy, v);
}

void nib(Grid& g, dreal fx, dreal fy, int th) {
  dreal x = jr(fx), y = jr(fy);
  if (th <= 1) px(g, x, y);
  else if (th == 2) { px(g, x, y); px(g, x + 1, y); px(g, x, y + 1); px(g, x + 1, y + 1); }
  else { px(g, x, y); px(g, x - 1, y); px(g, x + 1, y); px(g, x, y - 1); px(g, x, y + 1); }
}

void stroke(Grid& g, const Pts& p, int th) {
  if (p.n == 1) nib(g, p.x[0], p.y[0], th);
  for (int i = 0; i < p.n - 1; i++) {
    dreal a = p.x[i], b = p.y[i], c = p.x[i + 1], d = p.y[i + 1];
    int n = (int)ceil(hypot(c - a, d - b) * 2);
    if (n < 1) n = 1;
    for (int k = 0; k <= n; k++) nib(g, a + (c - a) * k / n, b + (d - b) * k / n, th);
  }
}

void bmp(Grid& g, const char* const* rows, int nrows, dreal x0, dreal y0, uint8_t v) {
  for (int y = 0; y < nrows; y++) for (int x = 0; rows[y][x]; x++) if (rows[y][x] == '#') px(g, jr(x0) + x, jr(y0) + y, v);
}

void plus(Grid& g, dreal x, dreal y) { line(g, x - 2, y, x + 2, y); line(g, x, y - 2, x, y + 2); }

void zee(Grid& g, dreal fx, dreal fy, bool big) {
  dreal x = jr(fx), y = jr(fy);
  if (big) { line(g, x, y, x + 4, y); line(g, x + 4, y, x, y + 4); line(g, x, y + 4, x + 4, y + 4); }
  else { line(g, x, y, x + 3, y); line(g, x + 3, y, x, y + 3); line(g, x, y + 3, x + 3, y + 3); }
}

// ---------- text ----------
static const Glyph* glyphIn(const Glyph* t, int n, char ch) {
  for (int i = 0; i < n; i++) if (t[i].ch == ch) return &t[i];
  return nullptr;
}
static const Glyph* glyphOf(Font f, char ch) {
  if (f == Font::Big) return glyphIn(FONT_BIG, FONT_BIG_N, ch);
  if (f == Font::Small) return glyphIn(FONT_SMALL, FONT_SMALL_N, ch);
  const Glyph* t = glyphIn(FONT_TXT, FONT_TXT_N, ch);
  return t ? t : glyphIn(FONT_SMALL, FONT_SMALL_N, ch);
}
static int gapOf(Font f, char a, char b) { return f == Font::Big ? ((a == ':' || b == ':') ? 3 : 2) : 1; }

static int textRun(Grid* g, const char* s, dreal x, dreal y, Font f, int sc, uint8_t v) {
  int cx = (int)x;  // callers pass whole numbers
  bool havePrev = false;
  char prev = 0;
  for (; *s; s++) {
    char ch = *s;
    if (ch == ' ') { cx += (f == Font::Big ? 5 : 4) * sc; havePrev = false; continue; }
    const Glyph* gl = glyphOf(f, ch);
    if (!gl) continue;
    if (havePrev) cx += gapOf(f, prev, ch) * sc;
    if (g) for (int r = 0; r < gl->h; r++) for (int c = 0; c < gl->w; c++) if (gl->rows[r * gl->w + c] == '#')
      for (int a = 0; a < sc; a++) for (int b = 0; b < sc; b++) px(*g, cx + c * sc + b, y + r * sc + a, v);
    cx += gl->w * sc;
    prev = ch; havePrev = true;
  }
  return cx - (int)x;
}
int text(Grid& g, const char* s, dreal x, dreal y, Font f, int sc, uint8_t v) { return textRun(&g, s, x, y, f, sc, v); }
int textW(const char* s, Font f, int sc) { return textRun(nullptr, s, 0, 0, f, sc, 1); }
void textC(Grid& g, const char* s, dreal cx, dreal y, Font f, int sc, uint8_t v) {
  text(g, s, jr(cx - (dreal)textW(s, f, sc) / 2), y, f, sc, v);
}

// ---------- eye ----------
void EyeP::apply(const EyeO& o) {
  if (has(o.w_)) w = o.w_;
  if (has(o.h_)) h = o.h_;
  if (has(o.r_)) r = o.r_;
  if (has(o.pr_)) pr = o.pr_;
  if (has(o.py_)) py = o.py_;
  if (has(o.px_)) px = o.px_;
  if (has(o.lt_)) lt = o.lt_;
  if (has(o.lb_)) lb = o.lb_;
  if (has(o.sl_)) sl = o.sl_;
  if (has(o.sm_)) sm = o.sm_;
  if (o.shape_ != Shape::Unset) shape = o.shape_;
  if (o.pup_ != Pup::Unset) pup = o.pup_;
  if (o.hasBrow_) { hasBrow = true; ba = o.ba_; bl = o.bl_; }
}


void eye(Grid& g, dreal cx, dreal cy, const EyeP& e, int side, dreal t) {
  dreal s = e.s != 0 ? e.s : 1;
  dreal w = e.w * s, h = e.h * s, hw = w / 2, hh = h / 2;
  dreal r = fmin(fmin(e.r * s, hw), hh), y0 = cy - hh;
  Shape shape = e.shape;
  dreal lt = e.lt, lb = e.lb, sl = e.sl;
  if (shape == Shape::Slab && lt + lb >= 0.9) shape = Shape::Unset;  // Unset = "shut" here
  int th = s >= 0.8 ? 3 : 2, th2 = s >= 0.8 ? 2 : 1;
  if (shape == Shape::Unset) {
    stroke(g, curve(cx, y0 + h * (lt / (lt + lb)) * 0.85 + h * 0.08, hw * 0.8, [=](dreal u) { return 1.4 * s * (1 - u * u); }), th2);
  } else if (shape == Shape::ArcDown) {
    stroke(g, curve(cx, cy + hh * 0.05, hw * 0.78, [=](dreal u) { return hh * 0.3 * (1 - u * u); }), th2);
  } else if (shape == Shape::ArcUp) {
    stroke(g, curve(cx, cy + hh * 0.3, hw * 0.8, [=](dreal u) { return -hh * 0.55 * (1 - u * u); }), th);
  } else if (shape == Shape::Chev) {
    dreal ox = cx + side * hw * 0.55, ix = cx - side * hw * 0.45;
    Pts p; p.add(ox, cy - hh * 0.45); p.add(ix, cy); p.add(ox, cy + hh * 0.45);
    stroke(g, p, th);
  } else if (shape == Shape::X) {
    Pts a; a.add(cx - hw * 0.5, cy - hh * 0.45); a.add(cx + hw * 0.5, cy + hh * 0.45); stroke(g, a, th);
    Pts b; b.add(cx + hw * 0.5, cy - hh * 0.45); b.add(cx - hw * 0.5, cy + hh * 0.45); stroke(g, b, th);
  } else {
    Pup pup = e.pup;
    dreal pr = (pup == Pup::Tiny ? 3 : (e.pr != 0 ? e.pr : 5)) * s;
    dreal PX = fmax(-1, fmin(1, e.px)), PY = fmax(-1, fmin(1, e.py));
    dreal pcx = cx + PX * fmax(0, hw - pr - 3 * s), pcy = cy + PY * fmax(0, hh - pr - 3 * s);
    auto RR = rr(cx, cy, hw, hh, r);
    dreal sr_ = w * 0.7 * e.sm, scy_ = y0 + h * 1.05, gx_ = pcx - pr * 0.45, gy_ = pcy - pr * 0.45;
    dreal rmax_ = fmin(hw, hh) - 2.5 * s;
    // Parameters above are computed in double (as the design does); the per-pixel test below runs in `real` (float on the chip).
    const real rcx = cx, rhw = hw, rside = side, rpr = pr, rpcx = pcx, rpcy = pcy, rsr = sr_, rscy = scy_,
               rrmax = rmax_, rt = t, ltop = y0 + lt * h, lbot = y0 + h - lb * h, slhh = sl * hh,
               rslit_x = 1.6 * s + 0.4, rslit_y = hh * 0.62, rcy = cy, rstar = pr * 1.5, rspir = 4.5 * s;
    const Pup pp = pup;
    area(g, [&](real X, real Y) {
      if (!RR(X, Y)) return false;
      real inner = (X - rcx) / rhw * (-rside);
      if (Y < ltop + slhh * inner) return false;
      if (Y > lbot) return false;
      if (rsr > 0 && (X - rcx) * (X - rcx) + (Y - rscy) * (Y - rscy) < rsr * rsr) return false;
      real dx = X - rpcx, dy = Y - rpcy;
      if (pp == Pup::Round || pp == Pup::Tiny) {
        // Near the pupil edge float can flip a pixel that sits exactly on the circle: decide those in double.
        if (fabs(dx) <= rpr + 1 && fabs(dy) <= rpr + 1) {
          double ddx = (double)X - pcx, ddy = (double)Y - pcy;
          if (ddx * ddx + ddy * ddy < pr * pr) return pp == Pup::Round && pr >= 3.5 && fabs((double)X - gx_) < 1.05 && fabs((double)Y - gy_) < 1.05;
        }
      } else if (pp == Pup::Slit) {
        if ((dx / rslit_x) * (dx / rslit_x) + (dy / rslit_y) * (dy / rslit_y) <= 1) return false;
      } else if (pp == Pup::Heart) {
        int hx = (int)floor(dx + (real)3.5), hy = (int)floor(dy + 3);
        if (hx >= 0 && hx < 7 && hy >= 0 && hy < 6 && HEART7[hy][hx] == '#') return false;
      } else if (pp == Pup::Spiral) {
        real qx = X - rcx, qy = Y - rcy, d = hypot(qx, qy);
        if (d < rrmax) {
          real v = fmod(fmod((atan2(qy, qx) / ((real)2 * (real)M_PI)) + d / rspir + rt, (real)1) + 1, (real)1);
          if (v < (real)0.34) return false;
        }
      } else if (pp == Pup::Star) {
        real a = fabs(dx) / rstar, b = fabs(dy) / rstar;
        if (sqrt(a) + sqrt(b) <= 1) return false;
      }
      return true;
    }, B{cx - hw - 1, cy - hh - 1, cx + hw + 1, cy + hh + 1});
  }
  if (e.hasBrow) {
    dreal by = y0 - 5 * s - e.bl * s, ix = cx - side * hw * 0.85, ox = cx + side * hw * 0.85, a = e.ba * s;
    Pts p; p.add(ox, by - a * 0.4); p.add((ix + ox) / 2, by - s + a * 0.3); p.add(ix, by + a);
    stroke(g, p, s >= 0.8 ? 3 : 2);
  }
}

// ---------- mouth ----------
void mouth(Grid& g, Mouth type, dreal mx, dreal my, dreal s, dreal mwf) {
  dreal hw = 12 * s * (mwf != 0 ? mwf : 1);
  int th = s >= 0.9 ? 2 : 1;
  auto fang = [&](dreal fx, dreal fy) {
    dreal x = jr(fx), y = jr(fy);
    if (s >= 0.9) { for (int k = -1; k <= 1; k++) { px(g, x + k, y); px(g, x + k, y + 1); } px(g, x, y + 2); }
    else { px(g, x, y); px(g, x + 1, y); px(g, x, y + 1); }
  };
  auto C = [&](auto fn, dreal w = 1) { stroke(g, curve(mx, my, hw * w, fn), th); };
  auto fangOn = [&](auto fn, dreal w, dreal u) { fang(mx + u * hw * w, my + fn(u) + th); };
  switch (type) {
    case Mouth::Line: { auto fn = [=](dreal u) { return 2 * s * (1 - u * u); }; C(fn); fangOn(fn, 1, 0.35); break; }
    case Mouth::Smile: { auto fn = [=](dreal u) { return 3.5 * s * (1 - u * u) - 0.5; }; C(fn); fangOn(fn, 1, 0.35); break; }
    case Mouth::Flat: { auto fn = [](dreal) { return 1; }; C(fn, 0.55); fangOn(fn, 0.55, 0.4); break; }
    case Mouth::Frown: C([=](dreal u) { return 2 * s - 2.8 * s * (1 - u * u); }, 0.7); break;
    case Mouth::Grin: {
      dreal w = hw * 0.85, top = my - 1, dep = 7.5 * s;
      ring(g, [=](dreal X, dreal Y) { dreal u = (X - mx) / w; return fabs(u) <= 1 && Y >= top && Y <= top + dep * (1 - u * u) + 0.5; },
           B{mx - w - 1, top - 1, mx + w + 1, top + dep + 2});
      fang(mx + w * 0.38, top + 1);
      break;
    }
    case Mouth::Rawr: {
      dreal rx = hw * 0.62, ry = 6.5 * s, cy = my + 1;
      ring(g, ell(mx, cy, rx, ry), bE(mx, cy, rx, ry));
      fang(mx - rx * 0.45, cy - ry + 1.5); fang(mx + rx * 0.45, cy - ry + 1.5);
      break;
    }
    case Mouth::ChompOpen: {
      dreal rx = hw * 0.6, ry = 5 * s, cy = my + 1;
      ring(g, ell(mx, cy, rx, ry), bE(mx, cy, rx, ry));
      fang(mx - rx * 0.4, cy - ry + 1.5); fang(mx + rx * 0.4, cy - ry + 1.5);
      dreal by = jr(cy + ry - 1.5);
      const dreal us[2] = {-0.15, 0.45};
      for (dreal u : us) { dreal x = jr(mx + u * rx); px(g, x, by); px(g, x + 1, by); px(g, x, by - 1); }
      break;
    }
    case Mouth::O: { dreal a = fmax(1.6, 3 * s), b = fmax(1.8, 3.6 * s); ring(g, ell(mx, my + 1, a, b), bE(mx, my + 1, a, b)); break; }
    case Mouth::OSmall: { dreal a = fmax(1.3, 2.2 * s); ring(g, ell(mx, my + 1, a, a), bE(mx, my + 1, a, a)); break; }
    case Mouth::Yawn: { dreal a = fmax(2, 4.6 * s), b = fmax(2.6, 6.6 * s); ring(g, ell(mx, my + 2, a, b), bE(mx, my + 2, a, b)); break; }
    case Mouth::Grr: {
      Pts p;
      int n = jr(hw * 1.6 / (2 * s + 0.5)); if (n < 4) n = 4;
      for (int i = 0; i <= n; i++) p.add(mx - hw * 0.8 + i * (hw * 1.6 / n), my + (i % 2 ? 2 * s : 0));
      stroke(g, p, th);
      break;
    }
    case Mouth::Wobble: C([=](dreal u) { return 1.6 * s * sin(u * M_PI * 2) + 1; }, 0.65); break;
    case Mouth::Smirk: { auto fn = [=](dreal u) { return u > 0 ? 1 - 2.8 * s * u * u : 1; }; C(fn, 0.8); fangOn(fn, 0.8, -0.35); break; }
    case Mouth::Tongue: {
      auto fn = [=](dreal u) { return 2.5 * s * (1 - u * u); };
      C(fn, 0.9);
      dreal tx = mx + hw * 0.25, ty = my + fn(0.25 / 0.9) + th;
      auto te = ell(tx, ty, 3.2 * s + 0.4, 4.4 * s + 0.4);
      area(g, [=](dreal X, dreal Y) { return Y >= ty && te(X, Y); }, B{tx - 5, ty, tx + 5, ty + 6});
      px(g, tx, ty + 1, 0); px(g, tx, ty + 2, 0);
      break;
    }
    case Mouth::Chomp: {
      Pts p;
      for (int i = 0; i <= 8; i++) p.add(mx - hw * 0.75 + i * (hw * 1.5 / 8), my + (i % 2 ? 3 * s : 0));
      stroke(g, p, th);
      break;
    }
    case Mouth::Drool: {
      C([](dreal) { return 1; }, 0.55);
      dreal x = mx + hw * 0.4;
      for (int k = 2; k < 5; k++) px(g, x, my + k * s + 1);
      dot(g, x, my + 6 * s + 1, 1);
      break;
    }
    default: break;
  }
}

// ---------- effects and the whole face ----------
struct EInfo { dreal ex, ey, hw, hh; };

static void fx(Grid& g, const FaceP& f, dreal cx, dreal cy, dreal s, dreal ax, const EInfo* E) {
  auto top = [](dreal y) { return fmax(1, y); };
  if (f.z != 0) { zee(g, ax + 1, cy - 6 * s, false); if (f.z > 1) zee(g, ax + 6, cy - 15 * s, true); }
  if (f.bang != 0) for (int k = 0; k < (int)f.bang; k++) text(g, "!", jr(ax + 2 + k * 4), top(cy - 24 * s), Font::Small);
  if (f.q != 0) text(g, "?", jr(ax + 1), top(cy - 24 * s), Font::Small);
  if (f.heart != 0) text(g, "*", jr(ax - 1), top(cy - 24 * s), Font::Small);
  if (f.note != 0) bmp(g, NOTE, 6, ax + 1, top(cy - 22 * s));
  if (f.sweat != 0) bmp(g, DROP, 5, ax, cy - 12 * s);
  if (f.tear != 0) bmp(g, DROP, 5, E[0].ex - 2, E[0].ey + E[0].hh + 3 * s);
  if (f.steam != 0) for (int i = 0; i < 2; i++) {
    const EInfo& q = E[i];
    dreal sd = i ? 1 : -1, x = q.ex + sd * (q.hw + 4 * s), y = cy - 17 * s;
    edge(g, ell(x, y, 2.4 * s + 0.6, 2 * s + 0.6), bE(x, y, 4, 4));
    edge(g, ell(x + sd * 3 * s, y - 4 * s, 1.6 * s + 0.4, 1.4 * s + 0.4), bE(x, y - 4 * s, 5, 5));
  }
  if (f.sparkle != 0) {
    plus(g, ax + 3, top(cy - 18 * s)); plus(g, E[0].ex - E[0].hw - 5 * s, cy - 14 * s); plus(g, ax + 1, cy + 12 * s);
  }
  if (f.crown != 0) bmp(g, CROWN, 4, cx - 3, top(cy - fmax(E[0].hh, E[1].hh) - 12 * s));
  if (f.dots != 0) for (int k = 0; k < (int)f.dots; k++) {
    px(g, ax + k * 4, cy + 10 * s); px(g, ax + k * 4 + 1, cy + 10 * s);
    px(g, ax + k * 4, cy + 10 * s + 1); px(g, ax + k * 4 + 1, cy + 10 * s + 1);
  }
}

void faceDraw(Grid& g, const FaceP& f) {
  dreal s = f.s, cx = f.x + f.dx, cy = f.y + f.dy, sp = f.sp * s, t = f.t;
  dreal lkx = f.lookx, lky = f.looky, gx = lkx * 3 * s, gy = lky * 2 * s;
  EInfo E[2];
  for (int i = 0; i < 2; i++) {
    int side = i ? 1 : -1;
    EyeP e = side < 0 ? f.L : f.R;
    e.s = s; e.px = e.px + lkx * 0.9; e.py = e.py + lky * 0.9;
    dreal ex = cx + side * sp + gx, ey = cy + gy;
    eye(g, ex, ey, e, side, t);
    E[i] = {ex, ey, e.w * s / 2, e.h * s / 2};
    if (f.cheeks != 0) {
      dreal y = ey + e.h * s / 2 + 3 * s;
      for (int k = -1; k <= 1; k++) { dreal x = ex + k * 5 * s; line(g, x - 1, y + 1, x + 1, y - 1); }
    }
  }
  if (f.glasses != 0) {
    for (int i = 0; i < 2; i++) {
      const EInfo& q = E[i];
      dreal hw = q.hw + 3 * s, hh = q.hh + 2.5 * s, sd = i ? 1 : -1;
      edge(g, rr(q.ex, q.ey, hw, hh, 5 * s), B{q.ex - hw - 1, q.ey - hh - 1, q.ex + hw + 1, q.ey + hh + 1});
      line(g, q.ex + sd * hw, q.ey - hh * 0.3, q.ex + sd * (hw + 5 * s), q.ey - hh * 0.45);
    }
    dreal y = E[0].ey - E[0].hh * 0.35;
    line(g, E[0].ex + E[0].hw + 3 * s, y, E[1].ex - E[1].hw - 3 * s, y);
  }
  if (f.nose != 0 && s >= 0.75) {
    dreal ny = jr(cy + 19 * s + gy * 0.6), nx = cx + gx * 0.6;
    px(g, nx - 5, ny); px(g, nx - 4, ny); px(g, nx + 4, ny); px(g, nx + 5, ny);
  }
  if (f.m != Mouth::None) mouth(g, f.m, cx + gx * 0.6 + f.mx * s, cy + f.my * s + gy * 0.5, s, f.mw);
  fx(g, f, cx, cy, s, E[1].ex + E[1].hw + 4 * s, E);
}

// ---------- expression presets (the design's EX table) ----------
static FaceO preset(Ex n) {
  FaceO f;
  switch (n) {
    case Ex::Idle: return f.m(Mouth::Line);
    case Ex::Happy: return f.E(EyeO().shape(Shape::ArcUp)).m(Mouth::Grin).cheeks();
    case Ex::Love: return f.E(EyeO().pup(Pup::Heart)).m(Mouth::Smile).cheeks().heart();
    case Ex::Curious: return f.L(EyeO().w(30).h(32).r(11).brow(-1, 3)).R(EyeO().lt(0.32).sl(-0.25).h(26)).m(Mouth::O).look(1, -0.3).mx(3);
    case Ex::Sleepy: return f.E(EyeO().lt(0.55).sl(-0.25).lb(0.04).py(0.5)).m(Mouth::Flat).y(28);
    case Ex::Asleep: return f.E(EyeO().shape(Shape::ArcDown)).m(Mouth::OSmall).y(29).z(1);
    case Ex::Surprised: return f.L(EyeO().w(32).h(34).r(12).pup(Pup::Tiny).brow(-1, 3)).R(EyeO().w(30).h(32).r(11).pup(Pup::Tiny).brow(-1, 3)).m(Mouth::O);
    case Ex::Grumpy: return f.E(EyeO().lt(0.38).sl(0.55).lb(0.1).py(0.25).brow(4)).m(Mouth::Grr).steam();
    case Ex::Rawr: return f.E(EyeO().pup(Pup::Slit).lt(0.2).sl(0.5).brow(4)).m(Mouth::Rawr);
    case Ex::Dizzy: return f.E(EyeO().pup(Pup::Spiral)).m(Mouth::Wobble);
    case Ex::Shy: return f.L(EyeO().w(22).h(24).r(8).lt(0.1).lb(0.15)).R(EyeO().w(21).h(23).r(8).lt(0.1).lb(0.15)).look(-1, 0.7).m(Mouth::Wobble).mw(0.7).cheeks();
    case Ex::Unimpressed: return f.E(EyeO().lt(0.5).lb(0.15).px(0.85).py(0.1)).m(Mouth::Flat);
    case Ex::Mischief: return f.L(EyeO().lt(0.34).sl(0.4).brow(3)).R(EyeO().shape(Shape::ArcUp)).m(Mouth::Tongue);
    case Ex::Sad: return f.E(EyeO().lt(0.18).sl(-0.55).py(0.5).brow(-3, 1)).m(Mouth::Frown).tear();
    case Ex::Focus: return f.E(EyeO().lt(0.32).lb(0.05).py(0.6)).glasses().m(Mouth::Flat);
    case Ex::Excited: return f.E(EyeO().pup(Pup::Star).pr(6)).m(Mouth::Grin).sparkle();
    case Ex::Hungry: return f.E(EyeO().lt(0.4).sl(-0.2).px(0.7).py(0.3)).m(Mouth::Drool);
    case Ex::Chew: return f.E(EyeO().shape(Shape::ArcUp)).m(Mouth::Chomp).cheeks();
    case Ex::Smug: return f.E(EyeO().lt(0.45).lb(0.12).px(0.6)).m(Mouth::Smirk);
    case Ex::Scared: return f.E(EyeO().pup(Pup::Tiny).brow(-3, 2)).m(Mouth::Wobble).sweat();
    case Ex::Determined: return f.E(EyeO().pup(Pup::Slit).lt(0.15).sl(0.3).brow(2)).m(Mouth::Flat);
    case Ex::Squeeze: return f.E(EyeO().shape(Shape::Chev)).m(Mouth::Grin).cheeks();
    case Ex::Wink: return f.R(EyeO().shape(Shape::ArcUp)).m(Mouth::Smile);
    case Ex::Yawn: return f.E(EyeO().shape(Shape::Chev)).m(Mouth::Yawn);
    default: return f.m(Mouth::Line);
  }
}

FaceO FaceO::over(const FaceO& o) const {
  FaceO r = *this;
  auto m = [](dreal& a, dreal b) { if (has(b)) a = b; };
  m(r.s_, o.s_); m(r.x_, o.x_); m(r.y_, o.y_); m(r.dx_, o.dx_); m(r.dy_, o.dy_); m(r.sp_, o.sp_); m(r.t_, o.t_);
  if (has(o.lookx_)) { r.lookx_ = o.lookx_; r.looky_ = o.looky_; }
  m(r.cheeks_, o.cheeks_); m(r.glasses_, o.glasses_); m(r.nose_, o.nose_); m(r.mx_, o.mx_); m(r.my_, o.my_); m(r.mw_, o.mw_);
  m(r.z_, o.z_); m(r.bang_, o.bang_); m(r.q_, o.q_); m(r.heart_, o.heart_); m(r.note_, o.note_); m(r.sweat_, o.sweat_);
  m(r.tear_, o.tear_); m(r.steam_, o.steam_); m(r.sparkle_, o.sparkle_); m(r.crown_, o.crown_); m(r.dots_, o.dots_);
  if (o.m_ != Mouth::Unset) r.m_ = o.m_;
  auto mergeEye = [](EyeO& a, const EyeO& b) {
    auto mm = [](dreal& p, dreal q) { if (has(q)) p = q; };
    mm(a.w_, b.w_); mm(a.h_, b.h_); mm(a.r_, b.r_); mm(a.pr_, b.pr_); mm(a.py_, b.py_); mm(a.px_, b.px_);
    mm(a.lt_, b.lt_); mm(a.lb_, b.lb_); mm(a.sl_, b.sl_); mm(a.sm_, b.sm_);
    if (b.shape_ != Shape::Unset) a.shape_ = b.shape_;
    if (b.pup_ != Pup::Unset) a.pup_ = b.pup_;
    if (b.hasBrow_) { a.hasBrow_ = true; a.ba_ = b.ba_; a.bl_ = b.bl_; }
  };
  if (o.hasE_) { mergeEye(r.e_, o.e_); r.hasE_ = true; }
  if (o.hasL_) { mergeEye(r.l_, o.l_); r.hasL_ = true; }
  if (o.hasR_) { mergeEye(r.r_, o.r_); r.hasR_ = true; }
  return r;
}

static void applyFace(FaceP& f, const FaceO& q) {
  auto m = [](dreal& a, dreal b) { if (has(b)) a = b; };
  if (q.hasE_) { f.L.apply(q.e_); f.R.apply(q.e_); }
  if (q.hasL_) f.L.apply(q.l_);
  if (q.hasR_) f.R.apply(q.r_);
  m(f.s, q.s_); m(f.x, q.x_); m(f.y, q.y_); m(f.dx, q.dx_); m(f.dy, q.dy_); m(f.sp, q.sp_); m(f.t, q.t_);
  if (has(q.lookx_)) { f.lookx = q.lookx_; f.looky = q.looky_; }
  m(f.cheeks, q.cheeks_); m(f.glasses, q.glasses_); m(f.nose, q.nose_); m(f.mx, q.mx_); m(f.my, q.my_); m(f.mw, q.mw_);
  m(f.z, q.z_); m(f.bang, q.bang_); m(f.q, q.q_); m(f.heart, q.heart_); m(f.note, q.note_); m(f.sweat, q.sweat_);
  m(f.tear, q.tear_); m(f.steam, q.steam_); m(f.sparkle, q.sparkle_); m(f.crown, q.crown_); m(f.dots, q.dots_);
  if (q.m_ != Mouth::Unset) f.m = q.m_;
}

FaceP resolve(Ex n, const FaceO& over) {
  FaceP f;
  f.L = EyeP(); f.R = EyeP();
  f.R.w = 26; f.R.h = 28; f.R.r = 9;  // the design's BR (right eye is slightly smaller than BL)
  applyFace(f, preset(n));
  applyFace(f, over);
  return f;
}

// ---------- props and pieces ----------
void stampIcon(Grid& g, Icon ic, dreal x0, dreal y0, uint8_t v) {
  const uint8_t* b = ICONS16[(int)ic];
  for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if (b[y * 2 + x / 8] & (0x80 >> (x % 8))) px(g, x0 + x, y0 + y, v);
}

void card(Grid& g, int x0, int y0, int w, int h) {
  int x1 = x0 + w - 1, y1 = y0 + h - 1;
  for (int y = y0; y <= y1 + 1; y++) for (int x = x0; x <= x1; x++) {
    if (y == y1 + 1) { if (x > x0 && x < x1) px(g, x, y); continue; }
    if ((x == x0 || x == x1) && (y == y0 || y == y1)) continue;
    px(g, x, y, (x == x0 || x == x1 || y == y0 || y == y1) ? 1 : 0);
  }
}

void alarmProp(Grid& g, int r, int x, int y) {
  stampIcon(g, Icon::Alarm, x + (r == 1 ? -1 : r == 2 ? 1 : 0), y);
  if (r == 1) { line(g, x - 4, y + 2, x - 6, y); line(g, x - 5, y + 7, x - 8, y + 7); }
  if (r == 2) { line(g, x + 18, y + 1, x + 20, y - 1); line(g, x + 18, y + 6, x + 21, y + 6); }
}

void batt(Grid& g, int x, int y, int n) {
  edge(g, rr(x + 7, y + 4.5, 7, 4.5, 0), B{(dreal)x - 1, (dreal)y - 1, (dreal)x + 15, (dreal)y + 10});
  for (int k = 3; k <= 5; k++) { px(g, x + 14, y + k); px(g, x + 15, y + k); }
  for (int k = 0; k < n; k++) fillRect(g, x + 2 + k * 4, y + 2, x + 4 + k * 4, y + 6);
}

void speed(Grid& g, int x, int y0, int y1) { for (int y = y0; y <= y1; y += 6) line(g, x, y, x + 3, y); }

void shiftG(const Grid& in, Grid& out, int dx, int dy) {
  out.clear();
  for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (in.d[y * W + x]) {
    int nx = x + dx, ny = y + dy;
    if (nx >= 0 && nx < W && ny >= 0 && ny < H) out.d[ny * W + nx] = 1;
  }
}

}  // namespace face
