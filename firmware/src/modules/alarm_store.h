// Alarm store: keeps the alarm in flash (NVS, namespace "dino") so it survives power cuts.
// Written only when the alarm changes (confirmed, turned off, test key), never per loop:
// flash wears out with writes.
#pragma once
#include <Preferences.h>
#include "../app/module.h"

class AlarmStore : public Module {
 public:
  const char* name() const override { return "alarm"; }
  bool begin(App& app) override;
  void onAlarmChanged(App& app, const utility::Alarm& a) override;

 private:
  Preferences prefs_;
};
