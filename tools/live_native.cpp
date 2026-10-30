// Desktop smoke test of the live dino clips: builds every utility clip with sample values, renders every frame,
// prints "<clip> <frames> <loop> <ms of first frame>" and checks "time" against the design clip.
#include <stdio.h>
#include <string.h>
#include "../firmware/src/scenes/dino_live.h"
int main() {
  static face::Grid g, ref;
  int bad = 0;
  utility::View v;
  for (int c = 0; c < (int)utility::Clip::StopwatchTap + 1; c++) {
    v = utility::View();
    v.clip = (utility::Clip)c; v.a = 1; v.b = 2; v.shift = 3;
    strcpy(v.text, c == (int)utility::Clip::Stopwatch ? "01:30" : "10:24"); strcpy(v.note, "+0:42");
    scenes::LiveClip lc;
    scenes::buildLive(v, lc);
    for (auto& f : lc.frames) scenes::renderFrame(f, g);
    printf("clip %2d: %2zu frames, %s, first hold %u ms\n", c, lc.frames.size(), lc.loop ? "loop" : "once", lc.frames.empty() ? 0 : (unsigned)lc.frames[0].ms);
    if (lc.frames.empty()) { printf("  EMPTY\n"); bad++; }
  }
  // the live time board with 10:24 must equal the design's "time" clip frame for frame
  std::vector<scenes::Clip> d; scenes::buildDino(d, "10:24", "SAM", "time");
  v = utility::View(); v.clip = utility::Clip::TimeBoard; strcpy(v.text, "10:24");
  scenes::LiveClip lc; scenes::buildLive(v, lc);
  if (lc.frames.size() != d[0].frames.size()) { printf("time board: frame count differs\n"); bad++; }
  else for (size_t i = 0; i < lc.frames.size(); i++) {
    scenes::renderFrame(lc.frames[i], g); scenes::renderFrame(d[0].frames[i], ref);
    if (memcmp(g.d, ref.d, sizeof g.d)) { printf("time board frame %zu differs\n", i); bad++; }
  }
  printf(bad ? "FAIL\n" : "OK\n");
  return bad;
}
