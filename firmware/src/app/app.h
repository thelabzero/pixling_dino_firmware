// App: the dino firmware's core. Holds the brain (utility::Machine) and the modules, and
// moves information between them each loop:
//
//   modules' loop()  ->  input events  ->  brain  ->  sounds, alarm changes  ->  modules
//
// main.cpp creates the modules that are switched on in config.h and add()s them here.
#pragma once
#include <Arduino.h>
#include "module.h"
#include "../input/events.h"
#include "../utility/machine.h"

#define FIRMWARE_VERSION "0.3.0"  // 0.3 = utility (clock, alarm, focus, stopwatch) on the parts rig

class App {
 public:
  explicit App(const utility::Settings& s);

  void add(Module& m);   // before begin(); start order = order added
  void begin();          // start the brain, then every module
  void loop();           // call from Arduino loop()

  // ---- for modules
  void emit(input::Event e);                      // an input happened
  void emitVoice(sound::Voice v);                 // ask the output modules to play a dino voice
  void setAlarm(const utility::Alarm& a);         // change the alarm (test key, later the app); tells all modules
  void restoreAlarm(const utility::Alarm& a);     // set the alarm at start-up without re-saving it
  void setTimeSource(const TimeSource* t) { time_ = t; }
  void setHomeScene(HomeScene* h) { home_ = h; }
  HomeScene* homeScene() const { return home_; }
  utility::WallTime wallTime() const;
  utility::Machine& machine() { return machine_; }
  const utility::Machine& machine() const { return machine_; }
  bool key(char c);                               // offer a typed key to every module
  void printKeyHelp(Print& out) const;
  void printInfo(Print& out) const;

 private:
  static const int MAX_MODULES = 16;
  Module* mods_[MAX_MODULES] = {};
  bool on_[MAX_MODULES] = {};  // begin() succeeded
  int n_ = 0;
  utility::Machine machine_;
  const TimeSource* time_ = nullptr;
  HomeScene* home_ = nullptr;
  const char* lastState_ = nullptr;
  uint32_t now_ = 0;  // time of the current loop: every event in one loop gets the same time
  void alarmChanged(const utility::Alarm& a);
};
