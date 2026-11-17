// HUD: the small things drawn over the dino's face on the home screen, straight into the SSD1306 page buffer
// (so the face frames stay exactly as the design sheet has them and tools/face_check.py is not affected).
//   - battery icon, top-left corner, with the percentage under it (a bolt instead while USB / charging);
//   - the clock (menu > CLOCK), bottom-right corner, 24 h "HH:MM", 3x5 digits drawn at 2x (34 x 10 px), moved by the burn-in shift.
// Both corners are empty in every dino clip (checked on the desktop render of all home clips). Pure C++, tested.
#pragma once
#include <stdint.h>

namespace hud {

constexpr int W = 128, H = 64;
constexpr int CLOCK_X = 91, CLOCK_Y = 52, CLOCK_W = 34, CLOCK_H = 10;   // the clock's top-left (shift 4) and size

struct State {
  bool battery = false;     // draw the battery icon
  int percent = 0;          // 0..100
  bool charging = false;    // USB voltage: bolt, no percentage (the divider cannot see the cell then)
  bool clock = false;       // draw the time
  int hour = 0, minute = 0;
  uint8_t shift = 0;        // burn-in position 0..8
  bool operator==(const State& o) const {
    return battery == o.battery && percent == o.percent && charging == o.charging && clock == o.clock &&
           hour == o.hour && minute == o.minute && shift == o.shift;
  }
  bool operator!=(const State& o) const { return !(*this == o); }
  bool any() const { return battery || clock; }
};

// Bars shown for a percentage: 0..4.
int bars(int percent);

// Draw into an SSD1306 page-layout buffer (W * H / 8 bytes): pixels are only turned on.
void draw(const State& s, uint8_t* page);

}  // namespace hud
