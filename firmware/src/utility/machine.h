// Utility state machine (plan step 8): what the dino shows and does for clock, time board,
// menu, set alarm, alarm ringing / snooze / dismiss, focus, break and stopwatch.
// Pure C++ (no Arduino, no drawing), tested in env:native.
//
// Inputs:  input events (touch, IMU), the wall clock (once a second is enough),
//          and onClipDone() when a one-shot animation has finished.
// Outputs: view() = which animation to show with which values; sounds and "save the
//          alarm" requests are queued and read with nextSound() / takeSaveRequest().
// Rule: alarms and timers follow the clock, never animation progress.
#pragma once
#include <stdint.h>
#include "../input/events.h"

namespace utility {

enum class Clip : uint8_t {
  Home,            // the dino idles; with clock mode on, time top-right (+ burn-in shift)
  TimeBoard,       // once: board with the time, then back
  Menu,            // loop: the menu shows card `a` (0 clock, 1 alarm, 2 focus, 3 play, 4 back)
  MenuSwap,        // once: card `a` slides away, card `b` pops up
  SetAlarm,        // loop: board HH:MM, field `a` (0 hours, 1 minutes) blinking
  SetAlarmTilt,    // once: value changed (ear flick + arrow)
  SetAlarmDone,    // once: confirmed (hop, happy, heart)
  AlarmRing,       // loop: stage `a` (1, 2, 3)
  Snooze,          // once
  Dismiss,         // once
  FocusPick,       // loop: length board, `a` = minutes (25/45/60)
  FocusPickTilt,   // once: board swings to the new length
  FocusStart,      // once: serious nod, board away
  FocusWork,       // loop: reading; text = minutes left
  FocusDone,       // once
  Break,           // loop: stretch along; text = minutes left
  Stopwatch,       // loop: text = MM:SS
  StopwatchTap,    // once: paw taps on the minute
};

enum class Sound : uint8_t {
  None, AlarmStage1, AlarmStage2, AlarmStage3, Snooze, Dismiss, AlarmSet, AlarmOff,
  MenuMove, MenuSelect, FocusStart, FocusDone, BreakDone, StopwatchMinute, Count
};
const char* soundName(Sound s);

struct View {
  Clip clip = Clip::Home;
  int a = 0, b = 0;
  char text[8] = "";        // time / countdown shown on this clip ("--:--" if unknown)
  char note[8] = "";        // SetAlarmDone: time until the alarm rings, "+HH:MM" ("" if unknown)
  bool clockMode = false;   // Home: show the time
  uint8_t shift = 0;        // burn-in position 0..8 (3 min each) for long-running screens
  bool night = true;        // alarm screens: window shows night (18:00-06:00) or day
  uint32_t serial = 0;      // changes whenever the clip must restart from its first frame
};

struct WallTime {
  bool valid = false;       // false until the clock has been synced
  uint32_t day = 0;         // any number that changes once per day (e.g. days since 1970)
  int hour = 0, minute = 0, second = 0;
};

struct Alarm {
  bool enabled = false;
  int hour = 7, minute = 30;
};

// Timings and choices. The defaults below are the tested values; the firmware fills
// them from include/config.h (main.cpp), the desktop tests use the defaults.
struct Settings {
  uint32_t stageMs = 20000;             // each alarm stage before escalating (1 -> 2 -> 3)
  uint32_t ringMaxMs = 10UL * 60000;    // nobody answers: stop ringing after this
  uint32_t snoozeMs = 5UL * 60000;      // tap while ringing: ring again after this
  uint32_t breakMs = 5UL * 60000;       // break after a finished focus
  uint32_t menuIdleMs = 10000;          // menu / set alarm / focus pick give up after this
  uint32_t burnInShiftMs = 3UL * 60000; // clock mode moves 1 px every this long (9 positions)
  int alarmMinuteStep = 5;              // set alarm: one minute tilt = this many minutes
  static const int MAX_FOCUS = 4;
  int focusMinutes[MAX_FOCUS] = {25, 45, 60, 0};  // focus lengths offered (0 = unused slot)
};

class Machine {
 public:
  void configure(const Settings& s) { set_ = s; }
  const Settings& settings() const { return set_; }
  void begin(const Alarm& saved, uint32_t nowMs);
  void onEvent(input::Event e, uint32_t nowMs);
  void tick(const WallTime& t, uint32_t nowMs);  // call often (every loop is fine)
  void onClipDone(uint32_t nowMs);               // a one-shot clip finished playing
  void setAlarm(const Alarm& a) { alarm_ = a; firedMinute_ = -1; snoozeUntilMs_ = 0; }  // e.g. a test or the app

  const View& view() const { return view_; }
  const Alarm& alarm() const { return alarm_; }
  int minutesUntilAlarm() const;  // 0..1439 (rounded up), or -1 if the alarm is off or the time unknown
  bool snoozed() const { return snoozeUntilMs_ != 0; }
  Sound nextSound();
  bool takeSaveRequest() { bool r = save_; save_ = false; return r; }
  const char* stateName() const;

 private:
  enum class State : uint8_t { Home, TimeBoard, Menu, MenuSwap, SetAlarm, SetAlarmDone, Ringing, Snooze, Dismiss,
                               FocusPick, FocusStart, FocusWork, FocusDone, Break, Stopwatch };
  void handle(input::Event e, uint32_t now);
  void clipDone(uint32_t now);
  void show(Clip c, int a = 0, int b = 0);
  void setText(const char* t);
  void go(State s, uint32_t now);
  void goHome(uint32_t now);
  void startRinging(uint32_t now);
  void play(Sound s);
  void refreshTexts(uint32_t now);
  void timeText(char* out) const;
  void startAlarmEdit();

  Settings set_;
  State state_ = State::Home;
  View view_;
  Alarm alarm_;
  WallTime time_;
  bool clockMode_ = false;
  uint32_t lastInputMs_ = 0;
  // menu
  int menu_ = 0, menuNext_ = 0;
  // set alarm
  int field_ = 0;
  Alarm editing_;
  // ringing
  uint32_t ringStartMs_ = 0, snoozeUntilMs_ = 0;
  int stage_ = 1;
  uint32_t firedDay_ = 0xFFFFFFFF;
  int firedMinute_ = -1;
  bool resumeFocus_ = false, resumeBreak_ = false, resumeStopwatch_ = false;
  // focus / break / stopwatch (all timed by millis, independent of animation)
  int focusLen_ = 25;
  uint32_t focusEndMs_ = 0, breakEndMs_ = 0;
  bool stopwatchRunning_ = false;
  uint32_t swStartMs_ = 0, swPausedMs_ = 0;
  int swLastMinute_ = 0;
  // outputs
  Sound sounds_[8];
  uint8_t sHead_ = 0, sLen_ = 0;
  bool save_ = false;
};

}  // namespace utility
