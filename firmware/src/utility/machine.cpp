#include "machine.h"
#include <stdio.h>
#include <string.h>

namespace utility {

using input::Event;

const char* soundName(Sound s) {
  static const char* const N[] = {"none", "alarm_stage1", "alarm_stage2", "alarm_stage3", "snooze", "dismiss",
                                  "alarm_set", "alarm_off", "menu_move", "menu_select", "focus_start",
                                  "focus_done", "break_done", "stopwatch_minute"};
  return (int)s < (int)Sound::Count ? N[(int)s] : "?";
}

static void hhmm(bool valid, int h, int m, char* out) {
  if (!valid) { strcpy(out, "--:--"); return; }
  out[0] = (char)('0' + h / 10); out[1] = (char)('0' + h % 10); out[2] = ':';
  out[3] = (char)('0' + m / 10); out[4] = (char)('0' + m % 10); out[5] = 0;
}

void Machine::begin(const Alarm& saved, uint32_t now) {
  alarm_ = saved;
  lastInputMs_ = now;
  goHome(now);
}

void Machine::play(Sound s) {
  if (sLen_ < 8) sounds_[(sHead_ + sLen_++) % 8] = s;
}

Sound Machine::nextSound() {
  if (!sLen_) return Sound::None;
  Sound s = sounds_[sHead_];
  sHead_ = (sHead_ + 1) % 8; sLen_--;
  return s;
}

void Machine::show(Clip c, int a, int b) {
  view_.clip = c; view_.a = a; view_.b = b;
  view_.clockMode = clockMode_;
  view_.serial++;
}

void Machine::setText(const char* t) {
  strncpy(view_.text, t, sizeof(view_.text) - 1);
  view_.text[sizeof(view_.text) - 1] = 0;
}

void Machine::timeText(char* out) const { hhmm(time_.valid, time_.hour, time_.minute, out); }

// Set alarm starts at the current time (10:18 -> 10:18; the first minute tilt snaps to 10:20 / 10:15).
// Before the first time sync: the saved alarm.
void Machine::startAlarmEdit() {
  editing_ = alarm_;
  if (!time_.valid) return;
  editing_.hour = time_.hour; editing_.minute = time_.minute;
}

int Machine::minutesUntilAlarm() const {
  if (!alarm_.enabled || !time_.valid) return -1;
  // whole minutes, i.e. the exact time left rounded up; the current minute is 0 (it rings now)
  return (alarm_.hour * 60 + alarm_.minute - (time_.hour * 60 + time_.minute) + 1440) % 1440;
}

void Machine::goHome(uint32_t now) {
  state_ = State::Home;
  show(Clip::Home);
  refreshTexts(now);
}

const char* Machine::stateName() const {
  static const char* const N[] = {"home", "time_board", "menu", "menu_swap", "set_alarm", "set_alarm_done", "ringing",
                                  "snooze", "dismiss", "focus_pick", "focus_start", "focus_work", "focus_done",
                                  "break", "stopwatch"};
  return N[(int)state_];
}

void Machine::startRinging(uint32_t now) {
  resumeFocus_ = state_ == State::FocusWork || state_ == State::FocusStart;
  resumeBreak_ = state_ == State::Break || state_ == State::FocusDone;
  resumeStopwatch_ = state_ == State::Stopwatch;
  state_ = State::Ringing;
  ringStartMs_ = now;
  snoozeUntilMs_ = 0;
  stage_ = 1;
  show(Clip::AlarmRing, 1);
  play(Sound::AlarmStage1);
}

// After the alarm: go back to a running focus / break / stopwatch, else home.
static bool before(uint32_t now, uint32_t end) { return (int32_t)(now - end) < 0; }

void Machine::go(State s, uint32_t now) {
  state_ = s;
  switch (s) {
    case State::Home: goHome(now); return;
    case State::FocusWork: show(Clip::FocusWork); break;
    case State::Break: show(Clip::Break); break;
    case State::Stopwatch: show(Clip::Stopwatch); break;
    default: break;
  }
  refreshTexts(now);
}

void Machine::onEvent(Event e, uint32_t now) {
  lastInputMs_ = now;
  handle(e, now);
  refreshTexts(now);
}

void Machine::handle(Event e, uint32_t now) {
  switch (state_) {
    case State::Ringing:
      if (e == Event::Tap) {
        snoozeUntilMs_ = now + set_.snoozeMs;
        state_ = State::Snooze; show(Clip::Snooze); play(Sound::Snooze);
      } else if (e == Event::Hold || e == Event::FlipDown) {
        snoozeUntilMs_ = 0;
        state_ = State::Dismiss; show(Clip::Dismiss); play(Sound::Dismiss);
      }
      return;

    case State::Home:
      if (e == Event::DoubleTap) { state_ = State::TimeBoard; show(Clip::TimeBoard); refreshTexts(now); }
      else if (e == Event::LongPress) { state_ = State::Menu; menu_ = 0; show(Clip::Menu, menu_); }
      return;

    case State::Menu:
      if (e == Event::TiltRight || e == Event::TiltLeft) {
        menuNext_ = (menu_ + (e == Event::TiltRight ? 1 : 4)) % 5;
        state_ = State::MenuSwap; show(Clip::MenuSwap, menu_, menuNext_); play(Sound::MenuMove);
      } else if (e == Event::Tap) {
        play(Sound::MenuSelect);
        switch (menu_) {
          case 0: clockMode_ = !clockMode_; goHome(now); break;                      // clock mode on/off
          case 1: startAlarmEdit(); field_ = 0; state_ = State::SetAlarm; show(Clip::SetAlarm, 0); break;
          case 2: focusLen_ = set_.focusMinutes[0]; state_ = State::FocusPick; show(Clip::FocusPick, focusLen_); break;
          case 3:  // Play: games come later; the stopwatch lives here for now
            stopwatchRunning_ = true; swStartMs_ = now; swPausedMs_ = 0; swLastMinute_ = 0;
            go(State::Stopwatch, now); break;
          default: goHome(now); break;                                               // back
        }
      } else if (e == Event::LongPress || e == Event::Hold) {
        goHome(now);
      }
      return;

    case State::SetAlarm:
      if (view_.clip != Clip::SetAlarm) return;  // let the tilt / confirm animation finish
      if (e == Event::TiltRight || e == Event::TiltLeft) {
        int d = e == Event::TiltRight ? 1 : -1;
        if (field_ == 0) editing_.hour = (editing_.hour + 24 + d) % 24;
        else {  // minute steps; an off-grid value snaps to the next mark in that direction
          const int st = set_.alarmMinuteStep > 0 ? set_.alarmMinuteStep : 1;
          int mi = editing_.minute;
          if (mi % st) mi = d > 0 ? (mi / st + 1) * st : (mi / st) * st;
          else mi += st * d;
          editing_.minute = (mi + 60) % 60;
        }
        show(Clip::SetAlarmTilt, field_, d);
        refreshTexts(now);
      } else if (e == Event::Tap) {
        if (field_ == 0) { field_ = 1; show(Clip::SetAlarm, 1); }
        else {
          alarm_ = editing_; alarm_.enabled = true; save_ = true;
          firedMinute_ = -1;
          state_ = State::SetAlarmDone; show(Clip::SetAlarmDone); play(Sound::AlarmSet);
        }
        refreshTexts(now);
      } else if (e == Event::Hold) {      // hold = alarm off
        alarm_.enabled = false; save_ = true; play(Sound::AlarmOff); goHome(now);
      } else if (e == Event::LongPress) { // long press = leave without changes
        goHome(now);
      }
      return;

    case State::FocusPick:
      if (view_.clip != Clip::FocusPick) return;
      if (e == Event::TiltRight || e == Event::TiltLeft) {
        int n = 0, i = 0;  // cycle through the configured lengths
        while (n < Settings::MAX_FOCUS && set_.focusMinutes[n] > 0) { if (set_.focusMinutes[n] == focusLen_) i = n; n++; }
        if (n > 0) { i = (i + (e == Event::TiltRight ? 1 : n - 1)) % n; focusLen_ = set_.focusMinutes[i]; }
        show(Clip::FocusPickTilt, focusLen_, e == Event::TiltRight ? 1 : -1);
      } else if (e == Event::FlipDown || e == Event::Tap) {
        focusEndMs_ = now + (uint32_t)focusLen_ * 60000UL;
        state_ = State::FocusStart; show(Clip::FocusStart, focusLen_); play(Sound::FocusStart);
      } else if (e == Event::LongPress || e == Event::Hold) {
        goHome(now);
      }
      return;

    case State::FocusWork:
      if (e == Event::LongPress || e == Event::Hold) { focusEndMs_ = 0; goHome(now); }  // give up
      return;

    case State::Break:
      if (e == Event::Tap || e == Event::LongPress) { breakEndMs_ = 0; goHome(now); }   // skip the break
      return;

    case State::Stopwatch:
      if (e == Event::Tap) {
        if (stopwatchRunning_) { swPausedMs_ = now - swStartMs_; stopwatchRunning_ = false; }
        else { swStartMs_ = now - swPausedMs_; stopwatchRunning_ = true; }
        refreshTexts(now);
      } else if (e == Event::LongPress || e == Event::Hold) {
        stopwatchRunning_ = false; goHome(now);
      }
      return;

    default:  // one-shot animations ignore input
      return;
  }
}

void Machine::onClipDone(uint32_t now) {
  clipDone(now);
  refreshTexts(now);
}

void Machine::clipDone(uint32_t now) {
  switch (view_.clip) {
    case Clip::TimeBoard: goHome(now); break;
    case Clip::MenuSwap: menu_ = menuNext_; state_ = State::Menu; show(Clip::Menu, menu_); break;
    case Clip::SetAlarmTilt: show(Clip::SetAlarm, field_); refreshTexts(now); break;
    case Clip::SetAlarmDone: goHome(now); break;
    case Clip::Snooze:
    case Clip::Dismiss:
      if (resumeFocus_ && focusEndMs_) go(State::FocusWork, now);
      else if (resumeBreak_ && breakEndMs_) go(State::Break, now);
      else if (resumeStopwatch_) go(State::Stopwatch, now);
      else goHome(now);
      resumeFocus_ = resumeBreak_ = resumeStopwatch_ = false;
      break;
    case Clip::FocusPickTilt: show(Clip::FocusPick, focusLen_); break;
    case Clip::FocusStart: go(State::FocusWork, now); break;
    case Clip::FocusDone: breakEndMs_ = now + set_.breakMs; go(State::Break, now); break;
    case Clip::StopwatchTap: show(Clip::Stopwatch); refreshTexts(now); break;
    default: break;
  }
}

void Machine::refreshTexts(uint32_t now) {
  char t[8];
  switch (view_.clip) {
    case Clip::Home: case Clip::TimeBoard:
      timeText(t); setText(t); break;
    case Clip::SetAlarm: case Clip::SetAlarmTilt: case Clip::SetAlarmDone: {
      const Alarm& a = state_ == State::SetAlarmDone ? alarm_ : editing_;
      hhmm(true, a.hour, a.minute, t); setText(t);
      view_.note[0] = 0;
      if (view_.clip == Clip::SetAlarmDone) {
        int left = minutesUntilAlarm();
        if (left >= 0) snprintf(view_.note, sizeof(view_.note), "+%02d:%02d", left / 60, left % 60);
      }
      break;
    }
    case Clip::FocusWork: case Clip::FocusDone: case Clip::Break: {
      uint32_t end = view_.clip == Clip::Break ? breakEndMs_ : focusEndMs_;
      uint32_t left = (end && before(now, end)) ? end - now : 0;
      int mins = (int)((left + 59999) / 60000);  // rounded up: shows 00 only when time is up
      if (mins > 99) mins = 99;
      t[0] = (char)('0' + mins / 10); t[1] = (char)('0' + mins % 10); t[2] = 0;
      setText(t); break;
    }
    case Clip::Stopwatch: case Clip::StopwatchTap: {
      uint32_t ms = stopwatchRunning_ ? now - swStartMs_ : swPausedMs_;
      uint32_t s = ms / 1000, m = s / 60;
      if (m > 99) m = 99;
      s %= 60;
      t[0] = (char)('0' + m / 10); t[1] = (char)('0' + m % 10); t[2] = ':';
      t[3] = (char)('0' + s / 10); t[4] = (char)('0' + s % 10); t[5] = 0;
      setText(t); break;
    }
    default: break;
  }
  view_.shift = (uint8_t)((now / (set_.burnInShiftMs ? set_.burnInShiftMs : 1)) % 9);
  view_.night = !time_.valid || time_.hour < 6 || time_.hour >= 18;
  view_.clockMode = clockMode_;
}

void Machine::tick(const WallTime& t, uint32_t now) {
  time_ = t;

  // ---- alarm: exact minute, once per day; snooze re-rings after 5 min
  if (state_ != State::Ringing) {
    if (snoozeUntilMs_ && !before(now, snoozeUntilMs_)) { startRinging(now); return; }
    if (alarm_.enabled && t.valid && t.hour == alarm_.hour && t.minute == alarm_.minute &&
        !(firedDay_ == t.day && firedMinute_ == t.minute)) {
      firedDay_ = t.day; firedMinute_ = t.minute;
      startRinging(now);
      return;
    }
  } else {
    uint32_t el = now - ringStartMs_;
    if (el >= set_.ringMaxMs) { goHome(now); return; }  // nobody answered: stop
    int st = el < set_.stageMs ? 1 : el < 2 * set_.stageMs ? 2 : 3;
    if (st != stage_) {
      stage_ = st;
      show(Clip::AlarmRing, st);
      play(st == 2 ? Sound::AlarmStage2 : Sound::AlarmStage3);
    }
  }

  // ---- timers
  if (state_ == State::FocusWork && focusEndMs_ && !before(now, focusEndMs_)) {
    focusEndMs_ = 0;
    state_ = State::FocusDone; show(Clip::FocusDone); play(Sound::FocusDone);
  }
  if (state_ == State::Break && breakEndMs_ && !before(now, breakEndMs_)) {
    breakEndMs_ = 0; play(Sound::BreakDone); goHome(now); return;
  }
  if (state_ == State::Stopwatch && stopwatchRunning_ && view_.clip == Clip::Stopwatch) {
    int m = (int)((now - swStartMs_) / 60000);
    if (m != swLastMinute_) { swLastMinute_ = m; show(Clip::StopwatchTap); play(Sound::StopwatchMinute); }
  }

  // ---- menus give up when left alone
  if ((state_ == State::Menu || state_ == State::SetAlarm || state_ == State::FocusPick) &&
      (int32_t)(now - lastInputMs_) > (int32_t)set_.menuIdleMs &&  // signed: an input stamped a moment after `now` is not "long ago"
      (view_.clip == Clip::Menu || view_.clip == Clip::SetAlarm || view_.clip == Clip::FocusPick)) {
    goHome(now);
    return;
  }
  refreshTexts(now);
}

}  // namespace utility
