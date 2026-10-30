#include "serial_console.h"
#include "../app/app.h"
#include "../input/serial_touch.h"

using input::Event;

void SerialConsole::loop(App& app, uint32_t) {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == '\n' || c == '\r' || c == ' ') continue;

    Event e = input::touchFromChar(c);  // t d l h
    switch (c) {                         // IMU stand-ins
      case ',': e = Event::TiltLeft; break;
      case '.': e = Event::TiltRight; break;
      case 'f': e = Event::FlipDown; break;
      case 'u': e = Event::FlipUp; break;
      default: break;
    }
    if (e != Event::None) { app.emit(e); continue; }

    if (c == 'a') testAlarm(app);
    else if (c == 'i') app.printInfo(Serial);
    else if (c == '?') { Serial.println("[keys]"); app.printKeyHelp(Serial); }
    else if (!app.key(c)) Serial.printf("[console] unknown key '%c' (? = list)\n", c);
  }
}

// Alarm at the start of the next minute: the quickest way to watch it ring.
void SerialConsole::testAlarm(App& app) {
  const utility::WallTime w = app.wallTime();
  if (!w.valid) { Serial.println("[alarm] time not known yet (Wi-Fi sync)"); return; }
  utility::Alarm a;
  a.enabled = true;
  a.hour = (w.hour + (w.minute == 59 ? 1 : 0)) % 24;
  a.minute = (w.minute + 1) % 60;
  app.setAlarm(a);
}
