#include "wifi_time.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "config.h"
#include "wifi_creds.h"

#if __has_include("secrets.h")
#include "secrets.h"
#define DINO_HAVE_SECRETS 1   // developer fallback when nothing was saved by the setup
#else
#define DINO_HAVE_SECRETS 0
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
  haveCreds_ = wificreds::load(ssid_, sizeof ssid_, pass_, sizeof pass_);  // saved by the setup (menu > Wi-Fi)
  if (!haveCreds_ && DINO_HAVE_SECRETS && WIFI_SSID[0]) {
    strncpy(ssid_, WIFI_SSID, sizeof ssid_ - 1); strncpy(pass_, WIFI_PASS, sizeof pass_ - 1);
    haveCreds_ = true;
  }
  if (!haveCreds_) {
    state_ = State::NoCredentials;
    Serial.println("[time] no Wi-Fi saved: long press > Wi-Fi to connect your dino. Clock shows --:--");
    return;
  }
  schedule_.start(millis());
}

void WifiTime::setCredentials(const char* ssid, const char* pass) {
  strncpy(ssid_, ssid, sizeof ssid_ - 1); ssid_[sizeof ssid_ - 1] = 0;
  strncpy(pass_, pass ? pass : "", sizeof pass_ - 1); pass_[sizeof pass_ - 1] = 0;
  haveCreds_ = true;
  if (state_ == State::NoCredentials) state_ = State::Idle;
  schedule_.start(millis());  // sync now, and forget old back-off
}

void WifiTime::holdOff(bool on) {
  hold_ = on;
  if (on && (state_ == State::Connecting || state_ == State::Syncing)) {  // drop a sync in progress; it is retried after the setup
    wifiOff();
    state_ = State::Idle;
  }
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
  if (hold_) return;  // the setup portal is using the radio
  switch (state_) {
    case State::NoCredentials:
      return;
    case State::Idle:
      if (!schedule_.due(now)) return;
      Serial.printf("[time] connecting to Wi-Fi \"%s\"...\n", ssid_);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid_, pass_);
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
