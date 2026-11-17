#include "face_store.h"
#include "../app/app.h"

// NVS key (keep it: changing it forgets the face chosen on every unit). Value = index into scenes::FACE_CHOICES; 0 = the classic face.
static const char* NS = "dino";
static const char* K_FACE = "face";

bool FaceStore::begin(App& app) {
  prefs_.begin(NS, true);
  const int idx = prefs_.getUChar(K_FACE, 0);
  prefs_.end();
  app.restoreFace(idx);  // an out-of-range value (older or newer firmware) falls back to 0
  Serial.printf("[face] restored default face: %s (%d)\n", scenes::faceChoice(app.machine().defaultFace()).name, app.machine().defaultFace());
  return true;
}

void FaceStore::onFaceChanged(App&, int index) {
  prefs_.begin(NS, false);
  prefs_.putUChar(K_FACE, (uint8_t)index);
  prefs_.end();
}
