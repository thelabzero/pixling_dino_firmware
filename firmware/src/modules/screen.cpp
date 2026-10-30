#include "screen.h"
#include <Wire.h>
#include <string.h>
#include "config.h"
#include "../app/app.h"

Screen::Screen() : display_(SCREEN_W, SCREEN_H, &Wire, OLED_RESET) {}

bool Screen::begin(App& app) {
  if (!display_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) return false;
  Serial.printf("[screen] SSD1306 at 0x%02X\n", OLED_ADDR);
  update(app, millis());
  return true;
}

void Screen::loop(App& app, uint32_t now) {
  update(app, now);
  advance(app, now);
}

// Values that change inside a clip without restarting it (the minute, a countdown...).
static bool sameValues(const utility::View& a, const utility::View& b) {
  return strcmp(a.text, b.text) == 0 && strcmp(a.note, b.note) == 0 && a.shift == b.shift &&
         a.night == b.night && a.clockMode == b.clockMode;
}

void Screen::build(App& app, const utility::View& v) {
  if (home_) app.homeScene()->buildHome(v, clip_);
  else scenes::buildLive(v, clip_);
}

// New clip -> start from frame 0. Same clip with new values -> rebuild, keep the position.
void Screen::update(App& app, uint32_t now) {
  const utility::View& v = app.machine().view();
  HomeScene* hs = v.clip == utility::Clip::Home ? app.homeScene() : nullptr;
  const bool home = hs != nullptr;
  const bool restart = v.serial != playing_.serial || home != home_ || (home && hs->homeSerial() != homeSerial_);
  if (restart) {
    playing_ = v;
    home_ = home;
    if (home) homeSerial_ = hs->homeSerial();
    build(app, v);
    finished_ = false;
    if (!clip_.frames.empty()) show(0, now);
  } else if (!sameValues(v, playing_)) {
    playing_ = v;
    build(app, v);
    if (frame_ >= clip_.frames.size()) frame_ = 0;
    if (!clip_.frames.empty()) show(frame_, now);
  }
  brightness(app);
}

// Night brightness only on the home screen while the dino sleeps; an alarm is always full brightness.
void Screen::brightness(App& app) {
  const bool dim = home_ && app.homeScene() && app.homeScene()->dimmed();
  const int want = dim ? SCREEN_NIGHT_CONTRAST : SCREEN_DAY_CONTRAST;
  if (want == contrast_) return;
  contrast_ = want;
  display_.ssd1306_command(SSD1306_SETCONTRAST);
  display_.ssd1306_command((uint8_t)want);
}

int Screen::nextIndex() const {
  if (frame_ + 1 < clip_.frames.size()) return (int)frame_ + 1;
  if (clip_.loop && clip_.frames.size() > 1) return 0;
  return -1;
}

void Screen::advance(App& app, uint32_t now) {
  if (finished_ || clip_.frames.empty()) return;
  if (now - frameAt_ < clip_.frames[frame_].ms) return;  // keep showing this frame
  const int next = nextIndex();
  if (next >= 0) {
    if (prepared_ == next) { face::Grid* t = shown_; shown_ = prep_; prep_ = t; }  // already drawn while the last one was held
    else scenes::renderFrame(clip_.frames[next], *shown_);
    present(*shown_);
    frame_ = (size_t)next;
    frameAt_ = now;
    prepareNext();
  } else if (clip_.loop) {  // a one-frame loop: nothing to redraw
    frameAt_ = now;
  } else {
    finished_ = true;
    if (home_) app.homeScene()->homeClipDone(now);
    else app.machine().onClipDone(now);
  }
}

// Render frame i, show it, and start on the next one while this one is held.
void Screen::show(size_t i, uint32_t now) {
  const uint32_t t0 = micros();
  scenes::renderFrame(clip_.frames[i], *shown_);
  lastRenderUs_ = micros() - t0;
  if (lastRenderUs_ > worstRenderUs_) worstRenderUs_ = lastRenderUs_;
  present(*shown_);
  frame_ = i;
  frameAt_ = now;
  prepareNext();
}

void Screen::prepareNext() {
  prepared_ = -1;
  const int next = nextIndex();
  if (next < 0) return;
  const uint32_t t0 = micros();
  scenes::renderFrame(clip_.frames[next], *prep_);
  lastRenderUs_ = micros() - t0;
  if (lastRenderUs_ > worstRenderUs_) worstRenderUs_ = lastRenderUs_;
  prepared_ = next;
}

void Screen::printInfo(Print& out) const {
  out.printf("[screen] %s clip, frame %u/%u, contrast %d, render last %lu us worst %lu us\n", home_ ? "home (life)" : "utility",
             (unsigned)frame_ + 1, (unsigned)clip_.frames.size(), contrast_, (unsigned long)lastRenderUs_, (unsigned long)worstRenderUs_);
}

// Pack 1 byte = 8 vertical pixels (SSD1306 page layout) and send.
void Screen::present(const face::Grid& g) {
  uint8_t* buf = display_.getBuffer();
  for (int page = 0; page < face::H / 8; page++)
    for (int x = 0; x < face::W; x++) {
      uint8_t b = 0;
      for (int bit = 0; bit < 8; bit++)
        if (g.d[(page * 8 + bit) * face::W + x]) b |= 1 << bit;
      buf[page * face::W + x] = b;
    }
  display_.display();
}
