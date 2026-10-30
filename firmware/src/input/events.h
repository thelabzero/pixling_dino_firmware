// Input events shared by touch, IMU and (for now) the Serial stand-in for touch.
// Pure C++: no Arduino calls, so it builds in env:native.
#pragma once
#include <stdint.h>

namespace input {

enum class Event : uint8_t {
  None,
  // touch (TTP223; until it is wired, typed in the Serial monitor - see serial_touch.h)
  Tap, DoubleTap, LongPress, Hold,
  // IMU (MPU6050)
  TiltLeft, TiltRight, FlipDown, FlipUp, PickUp, PutDown, Shake,
  Count
};

inline const char* eventName(Event e) {
  switch (e) {
    case Event::Tap: return "TAP";
    case Event::DoubleTap: return "DOUBLE_TAP";
    case Event::LongPress: return "LONG_PRESS";
    case Event::Hold: return "HOLD";
    case Event::TiltLeft: return "TILT_LEFT";
    case Event::TiltRight: return "TILT_RIGHT";
    case Event::FlipDown: return "FLIP_DOWN";
    case Event::FlipUp: return "FLIP_UP";
    case Event::PickUp: return "PICK_UP";
    case Event::PutDown: return "PUT_DOWN";
    case Event::Shake: return "SHAKE";
    default: return "NONE";
  }
}

}  // namespace input
