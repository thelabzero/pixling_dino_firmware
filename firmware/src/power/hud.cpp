#include "hud.h"

namespace hud {
namespace {

void px(uint8_t* page, int x, int y) {
  if (x < 0 || y < 0 || x >= W || y >= H) return;
  page[(y / 8) * W + x] |= (uint8_t)(1 << (y % 8));
}

// 3x5 digits, one row per byte (bit 2 = left pixel).
const uint8_t DIGITS[10][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7}};

void digit(uint8_t* page, int x, int y, int d) {
  if (d < 0 || d > 9) return;
  for (int r = 0; r < 5; r++)
    for (int c = 0; c < 3; c++)
      if (DIGITS[d][r] & (4 >> c)) px(page, x + c, y + r);
}

int number(uint8_t* page, int x, int y, int n) {  // 1-3 digits (0..100), returns the width drawn
  if (n < 0) n = 0;
  if (n > 100) n = 100;
  char t[3];
  int len = 0;
  if (n == 100) { t[0] = 1; t[1] = 0; t[2] = 0; len = 3; }
  else if (n >= 10) { t[0] = (char)(n / 10); t[1] = (char)(n % 10); len = 2; }
  else { t[0] = (char)n; len = 1; }
  for (int i = 0; i < len; i++) digit(page, x + i * 4, y, t[i]);
  return len * 4 - 1;
}

void batteryIcon(uint8_t* page, const State& s) {
  const int x0 = 1, y0 = 1;
  for (int x = 0; x < 13; x++) { px(page, x0 + x, y0); px(page, x0 + x, y0 + 5); }   // top, bottom
  for (int y = 0; y < 6; y++) { px(page, x0, y0 + y); px(page, x0 + 12, y0 + y); }   // sides
  px(page, x0 + 13, y0 + 2); px(page, x0 + 13, y0 + 3);                              // the tip
  if (s.charging) {  // a bolt in the body
    static const int8_t B[][2] = {{7, 1}, {8, 1}, {5, 2}, {6, 2}, {7, 2}, {6, 3}, {7, 3}, {8, 3}, {5, 4}, {6, 4}};
    for (auto& p : B) px(page, x0 + p[0], y0 + p[1]);
    return;
  }
  const int n = bars(s.percent);
  for (int b = 0; b < n; b++)
    for (int dx = 0; dx < 2; dx++)
      for (int y = 1; y <= 4; y++) px(page, x0 + 1 + b * 3 + dx, y0 + y);
  number(page, x0, y0 + 8, s.percent);
}

// The clock is the 3x5 digits drawn at 2x (6x10 each): 34 x 10 px, bottom-right. Measured on the desktop render of every home clip:
// nothing below row 46 reaches past x = 81, so x >= 90 is always free (the clock sits at x 91..125 plus the 1 px shift).
void digit2(uint8_t* page, int x, int y, int d) {
  if (d < 0 || d > 9) return;
  for (int r = 0; r < 5; r++)
    for (int c = 0; c < 3; c++)
      if (DIGITS[d][r] & (4 >> c))
        for (int i = 0; i < 2; i++)
          for (int j = 0; j < 2; j++) px(page, x + c * 2 + i, y + r * 2 + j);
}

void clockText(uint8_t* page, const State& s) {
  const int dx = (int)(s.shift % 3) - 1, dy = (int)((s.shift / 3) % 3) - 1;  // 3x3 ring, 1 px
  const int x = CLOCK_X + dx, y = CLOCK_Y + dy;
  digit2(page, x, y, s.hour / 10);
  digit2(page, x + 8, y, s.hour % 10);
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) { px(page, x + 16 + i, y + 2 + j); px(page, x + 16 + i, y + 6 + j); }  // colon: two 2x2 dots
  digit2(page, x + 20, y, s.minute / 10);
  digit2(page, x + 28, y, s.minute % 10);
}

}  // namespace

int bars(int percent) {
  if (percent <= 0) return 0;
  const int n = (percent + 24) / 25;
  return n > 4 ? 4 : n;
}

void draw(const State& s, uint8_t* page) {
  if (s.battery) batteryIcon(page, s);
  if (s.clock) clockText(page, s);
}

}  // namespace hud
