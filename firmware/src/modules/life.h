// Life: the dino's day on the home screen (life/life.* decides, scenes/dino_clips.* draws, the Screen
// module plays). Reacts to touch, shake, pick-up and tilt, quirks (rawr) now and then, falls asleep after
// 5 min without input. Triggers voices through App::emitVoice. All numbers in config.h, section 7b.
// Keys: n = rawr now, v = play each life clip in turn, s = fall asleep now (test).
#pragma once
#include "../app/module.h"
#include "../life/life.h"

class Life : public Module, public HomeScene {
 public:
  const char* name() const override { return "life"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  void onInput(App& app, input::Event e) override;
  void onSound(App& app, utility::Sound s) override;
  bool onKey(App& app, char c) override;
  const char* keyHelp() const override { return "n = rawr now, v = each life clip in turn, s = fall asleep now (test)"; }
  void printInfo(Print& out) const override;

  // HomeScene
  uint32_t homeSerial() const override { return brain_.view().serial; }
  // Builds ONLY the current clip (RAM and the 8 KB stack allow one at a time) and slices it to the Act's frames.
  void buildHome(const utility::View& home, scenes::LiveClip& out) const override;
  void homeClipDone(uint32_t now) override { brain_.onClipDone(now); }
  bool dimmed() const override { return brain_.view().dim; }

 private:
  static const int MAX_CUES = 4;
  life::Brain brain_;
  bool onHome_ = true;
  uint32_t lastSerial_ = 0;
  // Voices of the clip now playing: time (from when the clip was built) at which each fires.
  mutable uint32_t clipAt_ = 0;
  mutable uint32_t cueAt_[MAX_CUES] = {};
  mutable sound::Voice cueVoice_[MAX_CUES] = {};
  mutable int nCues_ = 0;
  int tour_ = 0;
};
