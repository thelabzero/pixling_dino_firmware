// Sound log: prints each sound the brain asks for ("[sound] alarm_stage1"), alongside the
// buzzer. The smallest possible module: a good template for a new one.
#pragma once
#include "../app/module.h"

class SoundLog : public Module {
 public:
  const char* name() const override { return "sound"; }
  bool begin(App&) override { return true; }
  void onSound(App&, utility::Sound s) override { Serial.printf("[sound] %s \n", utility::soundName(s)); }
};
