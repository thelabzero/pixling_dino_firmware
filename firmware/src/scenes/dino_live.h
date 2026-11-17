// Live dino clips: the design's Utility screens built with the current values (time, alarm field,
// countdown...), for the Screen module. One LiveClip = a short list of dino frames (scenes::Frame:
// draw ops + hold time) that either loops or plays once. Pure C++, desktop-checked.
// The home screen (dino's life) is built by the life module with the same type (HomeScene::buildHome).
#pragma once
#include <vector>
#include "dino_clips.h"
#include "../utility/machine.h"

namespace scenes {

struct LiveClip {
  std::vector<Frame> frames;
  bool loop = true;
};

// The faces on offer as dino's default face (menu > Face), in the order the picker shows them. Index 0 is the classic idle face.
// utility::Machine::FACE_CHOICES must equal the length (checked in dino_live.cpp). Chosen to be calm resting faces: none of them
// carries a looping effect (steam, tears, sparkles, glasses) and they all still blink.
struct FaceChoice { face::Ex ex; const char* name; };
const FaceChoice& faceChoice(int index);          // out of range -> index 0
void applyDefaultFace(int index);                 // tell the face rig: Ex::Idle now draws this choice

// The two touch-only reactions on the home screen: three taps = cute (angry = false), four taps = angry. Frames only; the life
// module plays them like any home clip. (Not in the design sheet, so tools/face_check.py does not compare them.)
void buildMood(bool angry, std::vector<Frame>& out);

// Utility clips (everything except Home) for the brain's current view.
void buildLive(const utility::View& v, LiveClip& out);

}  // namespace scenes
