// Live dino clips: see dino_live.h. The screens mirror the design sheet's Utility clips (dino_clips.cpp),
// with the brain's live values. Where a sheet clip needs no live value (alarm stages, snooze, dismiss, focus done,
// breathing break) the sheet's own frames are used: built one clip at a time (buildDino onlyKey), then moved.
#include "dino_live.h"
#include <string.h>
#include "dino_parts.h"

namespace scenes {

using UClip = utility::Clip;  // the brain's clip ids (scenes::Clip is the design sheet's clip)
using utility::View;

namespace {

// ---- menu: 6 cards (the brain's slots). Clock, Alarm, Focus, Stopwatch, Wi-Fi, Back. Games/Friends/Messages/Settings of
// the sheet's 8-card menu come when those features exist.
constexpr int SLOTS = utility::Machine::MENU_SLOTS;
struct Card { Icon icon; const char* label; Ex ex; };
const Card CARDS[SLOTS] = {{Icon::Clock, "CLOCK", Ex::Smug}, {Icon::Alarm, "ALARM", Ex::Surprised}, {Icon::Focus, "FOCUS", Ex::Focus},
                           {Icon::Clock, "STOPWATCH", Ex::Determined}, {Icon::Clock, "WI-FI", Ex::Curious}, {Icon::Back, "BACK", Ex::Idle}};
constexpr int STOPWATCH_SLOT = 3, WIFI_SLOT = 4, BACK_SLOT = 5;  // slots whose icon is drawn in code (the sheet has none) / the Back card

// The Wi-Fi logo (the sheet has none): a dot and up to `level` (0-3) arcs above it, drawn from the dot at (cx, cy).
void wifiLogo(Grid& g, int cx, int cy, int level) {
  fillRect(g, cx - 1, cy - 1, cx, cy);
  static const real R[3] = {5, 9, 13};
  for (int i = 0; i < level && i < 3; i++) {
    const real r = R[i], ccx = cx, ccy = cy;
    area(g, [=](real x, real y) {
      const real dx = x - ccx, dy = y - ccy;
      return dy < 0 && -dy >= fabs(dx) * 0.9 && fabs(sqrt(dx * dx + dy * dy) - r) <= 0.8;  // a 90-degree arc, about 1.5 px thick
    }, bE(cx, cy, r, r, 1));
  }
}

// The sheet has no stopwatch icon: a small one in the same 16x16, 1 px line style.
void stopwatchIcon(Grid& g, int x, int y) {
  ring(g, ell(x + 7.5, y + 9.5, 6.5, 6.5), bE(x + 7.5, y + 9.5, 6.5, 6.5));
  fillRect(g, x + 6, y, x + 9, y + 1);
  line(g, x + 12, y + 3, x + 14, y + 1);
  line(g, x + 7, y + 9, x + 7, y + 5);
  line(g, x + 7, y + 9, x + 10, y + 9);
}
Draw mcard(int slot, int dx = 0, int dy = 0) {
  return fn([=](Grid& g) {
    card(g, 58 + dx, 5 + dy, 28, 26);
    if (slot == STOPWATCH_SLOT) stopwatchIcon(g, 64 + dx, 10 + dy);
    else if (slot == WIFI_SLOT) wifiLogo(g, 72 + dx, 24 + dy, 3);
    else stampIcon(g, CARDS[slot].icon, 64 + dx, 10 + dy);
  });
}
Draw mdots(int i) {
  return fn([=](Grid& g) {
    for (int k = 0; k < SLOTS; k++) { int x = 57 + k * 6; if (k == i) fillRect(g, x - 1, 54, x + 2, 57); else fillRect(g, x, 55, x + 1, 56); }
  });
}
// Face in the menu: 1 px left of the sheet anchor (ADR 004) so it never touches the arrow.
FaceO menuLook(int slot, real ly = -0.2) { return slot == BACK_SLOT ? FaceO().x(23).look(-1, 0) : FaceO().x(23).look(1, ly); }
Draw menuArrows() { return fn([](Grid& g) { text(g, "<", 52, 15, Font::Small); text(g, ">", 91, 15, Font::Small); }); }
Draw spd(int x, int y0, int y1) { return fn([=](Grid& g) { speed(g, x, y0, y1); }); }

bool knownTime(const char* t) {  // "HH:MM" (digits and a colon) vs "--:--" before the first sync
  for (int i = 0; t[i]; i++) if (!((t[i] >= '0' && t[i] <= '9') || t[i] == ':')) return false;
  return t[0] != 0;
}
Draw timeBig(const std::string& t, real y) {
  return knownTime(t.c_str()) ? tx(t, 62, y, Font::Big) : tx(t, 72, y + 3, Font::Small);
}

// Burn-in: the brain's position 0..8 -> the sheet's 3x3 ring (same table as the clockshift clip).
void applyShift(LiveClip& c, int pos) {
  static const int8_t OFF[9][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
  pos %= 9;
  for (auto& f : c.frames) { f.shiftX = OFF[pos][0]; f.shiftY = OFF[pos][1]; }
}

// Moves frames [from, to) of a sheet clip into `out` (to < 0 = all).
void takeSheet(LiveClip& out, const char* key, const std::string& time, int from = 0, int to = -1) {
  std::vector<scenes::Clip> tmp;
  buildDino(tmp, time, "SAM", key);
  if (tmp.empty()) return;
  auto& fr_ = tmp[0].frames;
  const int n = (int)fr_.size();
  if (to < 0 || to > n) to = n;
  for (int i = from; i < to; i++) out.frames.push_back(std::move(fr_[i]));
}

// ---- the clips ----
__attribute__((noinline)) void menu(LiveClip& c, const View& v) {
  const int s = v.a % SLOTS;
  c.frames.push_back(fr(1100, "", {sm(CARDS[s].ex, menuLook(s)), mcard(s), tc(CARDS[s].label, 72, 40), mdots(s), menuArrows()}));
}
__attribute__((noinline)) void menuSwap(LiveClip& c, const View& v) {
  const int a = v.a % SLOTS, b = v.b % SLOTS;
  c.frames.push_back(fr(120, "slide", {sm(Ex::Idle, FaceO().x(23).look(1, 0)), mcard(a, 5), spd(52, 9, 27), mdots(a)}));
  c.frames.push_back(fr(120, "swap", {sm(Ex::Idle, FaceO().x(23).look(1, -0.4)), mdots(b)}));
  c.frames.push_back(fr(120, "pop", {sm(Ex::Idle, FaceO().x(23).look(1, -0.4)), mcard(b, 0, -2), mdots(b)}));
  c.loop = false;
}

// Alarm board (set alarm): 'h' / 'm' underline the field, 0 = none.
Draw aboard(std::string t, char u, int dy = 0) {
  return fn([=](Grid& g) {
    card(g, 56, 12 + dy, 60, 27);
    if (knownTime(t.c_str())) text(g, t.c_str(), 61, 16 + dy, Font::Big); else text(g, t.c_str(), 71, 19 + dy, Font::Small);
    const int ux = u == 'h' ? 61 : u == 'm' ? 89 : -1;
    if (ux >= 0) fillRect(g, ux, 33 + dy, ux + 19, 34 + dy);
  });
}
Frame aframe(const View& v, char u, Ex ex, const FaceO& o, uint32_t ms, int bdy = 0) {
  return fr(ms, "", {sm(ex, FaceO().look(1, -0.2).over(o)), aboard(v.text, u, bdy), tc("ALARM", 86, 2)});
}
__attribute__((noinline)) void setAlarm(LiveClip& c, const View& v) {
  const char u = v.a == 0 ? 'h' : 'm';
  c.frames.push_back(aframe(v, u, Ex::Idle, FaceO(), 400));   // underline on
  c.frames.push_back(aframe(v, 0, Ex::Idle, FaceO(), 400));   // underline off: blinks at 1.25 Hz (the only blinking thing)
}
__attribute__((noinline)) void setAlarmTilt(LiveClip& c, const View& v) {
  Frame f = aframe(v, v.a == 0 ? 'h' : 'm', Ex::Idle, FaceO().E(EyeO().brow(-1, 2)), 250);
  f.ops.push_back(v.b > 0 ? tx(">", 119, 22, Font::Small) : tx("<", 51, 22, Font::Small));
  c.frames.push_back(std::move(f));
  c.loop = false;
}
__attribute__((noinline)) void setAlarmDone(LiveClip& c, const View& v) {
  // v.note = time until it rings ("+23:58"), under the board, so a time already past is obvious
  const std::string note = v.note;
  Draw noteOp = fn([note](Grid& g) { if (!note.empty()) text(g, note.c_str(), 86 - textW(note.c_str(), Font::Small) / 2, 43, Font::Small); });
  Frame a = aframe(v, 0, Ex::Happy, FaceO().heart(), 900, -2); a.ops.push_back(noteOp);
  Frame b = aframe(v, 0, Ex::Happy, FaceO(), note.empty() ? 500 : 1500); b.ops.push_back(noteOp);
  c.frames.push_back(std::move(a));
  c.frames.push_back(std::move(b));
  c.frames.push_back(fr(140, "board away", {sm(Ex::Happy), aboard(v.text, 0, 14)}));
  c.frames.push_back(fr(400, "", {I()}));
  c.loop = false;
}

Draw fboard(int mins, int dy = 0) {
  char n[6]; snprintf(n, sizeof n, "%d", mins);
  std::string s = n;
  return fn([=](Grid& g) {
    card(g, 56, 10 + dy, 48, 26); text(g, s.c_str(), 70, 15 + dy, Font::Big); text(g, "<", 59, 18 + dy, Font::Small);
    text(g, ">", 98, 18 + dy, Font::Small); textC(g, "MIN", 80, 42 + dy);
  });
}

__attribute__((noinline)) void alarmRing(LiveClip& c, const View& v, const std::string& t) {
  const int stage = v.a < 1 ? 1 : v.a > 3 ? 3 : v.a;  // the sheet's alarm clip is 3 stages x 4 frames
  takeSheet(c, "alarm", t, (stage - 1) * 4, stage * 4);
}
__attribute__((noinline)) void focusPick(LiveClip& c, const View& v) {
  c.frames.push_back(fr(1000, "", {sm(Ex::Focus), fboard(v.a)}));
}
__attribute__((noinline)) void focusPickTilt(LiveClip& c, const View& v) {
  c.frames.push_back(fr(150, "", {sm(Ex::Focus, FaceO().dx(1)), fboard(v.a, -1)}));
  c.loop = false;
}
__attribute__((noinline)) void focusStart(LiveClip& c, const View& v) {
  c.frames.push_back(fr(400, "flip: serious nod", {sm(Ex::Focus, FaceO().E(SHUT).dy(2)), fboard(v.a)}));
  c.frames.push_back(fr(150, "board away", {sm(Ex::Focus), fboard(v.a, 20)}));
  c.loop = false;
}
__attribute__((noinline)) void focusWork(LiveClip& c, const View& v) {
  const std::string t = v.text;
  auto w = [&](const FaceO& o, uint32_t ms) { return fr(ms, "", {fa(Ex::Focus, o), tx(t, 114, 1, Font::Small)}); };
  c.frames.push_back(w(FaceO(), 6000));
  c.frames.push_back(w(FaceO().E(SHUT), 160));
  c.frames.push_back(w(FaceO(), 6000));
  c.frames.push_back(w(FaceO().E(EyeO().lt(0.15).py(-0.3)), 500));  // glance up at you
  applyShift(c, v.shift);
}
__attribute__((noinline)) void stopwatch(LiveClip& c, const View& v) {
  c.frames.push_back(fr(1000, "", {sm(Ex::Idle), timeBig(v.text, 25)}));
  applyShift(c, v.shift);
}
__attribute__((noinline)) void stopwatchTap(LiveClip& c, const View& v) {
  auto s = [&](Ex ex, const FaceO& o, uint32_t ms) { return fr(ms, "", {sm(ex, o), timeBig(v.text, 25)}); };
  c.frames.push_back(s(Ex::Idle, FaceO().E(SHUT).dy(2), 200));  // nod on the minute
  c.frames.push_back(s(Ex::Idle, FaceO(), 150));
  c.frames.push_back(s(Ex::Idle, FaceO().E(SHUT).dy(2), 200));
  c.frames.push_back(s(Ex::Happy, FaceO(), 800));
  c.loop = false;
  applyShift(c, v.shift);
}
__attribute__((noinline)) void timeBoard(LiveClip& c, const View& v) {
  if (knownTime(v.text)) { takeSheet(c, "time", v.text); c.loop = false; return; }
  // clock not synced yet: the sheet's digit drop needs digits; show "--:--" small beside the face instead
  c.frames.push_back(fr(300, "", {I(FaceO().E(EyeO().brow(-1, 3)).look(0, -0.3))}));
  c.frames.push_back(fr(1500, "", {sm(Ex::Idle), tx(v.text, 72, 28, Font::Small)}));
  c.frames.push_back(fr(300, "", {I()}));
  c.loop = false;
}

// ---- Wi-Fi setup ("connect your dino"). The dino looks for Wi-Fi: its eyes sweep around while the logo's arcs light up.
Draw wifiAt(int cx, int cy, int level, bool cross = false) {
  return fn([=](Grid& g) {
    wifiLogo(g, cx, cy, level);
    if (cross) { line(g, cx - 7, cy - 12, cx + 7, cy + 1); line(g, cx - 7, cy - 11, cx + 7, cy + 2); }  // "no Wi-Fi": a slash through it
  });
}
Draw lines3(const char* a, const char* b, const char* c) {
  return fn([=](Grid& g) { textC(g, a, 88, 31); textC(g, b, 88, 40); if (c) textC(g, c, 88, 49); });
}

// Waiting for a phone: three screens in a loop. 1: the message with the logo searching. 2: how to join (network + password).
__attribute__((noinline)) void wifiWaiting(LiveClip& c, const View& v) {
  static const struct { real lx, ly; } SWEEP[4] = {{1, -0.5}, {0.2, -0.6}, {-0.5, -0.2}, {1, -0.4}};  // up-right, up, left, back to the logo
  const char* const title[3] = {"CONNECT YOUR", "DINO TO SHOW", "THE TIME"};
  for (int k = 0; k < 8; k++) {   // two sweeps of the logo: 0, 1, 2, 3 arcs
    const int level = k % 4;
    c.frames.push_back(fr(300, "", {sm(Ex::Curious, FaceO().look(SWEEP[level].lx, SWEEP[level].ly)), wifiAt(88, 23, level),
                                    lines3(title[0], title[1], title[2])}));
  }
  c.frames.push_back(fr(90, "blink", {sm(Ex::Curious, FaceO().E(SHUT)), wifiAt(88, 23, 3), lines3(title[0], title[1], title[2])}));
  // join info: "JOIN WI-FI" / DINO-1234 / "PASSWORD" / XXXXX XXXXX
  std::string code = v.code;
  if (code.size() == 10) code.insert(5, " ");   // "XXXXX XXXXX": easier to read and type
  const std::string ssid = v.ssid[0] ? v.ssid : "...";
  Draw info = fn([=](Grid& g) {
    textC(g, "JOIN WI-FI", 88, 6); textC(g, ssid.c_str(), 88, 17);
    textC(g, "PASSWORD", 88, 33); textC(g, code.c_str(), 88, 44);
  });
  c.frames.push_back(fr(3600, "join", {sm(Ex::Idle, FaceO().look(1, -0.1)), info}));
  c.frames.push_back(fr(90, "blink", {sm(Ex::Idle, FaceO().E(SHUT)), info}));
  c.frames.push_back(fr(900, "", {sm(Ex::Idle, FaceO().look(1, -0.1)), info}));
}
// A phone is on the setup network: the page is open there. The logo is lit and pulses.
__attribute__((noinline)) void wifiJoined(LiveClip& c) {
  const Draw t = lines3("FINISH ON", "YOUR PHONE", nullptr);
  c.frames.push_back(fr(500, "", {sm(Ex::Happy, FaceO().look(1, -0.3)), wifiAt(88, 23, 3), t}));
  c.frames.push_back(fr(300, "", {sm(Ex::Happy, FaceO().look(1, -0.3)), wifiAt(88, 23, 2), t}));
  c.frames.push_back(fr(500, "", {sm(Ex::Happy, FaceO().look(1, -0.3)), wifiAt(88, 23, 3), t}));
  c.frames.push_back(fr(90, "blink", {sm(Ex::Happy, FaceO().E(SHUT)), wifiAt(88, 23, 3), t}));
}
// Trying your home Wi-Fi: the logo fills up quickly, the dino concentrates.
__attribute__((noinline)) void wifiConnecting(LiveClip& c) {
  for (int level = 0; level < 4; level++)
    c.frames.push_back(fr(220, "", {sm(Ex::Focus, FaceO().look(1, -0.3)), wifiAt(88, 23, level), lines3("CONNECTING", "", nullptr)}));
}
__attribute__((noinline)) void wifiPair(LiveClip& c, const View& v) {
  if (v.a == 1) wifiJoined(c);
  else if (v.a == 2) wifiConnecting(c);
  else wifiWaiting(c, v);
}
// Connected: happy, then the time (or "getting the time" until the sync lands; the clip refreshes when it does).
__attribute__((noinline)) void wifiDone(LiveClip& c, const View& v) {
  c.frames.push_back(fr(1100, "", {sm(Ex::Happy, FaceO().heart()), wifiAt(88, 23, 3), lines3("CONNECTED", "", nullptr)}));
  if (knownTime(v.text)) c.frames.push_back(fr(1800, "", {sm(Ex::Happy), timeBig(v.text, 25)}));
  else c.frames.push_back(fr(1800, "", {sm(Ex::Idle, FaceO().look(1, -0.3)), lines3("GETTING THE", "TIME", nullptr)}));
  c.frames.push_back(fr(400, "", {I()}));
  c.loop = false;
}
__attribute__((noinline)) void wifiFail(LiveClip& c) {
  c.frames.push_back(fr(1000, "", {sm(Ex::Sad), wifiAt(88, 23, 3, true), lines3("NO LUCK", "", nullptr)}));
  c.frames.push_back(fr(1600, "", {sm(Ex::Sad), lines3("CHECK THE", "PASSWORD", nullptr)}));
  c.loop = false;
}

}  // namespace

void buildLive(const View& v, LiveClip& out) {
  out.frames.clear();
  out.loop = true;
  switch (v.clip) {
    case UClip::TimeBoard: timeBoard(out, v); break;
    case UClip::Menu: menu(out, v); break;
    case UClip::MenuSwap: menuSwap(out, v); break;
    case UClip::SetAlarm: setAlarm(out, v); break;
    case UClip::SetAlarmTilt: setAlarmTilt(out, v); break;
    case UClip::SetAlarmDone: setAlarmDone(out, v); break;
    case UClip::AlarmRing: alarmRing(out, v, "10:24"); break;
    case UClip::Snooze: takeSheet(out, "snooze", "10:24"); out.loop = false; break;
    case UClip::Dismiss: takeSheet(out, "dismiss", "10:24"); out.loop = false; break;
    case UClip::FocusPick: focusPick(out, v); break;
    case UClip::FocusPickTilt: focusPickTilt(out, v); break;
    case UClip::FocusStart: focusStart(out, v); break;
    case UClip::FocusWork: focusWork(out, v); break;
    case UClip::FocusDone: takeSheet(out, "focusdone", "10:24"); out.loop = false; break;
    case UClip::Break: takeSheet(out, "break", "10:24"); break;
    case UClip::Stopwatch: stopwatch(out, v); break;
    case UClip::StopwatchTap: stopwatchTap(out, v); break;
    case UClip::WifiPair: wifiPair(out, v); break;
    case UClip::WifiDone: wifiDone(out, v); break;
    case UClip::WifiFail: wifiFail(out); break;
    case UClip::Home: default:  // the home screen belongs to the life module (HomeScene::buildHome); a plain idle face as the fallback
      out.frames.push_back(fr(2400, "", {I()}));
      out.frames.push_back(fr(60, "", {I(FaceO().E(HALF))}));
      out.frames.push_back(fr(80, "", {I(FaceO().E(SHUT))}));
      out.frames.push_back(fr(60, "", {I(FaceO().E(HALF))}));
      break;
  }
}

}  // namespace scenes
