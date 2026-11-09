// Boot film: step tables for the three boot-up candidates and the renderer. See boot_film.h.
// Bitmaps come from the brand sheet ("Pixling Brand Language"): the 24x23 Dino sprite with its 12x8 face
// window ('w'), the 12x18 pixel person and the 46x12 "Pixling" wordmark. Pure C++, no Arduino calls.
#include "boot/boot_film.h"
#include <string.h>

namespace boot {

// ---------------------------------------------------------------- bitmaps (brand sheet)
static const int DINO_ROWS = 23, DINO_W = 24, WIN_X = 6, WIN_ROWS = 8, WIN_W = 12;
static const char* const DINO[DINO_ROWS] = {
    "...........##...........", "..#......#.##.#......#..", "..##.....######.....##..",
    "..###..##########..###..", "...#.##############.#...", "...##################...",
    "...####wwwwwwwwww####...", "...###wwwwwwwwwwww###...", "...###wwwwwwwwwwww###...",
    "...###wwwwwwwwwwww###...", "...###wwwwwwwwwwww###...", "...###wwwwwwwwwwww###...",
    "...###wwwwwwwwwwww###...", "...####wwwwwwwwww####...", "...##################...",
    ".#.##################.#.", "##.##################.##", "##.##################.##",
    ".#.##################.#.", "...##################...", "....################....",
    "....######....######....", "....######....######....",
};
static const int PERSON_ROWS = 18, PERSON_W = 12;
static const char* const PERSON[PERSON_ROWS] = {
    ".##########.", "############", "############", "##..####..##", "##..####..##", "##..####..##",
    ".##########.", ".....##.....", ".....##.....", "..########..", "#.########.#", "#.########.#",
    "..########..", "..##....##..", "..##....##..", "..##....##..", ".###....###.", ".###....###.",
};
static const int WORD_ROWS = 12, WORD_W = 46;
static const char* const WORD[WORD_ROWS] = {
    "######...##...........##..##..................", "#######..##...........##..##..................",
    "##...##...............##......................", "##...##..##..##...##..##..##..######....######",
    "##...##..##..##...##..##..##..#######..#######", "#######..##...##.##...##..##..##...##..##...##",
    "######...##....###....##..##..##...##..##...##", "##.......##...##.##...##..##..##...##..##...##",
    "##.......##..##...##..##..##..##...##..#######", "##.......##..##...##..##..##..##...##...######",
    "............................................##", ".......................................######.",
};
// The face window of the sprite: four faces, 8 rows of 12 cells.
static const char* const FACE[4][WIN_ROWS] = {
    {"............", "............", "..##....##..", "..##....##..", "............", "....#..#....", ".....##.....", "............"},  // open
    {"............", "............", "............", "..##....##..", "............", "............", ".....##.....", "............"},  // half
    {"............", "............", "............", ".###....###.", "............", "............", "....####....", "............"},  // closed
    {"............", "............", "...#....#...", "..#.#..#.#..", "............", "....#..#....", ".....##.....", "............"},  // happy
};

bool dataOk() {
  for (int i = 0; i < DINO_ROWS; i++) if (!DINO[i] || (int)strlen(DINO[i]) != DINO_W) return false;
  for (int i = 0; i < PERSON_ROWS; i++) if (!PERSON[i] || (int)strlen(PERSON[i]) != PERSON_W) return false;
  for (int i = 0; i < WORD_ROWS; i++) if (!WORD[i] || (int)strlen(WORD[i]) != WORD_W) return false;
  for (int f = 0; f < 4; f++) for (int i = 0; i < WIN_ROWS; i++) if (!FACE[f][i] || (int)strlen(FACE[f][i]) != WIN_W) return false;
  return true;
}

// ---------------------------------------------------------------- layout (cells, 64 x 32 on screen)
static const int CX = 32;            // screen centre
static const int GROUND = 25;        // bottom row of the pixel / person / Dino
static const int WORD_OX = 9, WORD_OY = 10;      // top-left of the wordmark
static const int DOT_X = WORD_OX + 9, DOT_Y = WORD_OY;  // the dot of the first i (2x2 cells)
static const int DOT_FOCUS_X = (DOT_X + 1) * CELL, DOT_FOCUS_Y = (DOT_Y + 1) * CELL;  // pixel centre of the dot

// ---------------------------------------------------------------- drawing into a PIXELS buffer
static inline void px(uint8_t* b, int x, int y, uint8_t v = 1) {
  if (x >= 0 && x < W && y >= 0 && y < H) b[y * W + x] = v;
}
static void fillPx(uint8_t* b, int x, int y, int w, int h, uint8_t v = 1) {
  for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) px(b, x + i, y + j, v);
}
static inline void cell(uint8_t* b, int cx, int cy) { fillPx(b, cx * CELL, cy * CELL, CELL, CELL); }

static void drawSq(uint8_t* b, int w, int h) { fillPx(b, CX * CELL - w, (GROUND - h) * CELL, w * CELL, h * CELL); }

static void drawPerson(uint8_t* b) {
  const int ox = CX - PERSON_W / 2, oy = GROUND - PERSON_ROWS;
  for (int y = 0; y < PERSON_ROWS; y++) for (int x = 0; x < PERSON_W; x++) if (PERSON[y][x] == '#') cell(b, ox + x, oy + y);
}

// The Dino sprite at 1/k size (k = 1, 2, 4), nearest neighbour, top-left cell (ox, oy). rows = rows drawn (0 = all).
static void drawDino(uint8_t* b, int ox, int oy, int face, int rows, int k) {
  if (k < 1) k = 1;
  int winRow = 0;
  for (int y = 0; y < DINO_ROWS; y++) if (strchr(DINO[y], 'w')) { winRow = y; break; }
  const int w = (DINO_W + k - 1) / k, h = (DINO_ROWS + k - 1) / k;
  const char* const* f = FACE[face & 3];
  for (int ty = 0; ty < h; ty++) {
    if (rows > 0 && ty >= rows) break;
    const int sy = ty * k;
    for (int tx = 0; tx < w; tx++) {
      const int sx = tx * k;
      const char ch = DINO[sy][sx];
      if (ch == '#') cell(b, ox + tx, oy + ty);
      else if (ch == 'w') {
        const int fy = sy - winRow, fx = sx - WIN_X;
        if (fy >= 0 && fy < WIN_ROWS && fx >= 0 && fx < WIN_W && f[fy][fx] == '#') cell(b, ox + tx, oy + ty);
      }
    }
  }
}

// The wordmark, columns 0..cols-1, without the dot of the first i; the dot (4x4 px) is drawn separately and may hop.
static void drawWord(uint8_t* b, int cols, int dotMode, int dotUp) {
  if (cols > WORD_W) cols = WORD_W;
  for (int y = 0; y < WORD_ROWS; y++)
    for (int x = 0; x < cols; x++) {
      if (WORD[y][x] != '#') continue;
      if (x >= 9 && x <= 10 && y < 2) continue;  // the dot
      cell(b, WORD_OX + x, WORD_OY + y);
    }
  if (dotMode != DOT_NONE && cols > 10) {
    const int dx = DOT_X * CELL, dy = (DOT_Y - dotUp) * CELL;
    fillPx(b, dx, dy, 4, 4);
    if (dotMode == DOT_OPEN) fillPx(b, dx + 1, dy + 1, 2, 2, 0);   // two eyes, one hole
    else if (dotMode == DOT_SLIT) fillPx(b, dx + 1, dy + 1, 2, 1, 0);
  }
}

// A slab eye with stepped corners and a cut-out pupil: the zoomed-in face growing into the live face.
static void drawEye(uint8_t* b, int x, int y, int w, int h) {
  for (int r = 0; r < h; r++) {
    const int d = r < h - 1 - r ? r : h - 1 - r;
    const int inset = d >= 3 ? 0 : (d == 0 ? 4 : d == 1 ? 2 : 1);
    fillPx(b, x + inset, y + r, w - 2 * inset, 1);
  }
  const int p = (w * 28 + 50) / 100 < 2 ? 2 : (w * 28 + 50) / 100;
  fillPx(b, x + (w - p + 1) / 2, y + (2 * h - p + 2) / 4, p, p, 0);
}

static void zoomInto(const uint8_t* work, uint8_t* out, int z, int fx, int fy) {
  if (z <= 1) { memcpy(out, work, PIXELS); return; }
  const int sw = W / z, sh = H / z;
  int sx = fx - sw / 2, sy = fy - sh / 2;
  if (sx < 0) sx = 0;
  if (sy < 0) sy = 0;
  if (sx > W - sw) sx = W - sw;
  if (sy > H - sh) sy = H - sh;
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) out[y * W + x] = work[(sy + y / z) * W + sx + x / z];
}

void renderStep(const Step& s, uint8_t* work, uint8_t* out, LiveFaceFn live) {
  memset(work, 0, PIXELS);
  switch (s.op) {
    case Op::Dark: break;
    case Op::Sq: drawSq(work, s.a, s.b); break;
    case Op::Person: drawPerson(work); break;
    case Op::Dino:
      if (s.wm) drawWord(work, s.wm, DOT_NONE, 0);
      drawDino(work, s.ox, s.oy, s.a, s.b, s.c);
      break;
    case Op::Word: drawWord(work, s.a, s.b, s.c); break;
    case Op::Eyes1: drawEye(work, 32, 20, 16, 16); drawEye(work, 80, 20, 16, 16); break;
    case Op::Eyes2: drawEye(work, 26, 18, 22, 24); drawEye(work, 78, 19, 22, 24); break;
    case Op::Live: if (live) live(work, s.a); break;
  }
  zoomInto(work, out, s.z, s.fx, s.fy);
}

// ---------------------------------------------------------------- the three films
// S(ms, op, a, b, c, wm, ox, oy, z, fx, fy, label)
#define S(ms, op, a, b, c, wm, ox, oy, z, fx, fy, label) {ms, Op::op, a, b, c, wm, ox, oy, z, fx, fy, label}
#define PLAIN(ms, op, a, b, label) S(ms, op, a, b, 0, 0, 0, 0, 1, 64, 32, label)
#define DINO_AT(ms, face, rows, label) S(ms, Dino, face, rows, 1, 0, 20, 2, 1, 64, 32, label)  // Dino at its home spot
#define WORD_AT(ms, cols, dot, label) S(ms, Word, cols, dot, 0, 0, 0, 0, 1, 64, 32, label)

// A: born. pixel, wakes, pixel person, Dino, signs the name.
static const Step BORN[] = {
    PLAIN(150, Dark, 0, 0, "dark"),
    PLAIN(300, Sq, 1, 1, "a pixel"),
    PLAIN(70, Sq, 2, 2, "grows"), PLAIN(70, Sq, 4, 4, "grows"), PLAIN(70, Sq, 6, 6, "grows"),
    PLAIN(250, Sq, 8, 8, "wakes: 8x8 square"),
    PLAIN(70, Sq, 8, 12, "stretches"), PLAIN(70, Sq, 10, 15, "stretches"),
    PLAIN(350, Person, 0, 0, "pixel person"),
    DINO_AT(60, F_OPEN, 8, "becomes Dino 1/3"), DINO_AT(60, F_OPEN, 16, "becomes Dino 2/3"), DINO_AT(60, F_OPEN, 0, "becomes Dino 3/3"),
    DINO_AT(350, F_OPEN, 0, "Dino, eyes open"),
    DINO_AT(60, F_HALF, 0, "blink"), DINO_AT(100, F_CLOSED, 0, "blink"), DINO_AT(60, F_HALF, 0, "blink"),
    DINO_AT(350, F_HAPPY, 0, "Dino, happy"),
    WORD_AT(70, 8, DOT_SLIT, "name: P"), WORD_AT(70, 23, DOT_SLIT, "name: Pi"), WORD_AT(70, 34, DOT_SLIT, "name: Pixl"),
    WORD_AT(70, 46, DOT_SLIT, "name: Pixling"),
    WORD_AT(600, 46, DOT_SLIT, "hold"), WORD_AT(100, 46, DOT_CLOSED, "dot blinks"), WORD_AT(400, 46, DOT_SLIT, "hold"),
};

// B: Dino itself, no person. Dino hops, shrinks and leaps onto the i as its dot.
static const Step DINO_FILM[] = {
    PLAIN(150, Dark, 0, 0, "dark"),
    PLAIN(250, Sq, 1, 1, "a pixel"),
    PLAIN(60, Sq, 2, 2, "grows"), PLAIN(60, Sq, 4, 4, "grows"), PLAIN(60, Sq, 6, 6, "grows"),
    PLAIN(200, Sq, 8, 8, "wakes: 8x8 square"),
    DINO_AT(60, F_OPEN, 8, "becomes Dino 1/3"), DINO_AT(60, F_OPEN, 16, "becomes Dino 2/3"), DINO_AT(60, F_OPEN, 0, "becomes Dino 3/3"),
    DINO_AT(300, F_OPEN, 0, "Dino looks around"),
    DINO_AT(60, F_HALF, 0, "blink"), DINO_AT(100, F_CLOSED, 0, "blink"), DINO_AT(60, F_HALF, 0, "blink"),
    DINO_AT(200, F_HAPPY, 0, "happy"),
    S(120, Dino, F_HAPPY, 0, 1, 0, 20, -2, 1, 64, 32, "hop 1 up"), DINO_AT(100, F_HAPPY, 0, "hop 1 down"),
    S(120, Dino, F_HAPPY, 0, 1, 0, 20, -2, 1, 64, 32, "hop 2 up"), DINO_AT(100, F_HAPPY, 0, "hop 2 down"),
    S(140, Dino, F_HAPPY, 0, 2, 0, 26, 11, 1, 64, 32, "shrinks: half size"),
    S(140, Dino, F_HAPPY, 0, 2, 46, 16, 0, 1, 64, 32, "leaps left, letters rise"),
    S(140, Dino, F_HAPPY, 0, 4, 46, DOT_X - 2, DOT_Y - 8, 1, 64, 32, "smaller, over the i"),
    WORD_AT(120, 46, DOT_OPEN, "lands: becomes the dot"),
    WORD_AT(500, 46, DOT_OPEN, "hold: name signed"), WORD_AT(100, 46, DOT_CLOSED, "dot blinks"), WORD_AT(500, 46, DOT_OPEN, "hold"),
};

// C: zoom. Name first, the dot hops, zoom into the dot, Dino, zoom into the face, the live face.
static const Step ZOOM[] = {
    PLAIN(150, Dark, 0, 0, "dark"),
    WORD_AT(70, 8, DOT_OPEN, "name: P"), WORD_AT(70, 23, DOT_OPEN, "name: Pi"), WORD_AT(70, 34, DOT_OPEN, "name: Pixl"),
    WORD_AT(70, 46, DOT_OPEN, "name: Pixling"),
    WORD_AT(300, 46, DOT_OPEN, "hold"),
    S(90, Word, 46, DOT_OPEN, 2, 0, 0, 0, 1, 64, 32, "dot hops up"), WORD_AT(90, 46, DOT_OPEN, "dot down"),
    S(90, Word, 46, DOT_OPEN, 2, 0, 0, 0, 2, DOT_FOCUS_X, DOT_FOCUS_Y - 2, "dot up, zoom 2x"),
    S(90, Word, 46, DOT_OPEN, 0, 0, 0, 0, 2, DOT_FOCUS_X, DOT_FOCUS_Y - 2, "dot down, zoom 2x"),
    S(90, Word, 46, DOT_OPEN, 2, 0, 0, 0, 4, DOT_FOCUS_X, DOT_FOCUS_Y - 2, "dot up, zoom 4x"),
    S(90, Word, 46, DOT_OPEN, 0, 0, 0, 0, 4, DOT_FOCUS_X, DOT_FOCUS_Y - 2, "dot down, zoom 4x"),
    S(260, Word, 46, DOT_OPEN, 0, 0, 0, 0, 8, DOT_FOCUS_X, DOT_FOCUS_Y, "zoom 8x: only the dot"),
    PLAIN(120, Sq, 8, 8, "dot is a square"),
    DINO_AT(60, F_OPEN, 8, "Dino appears 1/3"), DINO_AT(60, F_OPEN, 16, "Dino appears 2/3"), DINO_AT(60, F_OPEN, 0, "Dino appears 3/3"),
    DINO_AT(300, F_OPEN, 0, "full Dino"),
    DINO_AT(60, F_HALF, 0, "blink"), DINO_AT(100, F_CLOSED, 0, "blink"), DINO_AT(60, F_HALF, 0, "blink"),
    S(220, Dino, F_HAPPY, 0, 1, 0, 20, 2, 2, 64, 23, "zoom 2x to the face"),
    S(320, Dino, F_HAPPY, 0, 1, 0, 20, 2, 4, 64, 23, "zoom 4x: the face"),
    PLAIN(100, Eyes1, 0, 0, "face grows"), PLAIN(100, Eyes2, 0, 0, "face grows"),
    PLAIN(400, Live, 0, 0, "live face"),
    PLAIN(60, Live, 1, 0, "blink"), PLAIN(100, Live, 2, 0, "blink"), PLAIN(60, Live, 1, 0, "blink"),
    PLAIN(500, Live, 0, 0, "live face (handover)"),
};

static const Film FILMS[V_COUNT] = {
    {"1", "A born: pixel, person, Dino, name", BORN, (int)(sizeof BORN / sizeof BORN[0])},
    {"2", "B dino: Dino itself hops onto the i", DINO_FILM, (int)(sizeof DINO_FILM / sizeof DINO_FILM[0])},
    {"3", "C zoom: name, dot, Dino, face, live face", ZOOM, (int)(sizeof ZOOM / sizeof ZOOM[0])},
};

const Film& film(int version) { return FILMS[(version % V_COUNT + V_COUNT) % V_COUNT]; }

uint32_t totalMs(const Film& f) {
  uint32_t t = 0;
  for (int i = 0; i < f.n; i++) t += f.steps[i].ms;
  return t;
}

}  // namespace boot
