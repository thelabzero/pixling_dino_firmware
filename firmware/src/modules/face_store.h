// Face store: keeps dino's default face (menu > Face) in flash (NVS, namespace "dino", key "face") so it survives power cuts.
// Written only when the user confirms a new default face, never per loop: flash wears out with writes.
// Without this module (FEATURE_FACE_STORE 0) the choice works until the next reset.
#pragma once
#include <Preferences.h>
#include "../app/module.h"

class FaceStore : public Module {
 public:
  const char* name() const override { return "face"; }
  bool begin(App& app) override;
  void onFaceChanged(App& app, int index) override;

 private:
  Preferences prefs_;
};
