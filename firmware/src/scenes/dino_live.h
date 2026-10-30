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

// Utility clips (everything except Home) for the brain's current view.
void buildLive(const utility::View& v, LiveClip& out);

}  // namespace scenes
