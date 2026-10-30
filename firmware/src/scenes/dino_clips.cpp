// Dino clips: see dino_clips.h. Ported from the design sheet's scene code (art/reference/dino-v1/rig.js,
// function build). Frame contents are kept in the same order as the design so the check can compare them 1:1.
#include "dino_clips.h"
#include "dino_parts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

using namespace face;

namespace scenes {

namespace {

// Draw-op builders (fa, I, sm, tx, tc, ic, fn, fr) and SHUT/HALF: see dino_parts.h.

std::string nextMin(const std::string& t) {
  int h = atoi(t.substr(0, 2).c_str()), m = atoi(t.substr(3).c_str()) + 1;
  if (m == 60) { m = 0; h = (h + 1) % 24; }
  char b[8]; snprintf(b, sizeof b, "%02d:%02d", h, m);
  return b;
}

// Eye cell for the rig sheet (design: cellE).
Draw cellE(const EyeO& o, real x, real y, int side = 1) {
  EyeP e; e.w = 28; e.h = 30; e.r = 10; e.pr = 5; e.py = 0.1; e.s = 0.5;
  e.apply(o);
  return [=](Grid& g) { eye(g, x, y, e, side, 0.1); };
}


// ---- shared by several clips ----
static std::string T, FN;  // set by buildDino
static Draw TB(const std::string& t, real y) { return tx(t, 62, y, Font::Big); }
static Clip mk(char grp, const char* key, const char* name) {
  Clip c; c.group = grp; c.key = key; c.name = name;
  return c;
}
static void put(std::vector<Clip>& out, char grp, const char* key, const char* name, std::vector<Frame> frames) {
  Clip c; c.group = grp; c.key = key; c.name = name; c.frames = std::move(frames);
  out.push_back(std::move(c));
}
static const auto ap = [](int r) { return fn([=](Grid& g) { alarmProp(g, r); }); };
static const auto bt = [](int x, int y, int n) { return fn([=](Grid& g) { batt(g, x, y, n); }); };

// One function per clip: building every clip in one function needed ~59 KB of stack (the chip's loop task has 8 KB),
// building just one needs only that clip's temporaries.
#define CLIP(key) __attribute__((noinline)) static void c_##key(std::vector<Clip>& out)
CLIP(rig_eyes) {
  out.clear();
  Frame f;
  f.note = "pupils · shapes";
  const EyeO top[6] = {EyeO(), EyeO().pup(Pup::Tiny), EyeO().pup(Pup::Slit), EyeO().pup(Pup::Heart), EyeO().pup(Pup::Spiral), EyeO().pup(Pup::Star).pr(6)};
  const EyeO bot[6] = {EyeO().shape(Shape::ArcUp), EyeO().shape(Shape::ArcDown), EyeO().shape(Shape::Chev), SHUT, EyeO().lt(0.45), EyeO().sm(1)};
  for (int i = 0; i < 6; i++) f.ops.push_back(cellE(top[i], 11 + i * 21, 15));
  for (int i = 0; i < 6; i++) f.ops.push_back(cellE(bot[i], 11 + i * 21, 47));
  put(out, 'R', "rig_eyes", "Eye parts", {f});
}

CLIP(rig_brows) {
  Frame f;
  f.note = "brows · lids";
  const EyeO top[4] = {EyeO().brow(-3, 2), EyeO().brow(0, 4), EyeO().brow(2), EyeO().brow(5).lt(0.3).sl(0.6)};
  const EyeO bot[4] = {EyeO().lt(0.3).sl(-0.45), EyeO().lt(0.5).lb(0.1), EyeO().lt(0.2).lb(0.35), EyeO().lt(0.4).sl(0.5).pup(Pup::Slit)};
  for (int i = 0; i < 4; i++) f.ops.push_back(cellE(top[i], 16 + i * 32, 22, -1));
  for (int i = 0; i < 4; i++) f.ops.push_back(cellE(bot[i], 16 + i * 32, 50, -1));
  put(out, 'R', "rig_brows", "Brows and lids", {f});
}

CLIP(rig_mouths) {
  static const Mouth MT[16] = {Mouth::Line, Mouth::Smile, Mouth::Flat, Mouth::Frown, Mouth::Grin, Mouth::Rawr, Mouth::O, Mouth::OSmall,
                                   Mouth::Yawn, Mouth::Grr, Mouth::Wobble, Mouth::Smirk, Mouth::Tongue, Mouth::Chomp, Mouth::ChompOpen, Mouth::Drool};
  Frame f;
  f.note = "16 mouths";
  for (int i = 0; i < 16; i++) { Mouth m = MT[i]; real x = 16 + (i % 4) * 32, y = 3 + (i / 4) * 16; f.ops.push_back([=](Grid& g) { mouth(g, m, x, y, 0.75, 0); }); }
  put(out, 'R', "rig_mouths", "Mouths", {f});
}

CLIP(rig_type) {
  {
    Clip c = mk('R', "rig_type", "Type and digits");
    c.frames.reserve(1);
    c.frames.push_back(fr(0, "full set", {tx("0123456789", 2, 1, Font::Big), tx("0123456789:+-!?*", 2, 17, Font::Small),
        tx("ABCDEFGHIJKLM", 2, 27), tx("NOPQRSTUVWXYZ", 2, 37), tx(FN + ": BOOP! *", 2, 50)}));
    out.push_back(std::move(c));
  }
}

CLIP(rig_icons) {
  Frame f;
  f.note = "menu row · props row";
  for (int i = 0; i < 16; i++) f.ops.push_back(ic((Icon)i, i % 8 * 16, i < 8 ? 10 : 38));
  put(out, 'R', "rig_icons", "Icons 16 x 16", {f});
}

CLIP(expressions) {
  std::vector<Frame> v;
  for (int i = 0; i < (int)Ex::Count; i++) v.push_back(fr(900, "", {fa((Ex)i)}));
  put(out, 'R', "expressions", "Expression sheet", v);
}

CLIP(scale) {
  {
    Clip c = mk('R', "scale", "Scale ladder");
    c.frames.reserve(1);
    c.frames.push_back(fr(0, "0.7 · 0.55 · 0.4 (not used)", {fa(Ex::Idle, FaceO().s(0.7).x(27).y(30)), fa(Ex::Idle, FaceO().s(0.55).x(75).y(30)), fa(Ex::Idle, FaceO().s(0.4).x(112).y(30))}));
    out.push_back(std::move(c));
  }
}

CLIP(idle) {
  {
    Clip c = mk('L', "idle", "Idle");
    c.frames.reserve(14);
    c.frames.push_back(fr(2400, "idle", {I()}));
    c.frames.push_back(fr(60, "blink", {I(FaceO().E(HALF))}));
    c.frames.push_back(fr(80, "", {I(FaceO().E(SHUT))}));
    c.frames.push_back(fr(60, "", {I(FaceO().E(HALF))}));
    c.frames.push_back(fr(1600, "", {I()}));
    c.frames.push_back(fr(900, "glance right", {I(FaceO().look(1, 0))}));
    c.frames.push_back(fr(700, "", {I(FaceO().look(1, -0.4))}));
    c.frames.push_back(fr(1100, "glance left", {I(FaceO().look(-1, 0.1))}));
    c.frames.push_back(fr(1200, "breathe in", {I(FaceO().y(26))}));
    c.frames.push_back(fr(1200, "breathe out", {I(FaceO().y(28))}));
    c.frames.push_back(fr(60, "double blink", {I(FaceO().E(SHUT))}));
    c.frames.push_back(fr(120, "", {I()}));
    c.frames.push_back(fr(60, "", {I(FaceO().E(SHUT))}));
    c.frames.push_back(fr(1400, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(pet) {
  {
    Clip c = mk('L', "pet", "Pet");
    c.frames.reserve(6);
    c.frames.push_back(fr(300, "touch", {I(FaceO().E(EyeO().brow(-1, 2)))}));
    c.frames.push_back(fr(700, "shy", {fa(Ex::Shy)}));
    c.frames.push_back(fr(1200, "heart eyes", {fa(Ex::Love)}));
    c.frames.push_back(fr(900, "grin", {fa(Ex::Squeeze)}));
    c.frames.push_back(fr(600, "happy", {fa(Ex::Happy)}));
    c.frames.push_back(fr(400, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(overpet) {
  {
    Clip c = mk('L', "overpet", "Over-petted");
    c.frames.reserve(6);
    c.frames.push_back(fr(600, "side-eye", {fa(Ex::Unimpressed)}));
    c.frames.push_back(fr(500, "grumble", {fa(Ex::Grumpy)}));
    c.frames.push_back(fr(400, "", {fa(Ex::Grumpy, FaceO().dy(1))}));
    c.frames.push_back(fr(500, "", {fa(Ex::Grumpy)}));
    c.frames.push_back(fr(800, "looks away", {fa(Ex::Unimpressed, FaceO().look(-1, 0).E(EyeO().px(-0.85)))}));
    c.frames.push_back(fr(400, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(rawr) {
  {
    Clip c = mk('L', "rawr", "RAWR");
    c.frames.reserve(9);
    c.frames.push_back(fr(500, "idle", {I()}));
    c.frames.push_back(fr(400, "anticipation", {fa(Ex::Determined, FaceO().dy(1))}));
    c.frames.push_back(fr(140, "RAWR", {fa(Ex::Rawr, FaceO().dx(2).bang())}));
    c.frames.push_back(fr(140, "", {fa(Ex::Rawr, FaceO().dx(-2).bang())}));
    c.frames.push_back(fr(500, "", {fa(Ex::Rawr, FaceO().bang())}));
    c.frames.push_back(fr(300, "giggle", {fa(Ex::Squeeze)}));
    c.frames.push_back(fr(200, "", {fa(Ex::Happy, FaceO().dy(-1))}));
    c.frames.push_back(fr(200, "", {fa(Ex::Squeeze)}));
    c.frames.push_back(fr(400, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(shake) {
  std::vector<Frame> v;
  v.push_back(fr(120, "jolt", {fa(Ex::Surprised, FaceO().dx(3).bang())}));
  v.push_back(fr(120, "", {fa(Ex::Surprised, FaceO().dx(-3).bang())}));
  const real ts[8] = {0, 0.25, 0.5, 0.75, 0, 0.25, 0.5, 0.75};
  for (int i = 0; i < 8; i++) v.push_back(fr(150, i ? "" : "dizzy", {fa(Ex::Dizzy, FaceO().t(ts[i]).dx(i % 2 ? 1 : -1))}));
  v.push_back(fr(500, "recovering", {fa(Ex::Sleepy)}));
  v.push_back(fr(80, "", {I(FaceO().E(SHUT))}));
  v.push_back(fr(400, "", {I()}));
  put(out, 'L', "shake", "Shaken", v);
}

CLIP(pickup) {
  {
    Clip c = mk('L', "pickup", "Picked up");
    c.frames.reserve(6);
    c.frames.push_back(fr(300, "lifted", {fa(Ex::Surprised, FaceO().dy(-2).bang())}));
    c.frames.push_back(fr(900, "looks down", {fa(Ex::Curious, FaceO().look(0, 1))}));
    c.frames.push_back(fr(700, "", {fa(Ex::Curious, FaceO().look(-1, 0.5))}));
    c.frames.push_back(fr(700, "", {fa(Ex::Curious, FaceO().look(1, 0.5))}));
    c.frames.push_back(fr(600, "", {fa(Ex::Happy)}));
    c.frames.push_back(fr(400, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(tilt) {
  {
    Clip c = mk('L', "tilt", "Tilted");
    c.frames.reserve(5);
    c.frames.push_back(fr(700, "tilt left", {fa(Ex::Scared, FaceO().dx(-6).look(-1, 0.4))}));
    c.frames.push_back(fr(300, "", {fa(Ex::Scared, FaceO().dx(-3).look(-0.5, 0.2))}));
    c.frames.push_back(fr(700, "tilt right", {fa(Ex::Scared, FaceO().dx(6).look(1, 0.4))}));
    c.frames.push_back(fr(300, "level", {I()}));
    c.frames.push_back(fr(700, "relief", {fa(Ex::Happy)}));
    out.push_back(std::move(c));
  }
}

CLIP(sleep) {
  {
    Clip c = mk('L', "sleep", "Fall asleep");
    c.frames.reserve(5);
    c.frames.push_back(fr(1500, "drowsy", {fa(Ex::Sleepy)}));
    c.frames.push_back(fr(1500, "", {fa(Ex::Sleepy, FaceO().E(EyeO().lt(0.7)))}));
    c.frames.push_back(fr(400, "", {fa(Ex::Sleepy, FaceO().E(SHUT))}));
    c.frames.push_back(fr(1600, "asleep", {fa(Ex::Asleep)}));
    c.frames.push_back(fr(1600, "", {fa(Ex::Asleep, FaceO().y(30).z(2))}));
    out.push_back(std::move(c));
  }
}

CLIP(wake) {
  {
    Clip c = mk('L', "wake", "Wake up");
    c.frames.reserve(8);
    c.frames.push_back(fr(800, "asleep", {fa(Ex::Asleep, FaceO().z(2))}));
    c.frames.push_back(fr(700, "one eye", {fa(Ex::Idle, FaceO().L(EyeO().shape(Shape::ArcDown)).R(EyeO().lt(0.5)).m(Mouth::Flat))}));
    c.frames.push_back(fr(900, "yawn", {fa(Ex::Yawn)}));
    c.frames.push_back(fr(400, "", {fa(Ex::Sleepy)}));
    c.frames.push_back(fr(80, "", {I(FaceO().E(SHUT))}));
    c.frames.push_back(fr(120, "", {I()}));
    c.frames.push_back(fr(80, "", {I(FaceO().E(SHUT))}));
    c.frames.push_back(fr(500, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(time) {
  {
    Clip c = mk('U', "time", "Time on request");
    c.frames.reserve(11);
    c.frames.push_back(fr(500, "idle", {I()}));
    c.frames.push_back(fr(200, "double tap: brows pop", {I(FaceO().E(EyeO().brow(-1, 3)).look(0, -0.3))}));
    c.frames.push_back(fr(120, "face slides aside", {fa(Ex::Idle, FaceO().s(0.8).x(48).y(28).look(1, 0))}));
    c.frames.push_back(fr(140, "", {sm(Ex::Idle)}));
    c.frames.push_back(fr(120, "digits drop in", {sm(Ex::Idle, FaceO().look(1, -0.6)), TB(T, 12)}));
    c.frames.push_back(fr(100, "overshoot", {sm(Ex::Idle), TB(T, 27)}));
    c.frames.push_back(fr(1400, "hold", {sm(Ex::Idle), TB(T, 25)}));
    c.frames.push_back(fr(1000, "smug at you", {sm(Ex::Smug, FaceO().look(0, 0).E(EyeO().px(0))), TB(T, 25)}));
    c.frames.push_back(fr(120, "digits fall", {sm(Ex::Idle, FaceO().look(1, 0.6)), TB(T, 44)}));
    c.frames.push_back(fr(140, "face grows back", {fa(Ex::Idle, FaceO().s(0.8).x(48).y(28))}));
    c.frames.push_back(fr(400, "", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(clock) {
  const std::string T2 = nextMin(T);
  {
    Clip c = mk('U', "clock", "Clock mode");
    c.frames.reserve(6);
    c.frames.push_back(fr(2500, "idle", {sm(Ex::Idle), TB(T, 25)}));
    c.frames.push_back(fr(150, "blink", {sm(Ex::Idle, FaceO().E(SHUT)), TB(T, 25)}));
    c.frames.push_back(fr(1500, "", {sm(Ex::Idle, FaceO().look(0, 0)), TB(T, 25)}));
    c.frames.push_back(fr(300, "minute flips: glance", {sm(Ex::Idle, FaceO().E(EyeO().brow(-1, 2))), TB(T2, 25)}));
    c.frames.push_back(fr(1200, "", {sm(Ex::Idle), TB(T2, 25)}));
    c.frames.push_back(fr(1500, "content", {sm(Ex::Sleepy, FaceO().y(31).look(0.5, 0)), TB(T2, 25)}));
    out.push_back(std::move(c));
  }
}

CLIP(clockshift) {
  static const int OFF[9][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
  auto base = std::make_shared<Grid>(), env = std::make_shared<Grid>();
  base->clear();
  env->clear();
  for (auto& d : std::vector<Draw>{sm(Ex::Idle), TB(T, 25)}) d(*base);
  std::vector<Frame> v;
  for (auto& o : OFF) {
        auto s = std::make_shared<Grid>();
        shiftG(*base, *s, o[0], o[1]);
        for (int k = 0; k < W * H; k++) env->d[k] |= s->d[k];
        Frame f; f.ms = 180000; f.previewMs = 700; f.note = "shift";
        f.ops.push_back([s](Grid& g) { g = *s; });
        v.push_back(std::move(f));
      }
  Frame f;
  f.ms = 0;
  f.previewMs = 1600;
  f.note = "all 9 overlaid (reference)";
  f.ops.push_back([env](Grid& g) { g = *env; });
  v.push_back(std::move(f));
  put(out, 'U', "clockshift", "Burn-in shift grid", v);
}

CLIP(menu) {
  struct M { Icon k; const char* lab; Ex ex; };
  static const M MENU[8] = {{Icon::Clock, "CLOCK", Ex::Smug}, {Icon::Alarm, "ALARM", Ex::Surprised}, {Icon::Focus, "FOCUS", Ex::Focus},
        {Icon::Games, "GAMES", Ex::Excited}, {Icon::Friends, "FRIENDS", Ex::Love}, {Icon::Chat, "MESSAGES", Ex::Curious},
        {Icon::Settings, "SETTINGS", Ex::Determined}, {Icon::Back, "BACK", Ex::Idle}};
  auto mcard = [](Icon k, int dx = 0, int dy = 0) { return fn([=](Grid& g) { card(g, 58 + dx, 5 + dy, 28, 26); stampIcon(g, k, 64 + dx, 10 + dy); }); };
  auto mdots = [](int i) { return fn([=](Grid& g) { for (int k = 0; k < 8; k++) { int x = 51 + k * 6; if (k == i) fillRect(g, x - 1, 54, x + 2, 57); else fillRect(g, x, 55, x + 1, 56); } }); };
  Draw arrows = fn([](Grid& g) { text(g, "<", 52, 15, Font::Small); text(g, ">", 91, 15, Font::Small); });
  auto spd = [](int x, int y0, int y1) { return fn([=](Grid& g) { speed(g, x, y0, y1); }); };
  std::vector<Frame> mf;
  for (int i = 0; i < 8; i++) {
        const M& m = MENU[i];
        FaceO o = (m.ex == Ex::Idle && m.k == Icon::Back) ? FaceO().x(23).look(-1, 0) : FaceO().x(23).look(1, -0.2);
        mf.push_back(fr(1100, i == 0 ? "clock (opens here)" : "", {sm(m.ex, o), mcard(m.k), tc(m.lab, 72, 40), mdots(i), arrows}));
        if (i < 7) {
          mf.push_back(fr(120, "tilt right: slide", {sm(Ex::Idle, FaceO().x(23).look(1, 0)), mcard(m.k, 5), spd(52, 9, 27), mdots(i)}));
          mf.push_back(fr(120, "swap", {sm(Ex::Idle, FaceO().x(23).look(1, -0.4)), mdots(i + 1)}));
          mf.push_back(fr(120, "next pops up", {sm(Ex::Idle, FaceO().x(23).look(1, -0.4)), mcard(MENU[i + 1].k, 0, -2), mdots(i + 1)}));
        }
      }
  put(out, 'U', "menu", "Menu", mf);
}

CLIP(setalarm) {
  auto aboard = [](std::string t, char u, int dy = 0) {
        return fn([=](Grid& g) {
          card(g, 56, 12 + dy, 60, 27); text(g, t.c_str(), 61, 16 + dy, Font::Big);
          int ux = u == 'h' ? 61 : u == 'm' ? 89 : -1;
          if (ux >= 0) fillRect(g, ux, 33 + dy, ux + 19, 34 + dy);
        });
      };
  auto A = [&](const char* t, char u, Ex ex, const FaceO& o, uint32_t ms, const char* note, int bdy, const std::vector<Draw>& more) {
        Frame f = fr(ms, note, {sm(ex, FaceO().look(1, -0.2).over(o)), aboard(t, u, bdy), tc("ALARM", 86, 2)});
        for (auto& d : more) f.ops.push_back(d);
        return f;
      };
  const FaceO up = FaceO().E(EyeO().brow(-1, 2));
  const Draw gt = tx(">", 119, 22, Font::Small);
  {
    Clip c = mk('U', "setalarm", "Set alarm");
    c.frames.reserve(14);
    c.frames.push_back(A("07:30", 'h', Ex::Idle, FaceO(), 400, "hours selected", 0, {}));
    c.frames.push_back(A("07:30", 0, Ex::Idle, FaceO(), 400, "underline blinks", 0, {}));
    c.frames.push_back(A("07:30", 'h', Ex::Idle, FaceO(), 400, "", 0, {}));
    c.frames.push_back(A("08:30", 'h', Ex::Idle, up, 250, "tilt right: +1 h", 0, {gt}));
    c.frames.push_back(A("08:30", 'h', Ex::Idle, FaceO(), 400, "", 0, {}));
    c.frames.push_back(A("08:30", 0, Ex::Idle, FaceO(), 400, "", 0, {}));
    c.frames.push_back(A("08:30", 'm', Ex::Idle, FaceO().look(1, 0), 400, "tap: minutes", 0, {}));
    c.frames.push_back(A("08:30", 0, Ex::Idle, FaceO().look(1, 0), 400, "", 0, {}));
    c.frames.push_back(A("08:35", 'm', Ex::Idle, up, 250, "tilt right: +5 min", 0, {gt}));
    c.frames.push_back(A("08:35", 'm', Ex::Idle, FaceO(), 400, "", 0, {}));
    c.frames.push_back(A("08:35", 0, Ex::Happy, FaceO().heart(), 900, "tap: confirmed", -2, {}));
    c.frames.push_back(A("08:35", 0, Ex::Happy, FaceO(), 500, "", 0, {}));
    c.frames.push_back(fr(140, "board away", {sm(Ex::Happy), aboard("08:35", 0, 14)}));
    c.frames.push_back(fr(400, "idle", {I()}));
    out.push_back(std::move(c));
  }
}

CLIP(alarm) {
  {
    Clip c = mk('U', "alarm", "Alarm ringing");
    c.frames.reserve(12);
    c.frames.push_back(fr(300, "stage 1: asleep", {fa(Ex::Asleep, FaceO().z(1)), ap(1)}));
    c.frames.push_back(fr(300, "brow twitch", {fa(Ex::Asleep, FaceO().E(EyeO().brow(1))), ap(2)}));
    c.frames.push_back(fr(400, "one eye opens", {fa(Ex::Idle, FaceO().L(EyeO().shape(Shape::ArcDown)).R(EyeO().lt(0.5).sl(0.3).brow(2)).m(Mouth::Flat)), ap(1)}));
    c.frames.push_back(fr(400, "", {fa(Ex::Idle, FaceO().L(EyeO().shape(Shape::ArcDown)).R(EyeO().lt(0.5).sl(0.3).brow(2).px(0.8).py(0.6)).m(Mouth::Grr)), ap(2)}));
    c.frames.push_back(fr(250, "stage 2: awake", {fa(Ex::Surprised, FaceO().dx(1).bang()), ap(1)}));
    c.frames.push_back(fr(250, "", {fa(Ex::Surprised, FaceO().dx(-1)), ap(2)}));
    c.frames.push_back(fr(250, "", {fa(Ex::Scared, FaceO().dx(1)), ap(1)}));
    c.frames.push_back(fr(250, "louder", {fa(Ex::Surprised, FaceO().dx(-1).bang(2)), ap(2)}));
    c.frames.push_back(fr(200, "stage 3: RAWR", {fa(Ex::Yawn, FaceO().m(Mouth::Rawr).steam().dx(2)), ap(1)}));
    c.frames.push_back(fr(200, "", {fa(Ex::Yawn, FaceO().m(Mouth::Rawr).steam().dx(-2).dy(1)), ap(2)}));
    c.frames.push_back(fr(220, "peak drama", {fa(Ex::Rawr, FaceO().bang(2).dy(-1)), ap(1)}));
    c.frames.push_back(fr(200, "", {fa(Ex::Yawn, FaceO().m(Mouth::Rawr).steam().dx(2)), ap(2)}));
    out.push_back(std::move(c));
  }
}

CLIP(snooze) {
  auto plus5 = fn([](Grid& g) { alarmProp(g, 0); text(g, "+5", 111, 36, Font::Small); });
  {
    Clip c = mk('U', "snooze", "Snooze");
    c.frames.reserve(5);
    c.frames.push_back(fr(600, "grumble", {fa(Ex::Grumpy), ap(0)}));
    c.frames.push_back(fr(800, "+5 min", {fa(Ex::Grumpy, FaceO().steam(0)), plus5}));
    c.frames.push_back(fr(500, "eyes droop", {fa(Ex::Sleepy), plus5}));
    c.frames.push_back(fr(1400, "asleep", {fa(Ex::Asleep), ap(0)}));
    c.frames.push_back(fr(1400, "", {fa(Ex::Asleep, FaceO().z(2).y(30)), ap(0)}));
    out.push_back(std::move(c));
  }
}

CLIP(dismiss) {
  {
    Clip c = mk('U', "dismiss", "Dismiss");
    c.frames.reserve(6);
    c.frames.push_back(fr(1000, "big yawn", {fa(Ex::Yawn), ap(0)}));
    c.frames.push_back(fr(400, "", {fa(Ex::Sleepy), ap(0)}));
    c.frames.push_back(fr(150, "", {I(FaceO().E(SHUT))}));
    c.frames.push_back(fr(300, "", {I()}));
    c.frames.push_back(fr(900, "good morning", {fa(Ex::Happy)}));
    c.frames.push_back(fr(1400, "", {fa(Ex::Love)}));
    out.push_back(std::move(c));
  }
}

CLIP(focuspick) {
  auto fb = [](const char* n, int dy = 0) {
        std::string s = n;
        return fn([=](Grid& g) {
          card(g, 56, 10 + dy, 48, 26); text(g, s.c_str(), 70, 15 + dy, Font::Big); text(g, "<", 59, 18 + dy, Font::Small);
          text(g, ">", 98, 18 + dy, Font::Small); textC(g, "MIN", 80, 42 + dy);
        });
      };
  {
    Clip c = mk('U', "focuspick", "Focus: pick length");
    c.frames.reserve(9);
    c.frames.push_back(fr(1200, "25 min", {sm(Ex::Focus), fb("25")}));
    c.frames.push_back(fr(150, "tilt right", {sm(Ex::Focus, FaceO().dx(1)), fb("45", -1)}));
    c.frames.push_back(fr(1000, "45 min", {sm(Ex::Focus), fb("45")}));
    c.frames.push_back(fr(150, "tilt right", {sm(Ex::Focus, FaceO().dx(1)), fb("60", -1)}));
    c.frames.push_back(fr(1000, "60 min", {sm(Ex::Focus), fb("60")}));
    c.frames.push_back(fr(150, "wraps to 25", {sm(Ex::Focus, FaceO().look(-1, 0)), fb("25", -1)}));
    c.frames.push_back(fr(900, "", {sm(Ex::Focus), fb("25")}));
    c.frames.push_back(fr(400, "flip: serious nod", {sm(Ex::Focus, FaceO().E(SHUT).dy(2)), fb("25")}));
    c.frames.push_back(fr(150, "board away", {sm(Ex::Focus), fb("25", 20)}));
    out.push_back(std::move(c));
  }
}

CLIP(focus) {
  auto fw = [](const FaceO& o, const char* t, uint32_t ms, const char* note, uint32_t pms = 0) { return fr(ms, note, {fa(Ex::Focus, o), tx(t, 114, 1, Font::Small)}, pms); };
  {
    Clip c = mk('U', "focus", "Focus: working");
    c.frames.reserve(5);
    c.frames.push_back(fw(FaceO(), "24", 6000, "working", 1400));
    c.frames.push_back(fw(FaceO().E(SHUT), "24", 160, "blink"));
    c.frames.push_back(fw(FaceO(), "24", 6000, "", 1400));
    c.frames.push_back(fw(FaceO().E(EyeO().lt(0.15).py(-0.3)), "24", 500, "glance at you"));
    c.frames.push_back(fw(FaceO(), "23", 6000, "minute passes", 1400));
    out.push_back(std::move(c));
  }
}

CLIP(focusdone) {
  {
    Clip c = mk('U', "focusdone", "Focus: done");
    c.frames.reserve(4);
    c.frames.push_back(fr(500, "time up", {fa(Ex::Surprised, FaceO().glasses().bang()), tx("00", 114, 1, Font::Small)}));
    c.frames.push_back(fr(900, "stars", {fa(Ex::Excited)}));
    c.frames.push_back(fr(900, "", {fa(Ex::Happy)}));
    c.frames.push_back(fr(1200, "proud", {fa(Ex::Smug, FaceO().E(EyeO().px(0)))}));
    out.push_back(std::move(c));
  }
}

CLIP(break) {
  auto bx = [](const char* s, real y) { return tc(s, 114, y); };
  const FaceO big = FaceO().E(EyeO().h(35).w(31).brow(-1, 0)).y(26);
  {
    Clip c = mk('U', "break", "Breathing break");
    c.frames.reserve(9);
    c.frames.push_back(fr(800, "looks at you", {fa(Ex::Happy), bx("BREAK", 56)}));
    c.frames.push_back(fr(1300, "in", {I(FaceO().E(EyeO().h(31).w(29)).y(27)), bx("IN", 56)}));
    c.frames.push_back(fr(1300, "", {I(FaceO().E(EyeO().h(33).w(30)).y(26)), bx("IN", 56)}));
    c.frames.push_back(fr(1400, "", {I(big), bx("IN", 56)}));
    c.frames.push_back(fr(4000, "hold", {I(big), bx("HOLD", 56)}));
    c.frames.push_back(fr(1300, "out", {I(FaceO().E(EyeO().lt(0.15)).y(26)), bx("OUT", 56)}));
    c.frames.push_back(fr(1300, "", {I(FaceO().E(EyeO().lt(0.3)).y(28)), bx("OUT", 56)}));
    c.frames.push_back(fr(1400, "", {fa(Ex::Sleepy, FaceO().y(29)), bx("OUT", 56)}));
    c.frames.push_back(fr(1000, "again", {fa(Ex::Happy)}));
    out.push_back(std::move(c));
  }
}

CLIP(stopwatch) {
  auto sw = [&](const char* t, Ex ex, const FaceO& o, uint32_t ms, const char* note) { return fr(ms, note, {sm(ex, o), TB(t, 25)}); };
  {
    Clip c = mk('U', "stopwatch", "Stopwatch");
    c.frames.reserve(8);
    c.frames.push_back(sw("00:58", Ex::Idle, FaceO(), 1000, ""));
    c.frames.push_back(sw("00:59", Ex::Idle, FaceO().look(1, 0).E(EyeO().brow(-1, 2)), 1000, "watching"));
    c.frames.push_back(sw("01:00", Ex::Idle, FaceO().E(SHUT).dy(2), 200, "nod"));
    c.frames.push_back(sw("01:00", Ex::Idle, FaceO(), 150, ""));
    c.frames.push_back(sw("01:00", Ex::Idle, FaceO().E(SHUT).dy(2), 200, "nod"));
    c.frames.push_back(sw("01:00", Ex::Happy, FaceO(), 800, "pleased"));
    c.frames.push_back(sw("01:01", Ex::Idle, FaceO(), 1000, ""));
    c.frames.push_back(sw("01:02", Ex::Idle, FaceO(), 1000, ""));
    out.push_back(std::move(c));
  }
}

CLIP(lowbatt) {
  {
    Clip c = mk('U', "lowbatt", "Low battery");
    c.frames.reserve(5);
    c.frames.push_back(fr(800, "", {I(), bt(106, 50, 1)}));
    c.frames.push_back(fr(600, "notices", {I(FaceO().look(1, 0.8).E(EyeO().brow(-1, 2))), bt(106, 50, 1)}));
    c.frames.push_back(fr(1000, "hungry", {fa(Ex::Hungry, FaceO().look(1, 0.6)), bt(106, 50, 1)}));
    c.frames.push_back(fr(900, "", {fa(Ex::Sad), bt(106, 50, 1)}));
    c.frames.push_back(fr(900, "drowsy", {fa(Ex::Sleepy), bt(106, 50, 1)}));
    out.push_back(std::move(c));
  }
}

CLIP(charging) {
  auto lf = [](int k) {
        return fn([=](Grid& g) { if (k > 0) stampIcon(g, Icon::Leaf, 96, 44); if (k == 2) fillRect(g, 96, 44, 103, 59, 0); });
      };
  {
    Clip c = mk('U', "charging", "Charging");
    c.frames.reserve(8);
    c.frames.push_back(fr(300, "plugged in", {fa(Ex::Surprised, FaceO().bang()), bt(108, 2, 1)}));
    c.frames.push_back(fr(500, "", {fa(Ex::Excited), bt(108, 2, 1), lf(1)}));
    c.frames.push_back(fr(200, "chomp", {fa(Ex::Chew, FaceO().m(Mouth::ChompOpen).look(0.6, 0.4)), bt(108, 2, 1), lf(1)}));
    c.frames.push_back(fr(200, "", {fa(Ex::Chew), bt(108, 2, 1), lf(2)}));
    c.frames.push_back(fr(200, "", {fa(Ex::Chew, FaceO().m(Mouth::ChompOpen)), bt(108, 2, 2), lf(2)}));
    c.frames.push_back(fr(200, "", {fa(Ex::Chew), bt(108, 2, 2)}));
    c.frames.push_back(fr(600, "munching", {fa(Ex::Chew, FaceO().dy(1)), bt(108, 2, 3)}));
    c.frames.push_back(fr(1400, "full", {fa(Ex::Love), bt(108, 2, 3), tx("FULL", 104, 56)}));
    out.push_back(std::move(c));
  }
}

CLIP(settings) {
  auto sb = [](const char* a, const char* b) {
        std::string A = a, Bs = b;
        return fn([=](Grid& g) {
          card(g, 52, 10, 72, 30); textC(g, A.c_str(), 88, 15); textC(g, Bs.c_str(), 88, 27); text(g, "<", 46, 21, Font::Small); text(g, ">", 125, 21, Font::Small);
        });
      };
  auto spdF = fn([](Grid& g) { speed(g, 48, 14, 32); });
  {
    Clip c = mk('U', "settings", "Settings");
    c.frames.reserve(8);
    c.frames.push_back(fr(900, "sound on", {sm(Ex::Happy, FaceO().note()), sb("SOUND", "ON")}));
    c.frames.push_back(fr(900, "tap: off", {sm(Ex::Idle, FaceO().E(EyeO().lt(0.45).lb(0.2)).m(Mouth::Flat)), sb("SOUND", "OFF")}));
    c.frames.push_back(fr(120, "tilt", {sm(Ex::Idle), spdF}));
    c.frames.push_back(fr(900, "light 1", {sm(Ex::Sleepy), sb("LIGHT", "1")}));
    c.frames.push_back(fr(700, "light 2", {sm(Ex::Idle), sb("LIGHT", "2")}));
    c.frames.push_back(fr(900, "light 3", {sm(Ex::Idle, FaceO().E(EyeO().lt(0.3).lb(0.3))), sb("LIGHT", "3")}));
    c.frames.push_back(fr(120, "tilt", {sm(Ex::Idle), spdF}));
    c.frames.push_back(fr(1200, "name", {sm(Ex::Smug, FaceO().E(EyeO().px(0))), sb("NAME", "DESKI")}));
    out.push_back(std::move(c));
  }
}

struct Entry { const char* key; void (*build)(std::vector<Clip>&); };
static const Entry TABLE[] = {
  {"rig_eyes", c_rig_eyes},
  {"rig_brows", c_rig_brows},
  {"rig_mouths", c_rig_mouths},
  {"rig_type", c_rig_type},
  {"rig_icons", c_rig_icons},
  {"expressions", c_expressions},
  {"scale", c_scale},
  {"idle", c_idle},
  {"pet", c_pet},
  {"overpet", c_overpet},
  {"rawr", c_rawr},
  {"shake", c_shake},
  {"pickup", c_pickup},
  {"tilt", c_tilt},
  {"sleep", c_sleep},
  {"wake", c_wake},
  {"time", c_time},
  {"clock", c_clock},
  {"clockshift", c_clockshift},
  {"menu", c_menu},
  {"setalarm", c_setalarm},
  {"alarm", c_alarm},
  {"snooze", c_snooze},
  {"dismiss", c_dismiss},
  {"focuspick", c_focuspick},
  {"focus", c_focus},
  {"focusdone", c_focusdone},
  {"break", c_break},
  {"stopwatch", c_stopwatch},
  {"lowbatt", c_lowbatt},
  {"charging", c_charging},
  {"settings", c_settings},
};
}  // namespace

void renderFrame(const Frame& f, Grid& g) {
  g.clear();
  for (auto& d : f.ops) d(g);
  if (f.shiftX || f.shiftY) {  // burn-in shift of the whole frame (clock, stopwatch, focus)
    static Grid tmp;           // 8 KB, static: too big for the stack
    shiftG(g, tmp, f.shiftX, f.shiftY);
    g = tmp;
  }
}

void buildDino(std::vector<Clip>& out, const std::string& time, const std::string& friendName, const char* onlyKey) {
  out.clear();
  T = time; FN = friendName;
  for (const Entry& e : TABLE) if (!onlyKey || strcmp(onlyKey, e.key) == 0) e.build(out);
}

}  // namespace scenes
