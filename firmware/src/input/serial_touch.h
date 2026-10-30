// Stand-in for the TTP223 touch sensor until it is wired (plan: docs/plan-c2-c3.md).
// Type one letter in the Serial monitor:  t = tap, d = double tap, l = long press, h = hold 2 s.
// The rest of the firmware only sees input::Event, so swapping in the real sensor
// later (input/touch.*) changes nothing downstream.
#pragma once
#include "events.h"

namespace input {

// Returns Event::None for any other character.
inline Event touchFromChar(char c) {
  switch (c) {
    case 't': case 'T': return Event::Tap;
    case 'd': case 'D': return Event::DoubleTap;
    case 'l': case 'L': return Event::LongPress;
    case 'h': case 'H': return Event::Hold;
    default: return Event::None;
  }
}

}  // namespace input
