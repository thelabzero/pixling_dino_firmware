// Boot film: the three boot-up animation candidates for the Pixling brand ("how a Pixling is born").
// Pure C++ (no Arduino calls): frames are DATA (a step table per film) and one renderer draws a step into a
// 128x64 one-byte-per-pixel buffer, so it is unit-tested on the desktop (test/test_boot_film) and previewed on
// the board by src/players/play_boot.cpp.
//
//   A  "born"   pixel -> square -> pixel person -> Dino -> signs the name "Pixling"
//   B  "dino"   pixel -> square -> Dino (no person) -> hops -> shrinks and leaps onto the i as its dot
//   C  "zoom"   name "Pixling" -> the i's dot hops -> zoom into the dot -> Dino -> zoom into its face -> the live face
//
// Rules from the brand sheet: whole 2x2-pixel cells, one colour (lit / dark), motion in whole cells, a hold
// between moments. Zooms are whole-number pixel zooms (2x, 4x, 8x): every pixel just gets bigger, nothing blurs.
// The last frames of film C are drawn by the real face rig through a hook (LiveFaceFn), so the handoff is exact.
#pragma once
#include <stdint.h>

namespace boot {

constexpr int W = 128, H = 64, CELL = 2;  // screen in pixels; one design cell = 2x2 pixels
constexpr int PIXELS = W * H;

enum class Op : uint8_t {
  Dark,    // nothing lit
  Sq,      // a bottom-centred block of a x b cells (the pixel, the growing square)
  Person,  // the 12x18 pixel person
  Dino,    // the 24x23 Dino sprite
  Word,    // the "Pixling" wordmark, left to right up to a columns, with the dot of the i
  Eyes1,   // the zoomed face's two eyes at 1x (16x16 px each): first step of the grow into the live face
  Eyes2,   // second step (22x24 px each)
  Live,    // the live face (the real rig, via the hook), a = 0 open, 1 half blink, 2 closed
};

enum FaceId : uint8_t { F_OPEN = 0, F_HALF = 1, F_CLOSED = 2, F_HAPPY = 3 };  // Dino sprite face window
enum DotMode : uint8_t { DOT_NONE = 0, DOT_OPEN = 1, DOT_CLOSED = 2, DOT_SLIT = 3 };

// One held frame. Fields are used per Op:
//   Sq    a,b = width,height in cells
//   Dino  a = FaceId, b = rows drawn from the top (0 = all), c = shrink 1/2/4 (0 = 1),
//         wm = wordmark columns drawn behind it (0 = none), ox,oy = top-left cell (may be negative: clipped)
//   Word  a = columns drawn, b = DotMode, c = how many cells the dot is raised (a hop)
//   Live  a = 0 open, 1 half, 2 closed
// Zoom (all Ops): z = 1, 2, 4 or 8; fx,fy = the pixel kept at the centre of the view.
struct Step {
  uint16_t ms;
  Op op;
  uint8_t a, b, c, wm;
  int8_t ox, oy;
  uint8_t z, fx, fy;
  const char* label;
};

struct Film {
  const char* key;   // one letter for the serial menu
  const char* name;
  const Step* steps;
  int n;
};

enum Version : uint8_t { V_BORN = 0, V_DINO = 1, V_ZOOM = 2, V_COUNT = 3 };
const Film& film(int version);
uint32_t totalMs(const Film& f);

// The live face is drawn by the caller (the face rig on the board, a stub in tests): fill `px` (W*H bytes, 0/1).
typedef void (*LiveFaceFn)(uint8_t* px, int variant);

// Draws one step. `work` and `out` are PIXELS bytes each (0 = dark, 1 = lit): the scene is drawn into `work`,
// then zoomed into `out` (equal to `work` at z = 1). `live` may be null when no Live step is drawn.
void renderStep(const Step& s, uint8_t* work, uint8_t* out, LiveFaceFn live);

// Sanity check of the built-in bitmaps (every row has the right width). Used by the tests.
bool dataOk();

}  // namespace boot
