// Desktop build of the dino clips: prints "<key> <ms> <8192 bits>" per frame for tools/face_check.py.
#include <stdio.h>
#include <chrono>
#include "../firmware/src/scenes/dino_clips.h"
int main() {
  std::vector<scenes::Clip> clips;
  scenes::buildDino(clips, "10:24", "SAM");
  static face::Grid g;
  double worst = 0; size_t n = 0;
  for (auto& c : clips) for (auto& f : c.frames) {
    auto t0 = std::chrono::steady_clock::now();
    scenes::renderFrame(f, g);
    double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
    if (us > worst) worst = us;
    printf("%s %u ", c.key, (unsigned)f.designMs());
    for (int i = 0; i < face::W * face::H; i++) putchar('0' + g.d[i]);
    putchar('\n');
    n++;
  }
  fprintf(stderr, "clips=%zu frames=%zu worst_render_us=%.0f\n", clips.size(), n, worst);
}
