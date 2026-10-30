// Renders the dino's buzzer sounds (firmware/src/sound/dino_sounds.cpp) to .wav files, using the
// same player as the firmware, so you can listen before uploading. A square wave, like the
// piezo; a real piezo sounds thinner and is loudest around 2-4 kHz.
// From the repo root:
//   c++ -std=c++17 -O2 -Ifirmware/src -o /tmp/sound_preview tools/sound_preview.cpp firmware/src/sound/*.cpp
//   mkdir -p art/sounds-preview && /tmp/sound_preview art/sounds-preview
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include "sound/dino_sounds.h"

static void writeWav(const std::string& path, const std::vector<int16_t>& s, int rate) {
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) { perror(path.c_str()); return; }
  auto u32 = [&](uint32_t v) { fwrite(&v, 4, 1, f); };
  auto u16 = [&](uint16_t v) { fwrite(&v, 2, 1, f); };
  fwrite("RIFF", 1, 4, f); u32(36 + s.size() * 2); fwrite("WAVEfmt ", 1, 8, f);
  u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
  fwrite("data", 1, 4, f); u32(s.size() * 2); fwrite(s.data(), 2, s.size(), f);
  fclose(f);
}

int main(int argc, char** argv) {
  const char* dir = argc > 1 ? argv[1] : ".";
  const int RATE = 44100;
  int n = 0;
  const sound::Named* all = sound::allPatterns(n);
  for (int i = 0; i < n; i++) {
    sound::Pattern p = all[i].pattern;
    const bool loops = p.loopGapMs > 0;
    sound::TonePlayer pl;
    pl.start(p, 0);
    std::vector<int16_t> out;
    double phase = 0;
    const uint32_t maxMs = loops ? 2 * 1000 + 3 * p.loopGapMs : 10000;  // alarm sounds: a few rounds
    for (uint32_t ms = 0; ms < maxMs && (pl.playing() || ms == 0); ms++) {
      pl.update(ms);
      for (int k = 0; k < RATE / 1000; k++) {
        int16_t v = 0;
        if (pl.freq()) {
          phase += (double)pl.freq() / RATE;
          phase -= (int)phase;
          v = (int16_t)((phase < 0.5 ? 1 : -1) * 9000 * pl.volume() / 100);
        }
        out.push_back(v);
      }
      if (loops && ms > 4000) break;
    }
    std::string name = all[i].name;
    for (char& c : name) if (c == ' ' || c == '(' || c == ')') c = '_';
    char path[512];
    snprintf(path, sizeof path, "%s/%02d_%s.wav", dir, i + 1, name.c_str());
    writeWav(path, out, RATE);
    printf("%s  (%.1f s)\n", path, out.size() / (double)RATE);
  }
}
