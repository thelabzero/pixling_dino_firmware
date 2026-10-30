#include "wifi_clock.h"
#include <time.h>
#include "../app/app.h"

bool WifiClock::begin(App& app) {
  clock_.begin();  // without include/secrets.h it says so and stays unsynced
  app.setTimeSource(this);
  return true;
}

void WifiClock::loop(App&, uint32_t) { clock_.loop(); }

bool WifiClock::onKey(App&, char c) {
  if (c != 's') return false;
  clock_.syncNow();
  return true;
}

void WifiClock::printInfo(Print& out) const { clock_.printInfo(out); }

utility::WallTime WifiClock::wallTime() const {
  utility::WallTime w;
  if (!clock_.timeValid()) return w;
  time_t t = time(nullptr);
  struct tm lt;
  localtime_r(&t, &lt);
  w.valid = true;
  w.day = (uint32_t)(lt.tm_year * 400 + lt.tm_yday);  // any number that changes once per local day
  w.hour = lt.tm_hour;
  w.minute = lt.tm_min;
  w.second = lt.tm_sec;
  return w;
}
