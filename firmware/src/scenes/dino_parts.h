// Small building blocks for dino screens: draw ops (face, text, icon) and frames. Shared by the design clips
// (dino_clips.cpp) and the live screens (dino_live.cpp). The names follow the design sheet's helpers.
#pragma once
#include <functional>
#include <initializer_list>
#include <string>
#include "dino_clips.h"

namespace scenes {

using namespace face;

inline const face::EyeO SHUT = face::EyeO().lt(0.5).lb(0.5);
inline const face::EyeO HALF = face::EyeO().lt(0.35).lb(0.2);

// ---- draw-op builders (the design's fa / sm / tx / tc / ic helpers) ----
inline Draw fa(Ex n, const FaceO& o = FaceO()) {
  FaceP p = resolve(n, o);
  return [p](Grid& g) { faceDraw(g, p); };
}
inline Draw I(const FaceO& o = FaceO()) { return fa(Ex::Idle, o); }
// Small face: scale 0.55 at x24 y31 looking right at the content beside it.
inline Draw sm(Ex n, const FaceO& o = FaceO()) { return fa(n, FaceO().s(0.55).x(24).y(31).look(1, 0).over(o)); }
inline Draw tx(std::string s, real x, real y, Font f = Font::Txt, int sc = 1) {
  return [=](Grid& g) { text(g, s.c_str(), x, y, f, sc); };
}
inline Draw tc(std::string s, real cx, real y, Font f = Font::Txt, int sc = 1) {
  return [=](Grid& g) { textC(g, s.c_str(), cx, y, f, sc); };
}
inline Draw ic(Icon k, real x, real y) { return [=](Grid& g) { stampIcon(g, k, x, y); }; }
inline Draw fn(std::function<void(Grid&)> f) { return f; }

inline Frame fr(uint32_t ms, const char* note, std::initializer_list<Draw> ds, uint32_t previewMs = 0) {
  Frame f;
  f.ms = ms; f.previewMs = previewMs; f.note = note;
  f.ops.assign(ds.begin(), ds.end());
  return f;
}


}  // namespace scenes
