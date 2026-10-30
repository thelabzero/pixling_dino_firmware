// Buzzer: Dino's voice on a passive piezo (PIN_BUZZER, PWM). Plays the dino sound for each
// of the brain's sounds (sound/dino_sounds.*).
//
// Timing: the sound is updated by its own 2 ms timer (esp_timer), NOT by the main loop.
// The loop is blocked for 60-90 ms while the screen draws a frame; updating from the loop
// turned every glide into a stepped warble and swallowed short sounds (found on hardware).
//
// Alarm sounds loop until the alarm stops; a new sound replaces the one playing, except that a
// dino voice (life) never cuts a looping alarm.
// Keys: p = play the next sound, r = pitch sweep, m = mute on/off.
#pragma once
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include "../app/module.h"
#include "../sound/tone_player.h"

class Buzzer : public Module {
 public:
  const char* name() const override { return "buzzer"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  void onSound(App& app, utility::Sound s) override;
  void onVoice(App& app, sound::Voice v) override;
  bool onKey(App& app, char c) override;
  const char* keyHelp() const override { return "p = play next sound, r = pitch sweep (find the loudest), m = mute on/off"; }
  void printInfo(Print& out) const override;

 private:
  // Shared between the main loop and the timer: always touch them under lock_.
  sound::TonePlayer player_;
  volatile bool muted_ = false;
  mutable portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;

  esp_timer_handle_t timer_ = nullptr;
  uint16_t outFreq_ = 0;   // what the PWM is set to now (timer side only)
  uint32_t outDuty_ = 0;
  int preview_ = 0;
  bool sweeping_ = false;
  uint16_t sweepShown_ = 0;

  static void onTimer(void* self);
  void tick();                                   // timer: advance the sound, set the PWM
  void play(const sound::Pattern& p);            // main loop: start a sound
  void output(uint16_t freq, uint8_t vol);
};
