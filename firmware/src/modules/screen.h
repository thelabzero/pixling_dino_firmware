// Screen: the SSD1306 OLED. Plays the dino clip for what the brain shows (machine.view(), built live by
// scenes/dino_live.*). On the home screen, a HomeScene module (the dino's day, modules/life.*) decides the
// clip instead, and may dim the screen at night. Tells whoever chose the clip when a one-shot clip has finished.
//
// Power (config.h section 3 and 5c, decisions in power/power_policy): awake and untouched -> dim; asleep -> night dim,
// then the panel is switched off and the CPU drops to the idle clock; touch, pick-up, tilt, shake and the alarm wake it.
// HUD (power/hud): the battery icon (after a touch, when low, or on USB) and the clock (menu > CLOCK) are drawn over the
// home screen's corners, which every dino clip leaves empty.
//
// Pipelined: while a frame is held on screen, the next one is rendered into a second buffer (a frame takes
// 8-216 ms to draw on the chip, a hold can be as short as 60 ms; found on hardware, docs/debug-play-dino.md).
#pragma once
#include <Adafruit_SSD1306.h>
#include "../app/module.h"
#include "../face/face.h"
#include "../scenes/dino_live.h"
#include "../boot/intro_player.h"
#include "../power/hud.h"
#include "../power/power_policy.h"

class Screen : public Module {
 public:
  Screen();
  const char* name() const override { return "screen"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  void printInfo(Print& out) const override;
  void soakFill(soak::Record& r) const override {
    r.renderWorstMs = (uint16_t)(worstRenderUs_ / 1000);
    if (off_) r.flags |= soak::FL_SCREEN_OFF | soak::FL_CPU_LOW;
  }
  void onInput(App& app, input::Event e) override;   // a touch skips the boot film
  bool swallowsInput() const override { return intro_.active(); }
  bool onKey(App& app, char c) override;             // B = replay the boot film
  const char* keyHelp() const override { return "B = replay the boot film"; }

 private:
  Adafruit_SSD1306 display_;
  scenes::LiveClip clip_;
  utility::View playing_;      // the view the current clip was built from
  bool home_ = false;          // current clip comes from the HomeScene
  uint32_t homeSerial_ = 0;
  size_t frame_ = 0;           // frame on screen
  int prepared_ = -1;          // index of the frame already rendered into *prep_ (-1 = none)
  uint32_t frameAt_ = 0;
  bool finished_ = false;
  int contrast_ = -1;
  power::ScreenMode mode_ = power::ScreenMode::Full;
  bool off_ = false;            // the panel is switched off
  bool wantOn_ = false;         // wake requested: the panel goes on once the right frame is in the buffer
  bool presented_ = false;      // a frame was sent during this loop
  uint32_t lastActivityMs_ = 0, asleepSinceMs_ = 0;
  bool asleepSeen_ = false;
  hud::State hud_;
  void policy(App& app, uint32_t now);
  hud::State hudNow(App& app, uint32_t now) const;
  void panel(bool on);
  face::Grid bufA_, bufB_;     // 8 KB each: the Screen must not live on the stack
  face::Grid *shown_ = &bufA_, *prep_ = &bufB_;
  uint32_t lastRenderUs_ = 0, worstRenderUs_ = 0;
  boot::IntroPlayer intro_;    // the boot film (FEATURE_BOOT_INTRO): while active it owns the screen
  void startIntro(uint32_t now);
  void drawIntro();
  void update(App& app, uint32_t now);
  void advance(App& app, uint32_t now);
  void build(App& app, const utility::View& v);
  void brightness(App& app);
  void show(size_t i, uint32_t now);   // render frame i now (blocks for one render), show it, then prepare the next
  int nextIndex() const;               // frame after the one on screen, or -1 when a one-shot clip is over
  void prepareNext();
  void present(const face::Grid& g);   // pack to the SSD1306 page layout and send
};
