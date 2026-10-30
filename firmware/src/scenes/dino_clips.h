// Dino clips: every screen of the design sheet as frame data (groups R face rig, L life, U utility).
// A Frame is a list of draw operations plus a hold time; render it with renderFrame().
// Built once per clip on demand. Pure C++, checked by tools/face_check.py against art/reference/dino-v1/.
#pragma once
#include <stdint.h>
#include <functional>
#include <string>
#include <vector>
#include "../face/face.h"

namespace scenes {

using Draw = std::function<void(face::Grid&)>;

struct Frame {
  uint32_t ms = 0;         // hold time on the device (0 = static sheet)
  uint32_t previewMs = 0;  // hold time in the design preview (0 = same as ms)
  const char* note = "";
  int8_t shiftX = 0, shiftY = 0;  // burn-in: the whole finished frame is moved by this many pixels
  std::vector<Draw> ops;
  uint32_t designMs() const { return previewMs ? previewMs : (ms ? ms : 1000); }  // what the reference timing holds
};

struct Clip {
  char group = 'R';
  const char* key = "";
  const char* name = "";
  std::vector<Frame> frames;
};

// Builds the R, L and U clips (games and friends come later). `time` is "HH:MM", `friendName` up to 6 chars.
// `onlyKey`: build just that clip (the hardware player holds one clip in RAM at a time); null = all.
void buildDino(std::vector<Clip>& out, const std::string& time, const std::string& friendName, const char* onlyKey = nullptr);

// Clip keys in design order (must match buildDino; the player steps through these).
static const char* const DINO_CLIP_KEYS[] = {
  "rig_eyes", "rig_brows", "rig_mouths", "rig_type", "rig_icons", "expressions", "scale",
  "idle", "pet", "overpet", "rawr", "shake", "pickup", "tilt", "sleep", "wake",
  "time", "clock", "clockshift", "menu", "setalarm", "alarm", "snooze", "dismiss", "focuspick", "focus", "focusdone",
  "break", "stopwatch", "lowbatt", "charging", "settings"};
static const int DINO_CLIP_COUNT = sizeof(DINO_CLIP_KEYS) / sizeof(DINO_CLIP_KEYS[0]);
void renderFrame(const Frame& f, face::Grid& g);

}  // namespace scenes
