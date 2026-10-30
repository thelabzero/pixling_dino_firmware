#include "app.h"
#include "config.h"

App::App(const utility::Settings& s) { machine_.configure(s); }

void App::add(Module& m) {
  if (n_ < MAX_MODULES) mods_[n_++] = &m;
  else Serial.printf("[app] too many modules, %s left out (raise MAX_MODULES)\n", m.name());
}

void App::begin() {
  Serial.printf("\nDino firmware %s\n", FIRMWARE_VERSION);
  now_ = millis();
  machine_.begin(utility::Alarm(), now_);  // the alarm store (if on) restores the saved alarm next
  for (int i = 0; i < n_; i++) {
    on_[i] = mods_[i]->begin(*this);
    Serial.printf("[app] %-8s %s\n", mods_[i]->name(), on_[i] ? "on" : "NOT FOUND (skipped)");
  }
  Serial.println("[app] type ? for keys");
}

void App::loop() {
  const uint32_t now = now_ = millis();

  // 1. Modules: inputs are read here and arrive through emit(); outputs draw here.
  for (int i = 0; i < n_; i++)
    if (on_[i]) mods_[i]->loop(*this, now);

  // 2. Brain: timers and the alarm follow the wall clock.
  machine_.tick(wallTime(), now);

  // 3. Brain -> modules.
  for (utility::Sound s; (s = machine_.nextSound()) != utility::Sound::None;)
    for (int i = 0; i < n_; i++)
      if (on_[i]) mods_[i]->onSound(*this, s);
  if (machine_.takeSaveRequest()) alarmChanged(machine_.alarm());

#if LOG_STATE
  const char* st = machine_.stateName();  // names are fixed strings, so comparing pointers is enough
  if (st != lastState_) {
    if (lastState_) {
      const utility::WallTime w = wallTime();
      if (w.valid) Serial.printf("[state] %s -> %s at %02d:%02d:%02d\n", lastState_, st, w.hour, w.minute, w.second);
      else Serial.printf("[state] %s -> %s (time not synced yet)\n", lastState_, st);
    }
    lastState_ = st;
  }
#endif
}

void App::emit(input::Event e) {
#if LOG_INPUT
  Serial.printf("[in] %s (%s)\n", input::eventName(e), machine_.stateName());
#endif
  for (int i = 0; i < n_; i++)
    if (on_[i]) mods_[i]->onInput(*this, e);
  machine_.onEvent(e, now_);
}

void App::emitVoice(sound::Voice v) {
  for (int i = 0; i < n_; i++)
    if (on_[i]) mods_[i]->onVoice(*this, v);
}

void App::setAlarm(const utility::Alarm& a) {
  machine_.setAlarm(a);
  alarmChanged(a);
}

void App::restoreAlarm(const utility::Alarm& a) { machine_.setAlarm(a); }

void App::alarmChanged(const utility::Alarm& a) {
  Serial.printf("[alarm] %s %02d:%02d", a.enabled ? "on" : "off", a.hour, a.minute);
  const int left = machine_.minutesUntilAlarm();
  if (a.enabled && left >= 0) Serial.printf(", rings in %dh %02dm", left / 60, left % 60);
  Serial.println();
  for (int i = 0; i < n_; i++)
    if (on_[i]) mods_[i]->onAlarmChanged(*this, a);
}

utility::WallTime App::wallTime() const { return time_ ? time_->wallTime() : utility::WallTime(); }

bool App::key(char c) {
  for (int i = 0; i < n_; i++)
    if (on_[i] && mods_[i]->onKey(*this, c)) return true;
  return false;
}

void App::printKeyHelp(Print& out) const {
  for (int i = 0; i < n_; i++)
    if (on_[i] && mods_[i]->keyHelp()) out.printf("  %-8s %s\n", mods_[i]->name(), mods_[i]->keyHelp());
}

void App::printInfo(Print& out) const {
  const utility::Alarm& a = machine_.alarm();
  out.printf("[info] Dino %s, state %s, alarm %s %02d:%02d%s, free heap %u B, stack left %u B\n", FIRMWARE_VERSION,
             machine_.stateName(), a.enabled ? "on" : "off", a.hour, a.minute, machine_.snoozed() ? " (snoozed)" : "",
             (unsigned)ESP.getFreeHeap(), (unsigned)uxTaskGetStackHighWaterMark(NULL));
  for (int i = 0; i < n_; i++) {
    if (!on_[i]) { out.printf("[%s] not found\n", mods_[i]->name()); continue; }
    mods_[i]->printInfo(out);
  }
}
