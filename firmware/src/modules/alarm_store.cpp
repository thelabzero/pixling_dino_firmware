#include "alarm_store.h"
#include "../app/app.h"

// NVS keys (keep them: changing a key forgets the alarm saved on every unit).
static const char* NS = "dino";
static const char* K_ON = "al_on";
static const char* K_H = "al_h";
static const char* K_M = "al_m";

bool AlarmStore::begin(App& app) {
  utility::Alarm a;  // defaults (off, 07:30) when nothing is saved yet
  prefs_.begin(NS, true);
  a.enabled = prefs_.getBool(K_ON, a.enabled);
  a.hour = prefs_.getUChar(K_H, (uint8_t)a.hour);
  a.minute = prefs_.getUChar(K_M, (uint8_t)a.minute);
  prefs_.end();
  app.restoreAlarm(a);
  Serial.printf("[alarm] restored: %s %02d:%02d\n", a.enabled ? "on" : "off", a.hour, a.minute);
  return true;
}

void AlarmStore::onAlarmChanged(App&, const utility::Alarm& a) {
  prefs_.begin(NS, false);
  prefs_.putBool(K_ON, a.enabled);
  prefs_.putUChar(K_H, (uint8_t)a.hour);
  prefs_.putUChar(K_M, (uint8_t)a.minute);
  prefs_.end();
}
