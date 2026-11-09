#include "wifi_clock.h"
#include <WiFi.h>
#include <esp_random.h>
#include <time.h>
#include "config.h"
#include "../app/app.h"
#include "../drivers/wifi_creds.h"

bool WifiClock::begin(App& app) {
  clock_.begin();  // without saved credentials it says so and stays unsynced
  wifisetup::Timing t;
  t.waitMs = (uint32_t)WIFI_SETUP_WAIT_MIN * 60000UL;
  t.connectMs = (uint32_t)WIFI_SETUP_CONNECT_S * 1000UL;
  session_.configure(t);
  app.setTimeSource(this);
  return true;
}

void WifiClock::loop(App& app, uint32_t now) {
  clock_.loop();

  // What the brain wants: menu > Wi-Fi asks to start, leaving the screen (or an alarm) asks to cancel.
  switch (app.machine().takeWifiRequest()) {
    case utility::WifiRequest::Start: startSetup(app, now); break;
    case utility::WifiRequest::Cancel: endSetup(); break;
    default: break;
  }
  if (setup_) runSetup(app, now);
}

void WifiClock::startSetup(App& app, uint32_t now) {
  clock_.holdOff(true);  // the portal needs the radio; a sync in progress is dropped and retried afterwards
  // Radio on first: with it on, esp_random() gives true random numbers (off, it is only pseudo-random). The code
  // protects the setup network (WPA2), so it must not be guessable.
  WiFi.mode(WIFI_AP_STA);
  char name[10], code[wifisetup::CODE_LEN + 1];
  wifisetup::makeApName((uint16_t)(ESP.getEfuseMac() & 0xFFFF), name);
  wifisetup::makeCode(((uint64_t)esp_random() << 32) | esp_random(), code);
  session_.begin(now);
  if (!portal_.start(name, code, session_)) {
    session_.cancel();
    clock_.holdOff(false);
    app.machine().onWifi(utility::WifiEvent::TimedOut, now);  // nothing to pair with: back to the home screen
    return;
  }
  setup_ = true;
  closeAtMs_ = 0;
  app.machine().setWifiInfo(name, code);
}

void WifiClock::endSetup() {
  if (!setup_) return;
  portal_.stop();
  session_.cancel();
  setup_ = false;
  closeAtMs_ = 0;
  clock_.holdOff(false);  // time sync may use the radio again (and starts at once if credentials were just saved)
}

void WifiClock::runSetup(App& app, uint32_t now) {
  portal_.loop(now);
  utility::WifiEvent e;
  while (session_.take(e)) {
    app.machine().onWifi(e, now);
    if (e == utility::WifiEvent::Connected) {
      // Saved only now that it worked: one flash write per successful setup.
      if (!wificreds::save(session_.ssid(), session_.pass())) Serial.println("[wifi] could not save the Wi-Fi to flash");
      clock_.setCredentials(session_.ssid(), session_.pass());
      closeAtMs_ = now + WIFI_SETUP_LINGER_MS;  // keep the network up a moment so the phone page can say "connected"
      Serial.printf("[wifi] connected to \"%s\"; saved\n", session_.ssid());
    } else if (e == utility::WifiEvent::TimedOut) {
      Serial.println("[wifi] setup timed out");
      endSetup();
      return;
    }
  }
  if (closeAtMs_ && (int32_t)(now - closeAtMs_) >= 0) endSetup();
}

bool WifiClock::onKey(App&, char c) {
  if (c != 's') return false;
  clock_.syncNow();
  return true;
}

void WifiClock::printInfo(Print& out) const {
  clock_.printInfo(out);
  out.printf("[wifi] %s, setup %s\n", clock_.hasCredentials() ? "a network is saved" : "no network saved",
             setup_ ? session_.stateName() : "closed");
}

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

void WifiClock::soakFill(soak::Record& r) const {
  const uint32_t ok = clock_.syncsOk(), bad = clock_.syncsFailed();
  r.syncOk = (uint8_t)(ok > 255 ? 255 : ok);
  r.syncFail = (uint8_t)(bad > 255 ? 255 : bad);
  if (clock_.driftKnown()) {
    const int32_t d = clock_.lastDriftS();
    r.driftS = (int16_t)(d > 32767 ? 32767 : d < -32768 ? -32768 : d);
  }
}
