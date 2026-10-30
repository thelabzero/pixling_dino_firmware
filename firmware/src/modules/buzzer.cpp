#include "buzzer.h"
#include "config.h"
#include "../app/app.h"
#include "../sound/dino_sounds.h"

// 10-bit PWM: the C3's LEDC then reaches down to ~80 Hz; at 8 bits it stops at ~300 Hz.
// 50 % duty (512) is the loudest a piezo gets.
static const uint8_t RES_BITS = 10;
static const uint32_t DUTY_MAX = 512;
static const uint32_t TICK_US = 2000;  // sound update every 2 ms

bool Buzzer::begin(App&) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  if (!ledcAttach(PIN_BUZZER, 2000, RES_BITS)) return false;
#else
  ledcSetup(0, 2000, RES_BITS);
  ledcAttachPin(PIN_BUZZER, 0);
#endif
  output(0, 0);
  muted_ = BUZZER_VOLUME == 0;

  esp_timer_create_args_t args = {};
  args.callback = &Buzzer::onTimer;
  args.arg = this;
  args.dispatch_method = ESP_TIMER_TASK;  // runs in the high-priority esp_timer task, not an interrupt
  args.name = "buzzer";
  if (esp_timer_create(&args, &timer_) != ESP_OK || esp_timer_start_periodic(timer_, TICK_US) != ESP_OK) {
    Serial.println("[buzzer] timer failed");
    return false;
  }
  Serial.printf("[buzzer] GPIO%d, volume %d %%\n", PIN_BUZZER, BUZZER_VOLUME);
  return true;
}

void Buzzer::onTimer(void* self) { static_cast<Buzzer*>(self)->tick(); }

void Buzzer::tick() {
  portENTER_CRITICAL(&lock_);
  player_.update(millis());
  const uint16_t f = player_.freq();
  const uint8_t v = muted_ ? 0 : player_.volume();
  portEXIT_CRITICAL(&lock_);
  output(f, v);  // LEDC calls outside the lock (they take their own locks)
}

void Buzzer::play(const sound::Pattern& p) {
  portENTER_CRITICAL(&lock_);
  player_.start(p, millis());
  portEXIT_CRITICAL(&lock_);
}

void Buzzer::onSound(App&, utility::Sound s) {
  const sound::Pattern p = sound::patternFor(s);
  if (p.n) { play(p); sweeping_ = false; }
}

void Buzzer::onVoice(App&, sound::Voice v) {
  const sound::Pattern p = sound::patternFor(v);
  if (!p.n) return;
  portENTER_CRITICAL(&lock_);
  const bool alarm = player_.looping();
  if (!alarm) player_.start(p, millis());
  portEXIT_CRITICAL(&lock_);
  if (!alarm) sweeping_ = false;
}

void Buzzer::loop(App& app, uint32_t) {
  // Alarm sounds loop; stop them as soon as the alarm is no longer ringing (snooze, dismiss, auto-stop).
  if (app.machine().view().clip != utility::Clip::AlarmRing) {
    portENTER_CRITICAL(&lock_);
    if (player_.looping()) player_.stop();
    portEXIT_CRITICAL(&lock_);
  }
  if (sweeping_) {  // name each pitch as it starts, so the loudest one can be noted
    portENTER_CRITICAL(&lock_);
    const bool playing = player_.playing();
    const uint16_t f = player_.freq();
    portEXIT_CRITICAL(&lock_);
    if (!playing) { sweeping_ = false; Serial.println("[buzzer] sweep done: which pitch was loudest?"); }
    else if (f && f != sweepShown_) { sweepShown_ = f; Serial.printf("[buzzer] sweep %u Hz\n", f); }
  }
}

// Loudness = PWM duty. A piezo gets quiet fast below ~25 % of full duty, so volume maps to
// 25-100 % of the loudest duty instead of 0-100 %.
static uint32_t dutyFor(uint8_t vol) {
  const uint32_t v = (uint32_t)vol * BUZZER_VOLUME / 100;  // 0-100
  return v ? DUTY_MAX * (25 + 75 * v / 100) / 100 : 0;
}

// Change the PWM only when the sound changes. During a glide, skip changes under ~0.5 %
// (inaudible): every frequency change restarts the PWM timer.
void Buzzer::output(uint16_t freq, uint8_t vol) {
  uint32_t duty = freq ? dutyFor(vol) : 0;
  if (duty == 0) freq = 0;
  const bool sameFreq = freq == outFreq_ || (freq && outFreq_ && (uint32_t)abs((int)freq - (int)outFreq_) * 200 < outFreq_);
  if (sameFreq && duty == outDuty_) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  if (freq && !sameFreq) ledcChangeFrequency(PIN_BUZZER, freq, RES_BITS);
  if (duty != outDuty_ || !sameFreq) ledcWrite(PIN_BUZZER, duty);
#else
  if (freq && !sameFreq) ledcChangeFrequency(0, freq, RES_BITS);
  if (duty != outDuty_ || !sameFreq) ledcWrite(0, duty);
#endif
  if (!sameFreq) outFreq_ = freq;
  outDuty_ = duty;
}

bool Buzzer::onKey(App&, char c) {
  if (c == 'm') {
    muted_ = !muted_;
    Serial.printf("[buzzer] %s\n", muted_ ? "muted" : "sound on");
    return true;
  }
  if (c == 'r') {
    play(sound::sweepPattern());
    sweeping_ = true; sweepShown_ = 0;
    Serial.println("[buzzer] sweep 500 Hz - 5 kHz, 0.4 s each");
    return true;
  }
  if (c == 'p') {
    int n = 0;
    const sound::Named* all = sound::allPatterns(n);
    const sound::Named& s = all[preview_ % n];
    preview_++;
    sound::Pattern p = s.pattern;
    p.loopGapMs = 0;  // play alarm sounds once when auditioning
    play(p);
    sweeping_ = false;
    Serial.printf("[buzzer] playing %d/%d: %s\n", (preview_ - 1) % n + 1, n, s.name);
    return true;
  }
  return false;
}

void Buzzer::printInfo(Print& out) const {
  portENTER_CRITICAL(&lock_);
  const bool playing = player_.playing();
  portEXIT_CRITICAL(&lock_);
  out.printf("[buzzer] GPIO%d, %s, volume %d %%, %s\n", PIN_BUZZER, muted_ ? "muted" : "on", BUZZER_VOLUME,
             playing ? "playing" : "quiet");
}
