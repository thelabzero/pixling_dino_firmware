#include "wifi_time.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "config.h"

#if __has_include("secrets.h")
#include "secrets.h"
#define DINO_HAVE_WIFI 1
#else
#define DINO_HAVE_WIFI 0  // copy include/secrets.example.h to include/secrets.h
#define WIFI_SSID ""
#define WIFI_PASS ""
#endif

static volatile bool gSynced = false;
static volatile int64_t gSyncedEpoch = 0;
static void onTimeSynced(struct timeval* tv) {
  gSyncedEpoch = tv ? (int64_t)tv->tv_sec : 0;
  gSynced = true;
}

static void wifiOff() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void WifiTime::begin() {
  setenv("TZ", TIME_TZ, 1);  // local time works even before the first sync
  tzset();
  sntp_set_time_sync_notification_cb(onTimeSynced);
  wifiOff();
  if (!DINO_HAVE_WIFI) {
    state_ = State::NoCredentials;
    Serial.println("[time] no include/secrets.h: copy secrets.example.h and add your Wi-Fi. Clock shows --:--");
    return;
  }
  schedule_.start(millis());
}

void WifiTime::syncNow() {
  if (state_ == State::Idle) schedule_.syncNow(millis());
}

bool WifiTime::timeValid() const { return syncs_ > 0 && timekeeping::epochLooksValid((int64_t)time(nullptr)); }

bool WifiTime::localTime(int& h, int& m, int& s) const {
  if (!timeValid()) return false;
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  h = t.tm_hour; m = t.tm_min; s = t.tm_sec;
  return true;
}

void WifiTime::finish(bool ok, const char* why) {
  wifiOff();
  uint32_t now = millis();
  if (ok) {
    syncs_++;
    lastSyncMs_ = now;
    schedule_.onSuccess(now);
    if (timekeeping::epochLooksValid(sysBefore_)) {  // we had a time before: how far had it drifted?
      int64_t expected = sysBefore_ + (int64_t)((now - msBefore_) / 1000);
      lastDriftS_ = (int32_t)(gSyncedEpoch - expected);
      driftKnown_ = true;
    }
    Serial.printf("[time] synced%s in %lu ms; Wi-Fi off\n",
                  driftKnown_ ? "" : " (first time)", (unsigned long)(now - msBefore_));
    if (driftKnown_) Serial.printf("[time] clock was off by %ld s since the last sync\n", (long)lastDriftS_);
  } else {
    fails_++;
    schedule_.onFailure(now);
    Serial.printf("[time] sync failed (%s); Wi-Fi off; retry in %lu s\n", why,
                  (unsigned long)(schedule_.msUntilNext(now) / 1000));
  }
  state_ = State::Idle;
}

void WifiTime::loop() {
  const uint32_t now = millis();
  switch (state_) {
    case State::NoCredentials:
      return;
    case State::Idle:
      if (!schedule_.due(now)) return;
      Serial.printf("[time] connecting to Wi-Fi \"%s\"...\n", WIFI_SSID);
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASS);
#if WIFI_LOW_TX_POWER
      WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
      stepStart_ = now;
      state_ = State::Connecting;
      return;
    case State::Connecting:
      if (WiFi.status() == WL_CONNECTED) {
        sysBefore_ = (int64_t)time(nullptr);
        msBefore_ = now;
        gSynced = false;
        configTzTime(TIME_TZ, NTP_SERVER_1, NTP_SERVER_2);  // starts SNTP; callback sets gSynced
        stepStart_ = now;
        state_ = State::Syncing;
      } else if (now - stepStart_ > WIFI_CONNECT_TIMEOUT_MS) {
        finish(false, "Wi-Fi did not connect: check name/password, 2.4 GHz network, signal");
      }
      return;
    case State::Syncing:
      if (gSynced) finish(true, "");
      else if (now - stepStart_ > NTP_TIMEOUT_MS) finish(false, "no answer from the time server");
      return;
  }
}

void WifiTime::printInfo(Print& out) const {
  const uint32_t now = millis();
  int h, m, s;
  if (localTime(h, m, s)) out.printf("[time] now %02d:%02d:%02d (%s)\n", h, m, s, TIME_TZ);
  else out.println("[time] time not known yet");
  out.printf("[time] syncs ok %lu, failed %lu", (unsigned long)syncs_, (unsigned long)fails_);
  if (syncs_) out.printf(", last %lu min ago", (unsigned long)((now - lastSyncMs_) / 60000));
  if (driftKnown_) out.printf(", last drift %ld s", (long)lastDriftS_);
  if (state_ == State::Idle) out.printf(", next in %lu min", (unsigned long)(schedule_.msUntilNext(now) / 60000));
  out.println();
}
