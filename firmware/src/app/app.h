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

// Name and version: the dino_wedge_c3 build (wedge-eye face, same logic) sets its own in platformio.ini.
#ifndef FIRMWARE_NAME
#define FIRMWARE_NAME "Dino"
#endif
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.4.0"  // 0.3 = utility on the parts rig; 0.3.1 = soak log; 0.4.0 = power saving, battery icon, clock mode
#endif

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
  void restoreFace(int index);                    // set dino's default face at start-up without re-saving it
  void setTimeSource(const TimeSource* t) { time_ = t; }
  void setHomeScene(HomeScene* h) { home_ = h; }
  HomeScene* homeScene() const { return home_; }
  void setBatteryInfo(BatteryInfo* b) { battery_ = b; }
  BatteryInfo* batteryInfo() const { return battery_; }
  void prepareShutdown();                         // tell every module the battery is empty (they save what they must)
  utility::WallTime wallTime() const;
  utility::Machine& machine() { return machine_; }
  const utility::Machine& machine() const { return machine_; }
  bool key(char c);                               // offer a typed key to every module
  void printKeyHelp(Print& out) const;
  void printInfo(Print& out) const;

  // ---- for the soak log
  void soakFill(soak::Record& r) const;           // every module adds what it knows; the App adds state and the clock
  uint32_t takeLoopWorstMs();                     // longest gap between two loops since the last call; resets it
  uint32_t lastLoopGapMs() const { return lastGapMs_; }

 private:
  static const int MAX_MODULES = 16;
  Module* mods_[MAX_MODULES] = {};
  bool on_[MAX_MODULES] = {};  // begin() succeeded
  int n_ = 0;
  utility::Machine machine_;
  const TimeSource* time_ = nullptr;
  HomeScene* home_ = nullptr;
  BatteryInfo* battery_ = nullptr;
  const char* lastState_ = nullptr;
  uint32_t lastLoopStartMs_ = 0, lastGapMs_ = 0, loopWorstMs_ = 0;
  uint32_t now_ = 0;  // time of the current loop: every event in one loop gets the same time
  void alarmChanged(const utility::Alarm& a);
  void faceChanged(int index);
};
