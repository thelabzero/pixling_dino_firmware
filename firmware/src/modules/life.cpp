#include "life.h"
#include <esp_random.h>
#include <vector>
#include "config.h"
#include "../app/app.h"
#include "../scenes/dino_clips.h"

bool Life::begin(App& app) {
  life::Settings s;
  s.sleepAfterMs = (uint32_t)LIFE_SLEEP_AFTER_S * 1000UL;
  s.rawrMinMs = (uint32_t)LIFE_RAWR_MIN_S * 1000UL;
  s.rawrMaxMs = (uint32_t)LIFE_RAWR_MAX_S * 1000UL;
  s.overpetTouches = LIFE_OVERPET_TOUCHES;
  s.overpetWindowMs = (uint32_t)LIFE_OVERPET_WINDOW_S * 1000UL;
  s.overpetLockoutMs = (uint32_t)LIFE_OVERPET_LOCKOUT_S * 1000UL;
  s.reactMinMs = LIFE_REACT_MIN_MS;
  s.reactMaxMs = LIFE_REACT_MAX_MS;
  s.tiltCooldownMs = (uint32_t)LIFE_TILT_COOLDOWN_S * 1000UL;
  brain_.configure(s);
  brain_.seed(esp_random());  // a different day every boot
  app.setHomeScene(this);
  return true;
}

void Life::loop(App& app, uint32_t now) {
  // Alarms and timers win: away from the home screen the dino just waits (and resumes from idle).
  const bool home = app.machine().view().clip == utility::Clip::Home;
  if (home && !onHome_) { brain_.onHomeResumed(now); lastSerial_ = 0; }
  onHome_ = home;
  if (!home) { nCues_ = 0; return; }
  brain_.tick(now);
  const life::View& v = brain_.view();
  if (v.serial != lastSerial_) {
    lastSerial_ = v.serial;
    nCues_ = 0;  // buildHome() sets the new clip's voices when the screen builds it
#if LOG_LIFE
    Serial.printf("[life] %s%s\n", life::actName(v.act), v.dim ? " (dim)" : "");
#endif
  }
  for (int i = 0; i < nCues_; i++)
    if (cueVoice_[i] != sound::Voice::None && (int32_t)(now - cueAt_[i]) >= 0) {
      app.emitVoice(cueVoice_[i]);
      cueVoice_[i] = sound::Voice::None;
    }
}

// Any input counts as activity. Touch, shake, pick-up and tilt on the home screen are the dino's to answer
// (the utility brain ignores them there); other inputs just wake it / keep it awake.
void Life::onInput(App& app, input::Event e) {
  const uint32_t now = millis();
  const bool home = app.machine().view().clip == utility::Clip::Home;
  if (!home) { brain_.onActivity(now); return; }
  switch (e) {
    case input::Event::Tap: brain_.onTouch(now); break;
    case input::Event::Shake: brain_.onShake(now); break;
    case input::Event::PickUp: brain_.onPickUp(now); break;
    case input::Event::TiltLeft:
    case input::Event::TiltRight: brain_.onTilt(now); break;
    default: brain_.onActivity(now); break;
  }
}

void Life::onSound(App&, utility::Sound s) {
  if (s == utility::Sound::Dismiss) brain_.onAlarmDismissed(millis());
}

void Life::buildHome(const utility::View&, scenes::LiveClip& out) const {
  const life::View& v = brain_.view();
  const life::ActDef& d = life::actDef(v.act);
  std::vector<scenes::Clip> built;
  scenes::buildDino(built, "00:00", "", d.clip);  // one clip only
  out.frames.clear();
  out.loop = v.loop;
  if (built.empty()) return;
  std::vector<scenes::Frame>& f = built[0].frames;
  const size_t last = d.last < f.size() ? d.last : f.size() - 1;
  // Voices: when each cue frame starts, counted from when this clip was built.
  clipAt_ = millis();
  nCues_ = 0;
  int n = 0;
  const life::Cue* c = life::cues(n);
  for (int i = 0; i < n && nCues_ < MAX_CUES; i++) {
    if (c[i].act != v.act) continue;
    uint32_t ms = 0;
    for (size_t k = d.first; k < c[i].frame && k < f.size(); k++) ms += f[k].ms;
    cueAt_[nCues_] = clipAt_ + ms;
    cueVoice_[nCues_++] = c[i].voice;
  }
  for (size_t k = d.first; k <= last; k++) out.frames.push_back(std::move(f[k]));
}

bool Life::onKey(App&, char c) {
  if (c == 'n') { brain_.nextNow(millis()); Serial.println("[life] rawr now"); return true; }
  if (c == 's') { brain_.playNow(life::Act::FallAsleep, millis()); Serial.println("[life] falling asleep"); return true; }
  if (c == 'v') {  // every life clip in turn, to see them all
    static const life::Act TOUR[] = {life::Act::Glance, life::Act::Pet, life::Act::Overpet, life::Act::Rawr,
                                     life::Act::Shake, life::Act::PickUp, life::Act::Tilt, life::Act::FallAsleep};
    const int n = sizeof(TOUR) / sizeof(TOUR[0]);
    if (brain_.asleep() || brain_.busy()) { Serial.println("[life] busy or asleep: try again in a moment"); return true; }
    const life::Act a = TOUR[tour_ % n];
    Serial.printf("[life] showing %d/%d: %s\n", tour_ % n + 1, n, life::actName(a));
    tour_++;
    brain_.playNow(a, millis());
    return true;
  }
  return false;
}

void Life::printInfo(Print& out) const {
  const life::View& v = brain_.view();
  const uint32_t now = millis();
  out.printf("[life] %s%s, rawr in %lu s, sleep in %lu s\n", life::actName(v.act), v.dim ? " (dim)" : "",
             (unsigned long)(brain_.msToRawr(now) / 1000), (unsigned long)(brain_.msToSleep(now) / 1000));
}
