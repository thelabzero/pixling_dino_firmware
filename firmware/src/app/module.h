// Module: one feature of the dino (a sensor, an output or a service), plugged into the App.
//
// The App owns the brain (utility::Machine). Modules never talk to each other directly:
//   - input modules call app.emit(event)          (touch, IMU, Serial keys...)
//   - output modules react to onSound / view       (screen, sound...)
//   - services keep state                          (time, alarm storage...)
// So a module can be added or removed without touching the others. Every hook has an
// empty default: override only what your module needs. Example: src/modules/sound_log.*
#pragma once
#include <Arduino.h>
#include "../input/events.h"
#include "../scenes/dino_live.h"
#include "../sound/dino_sounds.h"
#include "../utility/machine.h"

class App;

class Module {
 public:
  virtual ~Module() {}

  // Short lower-case name for logs: "[imu] ...".
  virtual const char* name() const = 0;

  // Called once at start-up, in the order the modules were added in main.cpp.
  // Return false if the hardware is missing: the module is then skipped (the dino keeps running).
  virtual bool begin(App& app) = 0;

  // Called every loop (~1 ms). Never block: no long delay(), no waiting for hardware.
  virtual void loop(App&, uint32_t /*nowMs*/) {}

  // An input happened (touch, IMU, key). Called before the brain handles it.
  virtual void onInput(App&, input::Event) {}

  // The brain asked for a sound (alarm stage, menu move...).
  virtual void onSound(App&, utility::Sound) {}

  // A module asked for a dino voice (App::emitVoice): the life brain's purr, growl, yelp...
  // Only output modules (the buzzer) need this.
  virtual void onVoice(App&, sound::Voice) {}

  // The alarm was confirmed, changed or turned off.
  virtual void onAlarmChanged(App&, const utility::Alarm&) {}

  // A key typed in the Serial monitor that the console did not use. Return true if handled.
  virtual bool onKey(App&, char) { return false; }

  // One line of help for the keys handled in onKey ("s = sync time now"), or nullptr.
  virtual const char* keyHelp() const { return nullptr; }

  // Status for the "i" key.
  virtual void printInfo(Print&) const {}
};

// A module that knows the time implements this and registers with app.setTimeSource().
class TimeSource {
 public:
  virtual ~TimeSource() {}
  virtual utility::WallTime wallTime() const = 0;  // .valid = false until the time is known
};

// A module that decides the home screen (the dino's day) implements this and registers with
// app.setHomeScene(). Without one, the screen plays the brain's plain home loop.
class HomeScene {
 public:
  virtual ~HomeScene() {}
  virtual uint32_t homeSerial() const = 0;                                    // changes = start a new clip
  virtual void buildHome(const utility::View& home, scenes::LiveClip& out) const = 0;
  virtual void homeClipDone(uint32_t nowMs) = 0;                              // a one-shot home clip finished
  virtual bool dimmed() const = 0;                                            // night brightness
};
