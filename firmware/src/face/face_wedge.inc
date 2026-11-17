// Wedge face (new-eyes sheet, page D): flat-cut wedge eyes and a small 1 px mouth. No pupils, no brows, no nose.
// Included by face.cpp inside namespace face when DINO_WEDGE is defined (firmware dino_wedge_c3); the default
// firmware never sees this file. Same API as the original eye()/mouth()/preset(), so scenes, life and the brain
// are unchanged. Port of art/reference/dino-wedge-v1/wedge_face.js: keep the two in step (tools/face_check_wedge.py).

// Scene overrides were written for the old 28 x 30 slab: scale their w/h to the 18 x 14 wedge.
static const dreal WEDGE_LEGACY_W = 18.0 / 28.0, WEDGE_LEGACY_H = 14.0 / 30.0;

// Top-edge drop as a fraction of h, at the outer and inner edge. lt = lid height, sl = slant (+ slants in, - slants out).
static void lidWedge(dreal lt, dreal sl, dreal& od, dreal& id) {
  od = lt - sl / 2; id = lt + sl / 2;
  dreal m = fmin(od, id);
  if (m < 0) { od -= m; id -= m; }
  od = fmin(od, 0.95); id = fmin(id, 0.95);
}

void eye(Grid& g, dreal cx, dreal cy, const EyeP& e, int side, dreal t) {
  const dreal s = e.s != 0 ? e.s : 1;
  dreal h0 = e.h, lt = e.lt, lb = e.lb, sl = e.sl;
  if (e.hasBrow) {  // no brows on this face: a raised brow makes the eye taller, a hard one slants the lid
    if (e.bl > 0) h0 += e.bl * 0.8;
    if (e.ba >= 2 && sl == 0 && lt == 0) { lt = 0.15; sl = 0.5; }
  }
  const dreal w = e.w * s, h = h0 * s, hw = w / 2, hh = h / 2, y0 = cy - hh;
  enum Kind { SLAB, SHUT, ARC_DOWN, ARC_UP, CHEV, CROSS, HEART, STAR, SPIRAL } kind = SLAB;
  switch (e.shape) {
    case Shape::ArcDown: kind = ARC_DOWN; break;
    case Shape::ArcUp: kind = ARC_UP; break;
    case Shape::Chev: kind = CHEV; break;
    case Shape::X: kind = CROSS; break;
    default:
      if (e.pup == Pup::Heart) kind = HEART;
      else if (e.pup == Pup::Star) kind = STAR;
      else if (e.pup == Pup::Spiral) kind = SPIRAL;
      else if (lt + lb >= 0.9) kind = SHUT;
  }
  const int th = s >= 0.8 ? 3 : 2, th2 = s >= 0.8 ? 2 : 1;
  switch (kind) {
    case SHUT: {
      dreal yy = jr(y0 + h * (lt / (lt + lb)) * 0.85 + h * 0.08);
      fillRect(g, cx - hw * 0.95, yy, cx + hw * 0.95, yy + th2 - 1);
      break;
    }
    case ARC_DOWN:
      stroke(g, curve(cx, cy - 1 * s, hw * 1.05, [=](dreal u) { return 4 * s * (1 - u * u); }), th2);
      break;
    case ARC_UP: {
      Pts p; p.add(cx - hw * 1.1, cy + 4 * s); p.add(cx, cy - 4 * s); p.add(cx + hw * 1.1, cy + 4 * s);
      stroke(g, p, th);
      break;
    }
    case CHEV: {
      dreal ox = cx + side * hw * 0.55, ix = cx - side * hw * 0.45;
      Pts p; p.add(ox, cy - hh * 0.45); p.add(ix, cy); p.add(ox, cy + hh * 0.45);
      stroke(g, p, th);
      break;
    }
    case CROSS: {
      Pts a; a.add(cx - hw * 0.5, cy - hh * 0.45); a.add(cx + hw * 0.5, cy + hh * 0.45); stroke(g, a, th);
      Pts b; b.add(cx + hw * 0.5, cy - hh * 0.45); b.add(cx - hw * 0.5, cy + hh * 0.45); stroke(g, b, th);
      break;
    }
    case HEART: {  // the 7 x 6 heart bitmap at 3 px per cell
      const dreal k = 3 * s, x0 = cx - 3.5 * k, yy = cy - 3 * k;
      const real rk = k, rx0 = x0, ry0 = yy;
      area(g, [=](real X, real Y) {
        int c = (int)floor((X - rx0) / rk), r = (int)floor((Y - ry0) / rk);
        return c >= 0 && c < 7 && r >= 0 && r < 6 && HEART7[r][c] == '#';
      }, B{x0 - 1, yy - 1, x0 + 7 * k + 1, yy + 6 * k + 1});
      break;
    }
    case STAR: {  // four-point star
      const dreal a = 8.5 * s, b = 10 * s;
      const real rcx = cx, rcy = cy, ra = a, rb = b;
      area(g, [=](real X, real Y) { return pow(fabs(X - rcx) / ra, (real)0.6) + pow(fabs(Y - rcy) / rb, (real)0.6) <= 1; },
           B{cx - a - 1, cy - b - 1, cx + a + 1, cy + b + 1});
      break;
    }
    case SPIRAL: {  // 38 points: Pts holds at most 40
      const dreal R = 10 * s, turns = 2.1, ph = t * 2 * M_PI;
      Pts p;
      for (int k = 0; k <= 38; k++) {
        dreal q = k / 38.0 * turns * 2 * M_PI, r = R * q / (turns * 2 * M_PI);
        p.add(cx + r * cos(q + ph), cy + r * sin(q + ph));
      }
      stroke(g, p, th2);
      break;
    }
    default: {  // the wedge slab
      dreal od, id;
      lidWedge(lt, sl, od, id);
      const int ch = (od == 0 && id == 0) ? (int)fmax(1, jr(2 * s)) : 0;  // outer top corner is chamfered on a flat eye
      const real rcx = cx, rhw = hw, rside = side, ry0 = y0, rh = h, rod = od, rid = id, rbot = y0 + h - lb * h, rch = ch;
      area(g, [=](real X, real Y) {
        real u = (X - rcx) / rhw;
        if (fabs(u) > 1 || Y < ry0 || Y > ry0 + rh) return false;
        real f = rod + (rid - rod) * ((-rside * u) + 1) / 2;
        if (Y < ry0 + f * rh || Y > rbot) return false;
        if (ch) {
          real dxo = rside < 0 ? X - (rcx - rhw) : (rcx + rhw) - X;
          if (dxo + (Y - ry0) < rch + (real)0.5) return false;
        }
        return true;
      }, B{cx - hw - 1, cy - hh - 1, cx + hw + 1, cy + hh + 1});
    }
  }
}

// ---------- mouth: small, 1 px stroke ----------
void mouth(Grid& g, Mouth type, dreal mx, dreal my, dreal s, dreal mwf) {
  const dreal hw = 5 * s * (mwf != 0 ? mwf : 1);
  auto C = [&](auto fn, dreal w = 1) { stroke(g, curve(mx, my, hw * w, fn), 1); };
  auto box = [&](dreal rx, dreal ry) { edge(g, rr(mx, my + ry, rx, ry, 1.2 * s), B{mx - rx - 1, my - 1, mx + rx + 1, my + 2 * ry + 1}); };
  switch (type) {
    case Mouth::Line: C([=](dreal u) { return 1.6 * s * (1 - u * u); }); break;
    case Mouth::Smile: C([=](dreal u) { return 2.4 * s * (1 - u * u); }, 1.2); break;
    case Mouth::Flat: C([](dreal) { return 1; }, 0.8); break;
    case Mouth::Frown: C([=](dreal u) { return 1.8 * s - 2.2 * s * (1 - u * u); }, 0.9); break;
    case Mouth::Grin: {  // filled wedge, flat on top
      const dreal w = hw * 1.1, dep = 4.5 * s;
      area(g, [=](real X, real Y) { real u = fabs((X - mx) / w); return u <= 1 && Y >= my && Y <= my + dep * (1 - u) + 0.4; },
           B{mx - w - 1, my - 1, mx + w + 1, my + dep + 2});
      break;
    }
    case Mouth::Rawr: {
      const dreal rx = hw * 1.35, ry = 3 * s + 0.5;
      box(rx, ry);
      const dreal tx = jr(rx * 0.5);
      for (int sd = -1; sd <= 1; sd += 2) { px(g, mx + sd * tx, my + 1); if (s >= 0.9) px(g, mx + sd * tx, my + 2); }
      break;
    }
    case Mouth::ChompOpen: {
      const dreal rx = hw * 1.3, ry = 3 * s + 0.5;
      box(rx, ry);
      const dreal tx = jr(rx * 0.5), by = jr(my + 2 * ry - 1);
      for (int sd = -1; sd <= 1; sd += 2) { px(g, mx + sd * tx, my + 1); px(g, mx + sd * jr(tx * 0.3), by - 1); }
      break;
    }
    case Mouth::O: { dreal a = fmax(2.4, 2.8 * s), b = fmax(2.2, 2.5 * s); ring(g, ell(mx, my + 2 * s, a, b), bE(mx, my + 2 * s, a, b)); break; }
    case Mouth::OSmall: { dreal a = fmax(2.2, 2.4 * s); ring(g, ell(mx, my + 1, a, a), bE(mx, my + 1, a, a)); break; }
    case Mouth::Yawn: { dreal a = fmax(2.6, 3.8 * s), b = fmax(3.2, 5.4 * s); ring(g, ell(mx, my + 3 * s, a, b), bE(mx, my + 3 * s, a, b)); break; }
    case Mouth::Grr: {
      Pts p;
      for (int i = 0; i <= 6; i++) p.add(mx - hw * 0.9 + i * (hw * 1.8 / 6), my + (i % 2 ? 1.6 * s : 0));
      stroke(g, p, 1);
      break;
    }
    case Mouth::Wobble: C([=](dreal u) { return 1.1 * s * sin(u * M_PI * 3) + 1; }, 1.25); break;
    case Mouth::Smirk: C([=](dreal u) { return u > 0 ? 1 - 2 * s * u * u : 1; }, 0.9); break;
    case Mouth::Tongue: {
      auto fn = [=](dreal u) { return 2 * s * (1 - u * u); };
      C(fn, 1.05);
      const dreal tx = jr(mx + hw * 0.3), ty = jr(my + fn(0.3) + 1);
      fillRect(g, tx - 1, ty, tx + 1, ty + fmax(1, jr(2 * s)));
      break;
    }
    case Mouth::Chomp: {
      Pts p;
      for (int i = 0; i <= 8; i++) p.add(mx - hw * 0.95 + i * (hw * 1.9 / 8), my + (i % 2 ? 2 * s : 0));
      stroke(g, p, 1);
      break;
    }
    case Mouth::Drool: {
      C([](dreal) { return 1; }, 0.7);
      const dreal x = mx + hw * 0.5;
      for (int k = 2; k < 5; k++) px(g, x, my + k * s + 1);
      dot(g, x, my + 6 * s + 1, 1);
      break;
    }
    default: break;
  }
}

// ---------- the 24 expression presets, in wedge units (page D of the new-eyes sheet + 8 of our own) ----------
static FaceO presetWedge(Ex n) {
  FaceO f;
  switch (n) {
    case Ex::Idle: return f.m(Mouth::Line);
    case Ex::Happy: return f.E(EyeO().shape(Shape::ArcUp)).m(Mouth::Grin);
    case Ex::Love: return f.E(EyeO().pup(Pup::Heart)).m(Mouth::Wobble).mw(0.7).heart();
    case Ex::Curious: return f.L(EyeO().w(12).h(19)).R(EyeO().w(18).h(7)).m(Mouth::OSmall).my(16);
    case Ex::Sleepy: return f.E(EyeO().h(4)).m(Mouth::Flat).mw(0.6).y(28).my(13);
    case Ex::Asleep: return f.E(EyeO().shape(Shape::ArcDown)).m(Mouth::OSmall).y(29).my(14).z(1);
    case Ex::Surprised: return f.E(EyeO().w(12).h(19)).m(Mouth::OSmall).my(17);
    case Ex::Grumpy: return f.E(EyeO().h(12).lt(0.35).sl(0.8)).m(Mouth::Flat).mw(0.8).my(13).steam();
    case Ex::Rawr: return f.E(EyeO().w(20).h(17).lt(0.25).sl(0.9)).m(Mouth::Rawr).my(17);
    case Ex::Dizzy: return f.E(EyeO().pup(Pup::Spiral)).m(Mouth::Wobble).mw(1.1).my(17);
    case Ex::Shy: return f.E(EyeO().w(12).h(9)).look(-1, 0.7).m(Mouth::Wobble).mw(0.9).my(11);
    case Ex::Unimpressed: return f.E(EyeO().h(12).lt(0.45)).m(Mouth::Flat);
    case Ex::Mischief: return f.E(EyeO().lt(0.3).sl(0.6)).m(Mouth::Line);
    case Ex::Sad: return f.E(EyeO().lt(0.1).sl(-0.6)).m(Mouth::Frown).my(15);
    case Ex::Focus: return f.E(EyeO().h(10).lt(0.3)).glasses().m(Mouth::Flat);
    case Ex::Excited: return f.E(EyeO().pup(Pup::Star)).m(Mouth::Grin).my(17);
    case Ex::Hungry: return f.E(EyeO().lt(0.35).sl(-0.2)).m(Mouth::Drool);
    case Ex::Chew: return f.E(EyeO().shape(Shape::ArcUp)).m(Mouth::Chomp);
    case Ex::Smug: return f.E(EyeO().h(8).lt(0.05)).m(Mouth::Flat).mw(0.9);
    case Ex::Scared: return f.E(EyeO().w(12).h(19)).m(Mouth::Wobble).mw(1).my(17).sweat();
    case Ex::Determined: return f.E(EyeO().lt(0.25).sl(0.5)).m(Mouth::Flat);
    case Ex::Squeeze: return f.E(EyeO().shape(Shape::Chev)).m(Mouth::Grin);
    case Ex::Wink: return f.R(EyeO().shape(Shape::ArcUp)).m(Mouth::Grin).mw(0.7);
    case Ex::Yawn: return f.E(EyeO().shape(Shape::Chev)).m(Mouth::Yawn).my(13);
    default: return f.m(Mouth::Line);
  }
}
