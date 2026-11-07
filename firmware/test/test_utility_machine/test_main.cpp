// Run: cd firmware && pio test -e native
#include <unity.h>
#include <string.h>
#include "utility/machine.h"

using namespace utility;
using input::Event;

static Machine m;
static uint32_t T;
static WallTime W;

static void at(int h, int mi, int s = 0) { W.valid = true; W.day = 20000; W.hour = h; W.minute = mi; W.second = s; }
static void run(uint32_t ms) { for (uint32_t e = 0; e < ms; e += 100, T += 100) m.tick(W, T); }
static void ev(Event e) { m.onEvent(e, T); }
static void done() { m.onClipDone(T); }
static Clip clip() { return m.view().clip; }
static int countSound(Sound s) { int n = 0; for (Sound x; (x = m.nextSound()) != Sound::None;) if (x == s) n++; return n; }
static void drain() { while (m.nextSound() != Sound::None) {} }

void setUp() { m = Machine(); T = 1000; W = WallTime(); Alarm a; m.begin(a, T); }
void tearDown() {}

void test_home_shows_dashes_until_synced() {
  run(200);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "--:--") == 0);
  at(9, 5); run(200);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "09:05") == 0);
}

void test_double_tap_time_board_returns_home() {
  at(10, 24); run(100);
  ev(Event::DoubleTap);
  TEST_ASSERT_TRUE(clip() == Clip::TimeBoard);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "10:24") == 0);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_menu_navigation_and_back() {
  ev(Event::LongPress);
  TEST_ASSERT_TRUE(clip() == Clip::Menu); TEST_ASSERT_EQUAL(0, m.view().a);
  ev(Event::TiltRight);
  TEST_ASSERT_TRUE(clip() == Clip::MenuSwap); TEST_ASSERT_EQUAL(0, m.view().a); TEST_ASSERT_EQUAL(1, m.view().b);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::Menu); TEST_ASSERT_EQUAL(1, m.view().a);
  ev(Event::TiltLeft); done(); ev(Event::TiltLeft); done();   // 1 -> 0 -> 5 (wraps to Back)
  TEST_ASSERT_EQUAL(5, m.view().a);
  ev(Event::Tap);                                              // back
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_menu_times_out() {
  ev(Event::LongPress);
  run(Settings().menuIdleMs + 500);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_clock_mode_toggle() {
  ev(Event::LongPress); ev(Event::Tap);   // menu item 0 = clock
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  TEST_ASSERT_TRUE(m.view().clockMode);
  ev(Event::LongPress); ev(Event::Tap);
  TEST_ASSERT_TRUE(!m.view().clockMode);
}

static void openSetAlarm() { ev(Event::LongPress); ev(Event::TiltRight); done(); ev(Event::Tap); }

void test_set_alarm_and_ring_snooze_dismiss() {
  at(7, 30); run(100);
  openSetAlarm();
  TEST_ASSERT_TRUE(clip() == Clip::SetAlarm);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "07:30") == 0);  // starts at the current time
  ev(Event::TiltRight); done();          // 08:30
  ev(Event::Tap);                        // minutes
  ev(Event::TiltRight); done();          // 08:35
  ev(Event::TiltRight); done();          // 08:40
  ev(Event::TiltLeft); done();           // 08:35
  ev(Event::Tap);                        // confirm
  TEST_ASSERT_TRUE(clip() == Clip::SetAlarmDone);
  TEST_ASSERT_TRUE(strcmp(m.view().note, "+01:05") == 0);  // rings in 1 h 5 min
  TEST_ASSERT_TRUE(m.takeSaveRequest());
  TEST_ASSERT_TRUE(m.alarm().enabled); TEST_ASSERT_EQUAL(8, m.alarm().hour); TEST_ASSERT_EQUAL(35, m.alarm().minute);
  done(); drain();
  at(8, 34, 59); run(500);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  at(8, 35, 0); run(100);
  TEST_ASSERT_TRUE(clip() == Clip::AlarmRing); TEST_ASSERT_EQUAL(1, m.view().a);
  TEST_ASSERT_EQUAL(1, countSound(Sound::AlarmStage1));
  run(Settings().stageMs);
  TEST_ASSERT_EQUAL(2, m.view().a);
  run(Settings().stageMs);
  TEST_ASSERT_EQUAL(3, m.view().a);
  ev(Event::Tap);                        // snooze
  TEST_ASSERT_TRUE(clip() == Clip::Snooze);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  run(Settings().snoozeMs - 1000);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  run(2000);
  TEST_ASSERT_TRUE(clip() == Clip::AlarmRing);  // rings again after 5 min
  ev(Event::FlipDown);                   // dismiss
  TEST_ASSERT_TRUE(clip() == Clip::Dismiss);
  done(); run(60000);
  TEST_ASSERT_TRUE(clip() == Clip::Home);       // no more ringing
}

void test_alarm_once_per_day_and_not_without_time() {
  Alarm a; a.enabled = true; a.hour = 6; a.minute = 0;
  m.begin(a, T);
  W.valid = false; W.hour = 6; W.minute = 0; run(1000);
  TEST_ASSERT_TRUE(clip() == Clip::Home);        // time unknown: never rings
  at(6, 0); run(100);
  TEST_ASSERT_TRUE(clip() == Clip::AlarmRing);
  ev(Event::Hold); done(); run(1000);
  TEST_ASSERT_TRUE(clip() == Clip::Home);        // same minute: does not re-fire
  W.day++; run(100);
  TEST_ASSERT_TRUE(clip() == Clip::AlarmRing);   // next day: fires again
}

void test_ringing_gives_up_after_10_min() {
  Alarm a; a.enabled = true; a.hour = 6; a.minute = 0;
  m.begin(a, T);
  at(6, 0); run(100);
  run(Settings().ringMaxMs);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_hold_turns_alarm_off() {
  Alarm a; a.enabled = true; m.begin(a, T);
  openSetAlarm();
  ev(Event::Hold);
  TEST_ASSERT_TRUE(!m.alarm().enabled);
  TEST_ASSERT_TRUE(m.takeSaveRequest());
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

static void openFocus() { ev(Event::LongPress); ev(Event::TiltRight); done(); ev(Event::TiltRight); done(); ev(Event::Tap); }

void test_focus_pick_work_done_break() {
  openFocus();
  TEST_ASSERT_TRUE(clip() == Clip::FocusPick); TEST_ASSERT_EQUAL(25, m.view().a);
  ev(Event::TiltRight); done(); TEST_ASSERT_EQUAL(45, m.view().a);
  ev(Event::TiltRight); done(); TEST_ASSERT_EQUAL(60, m.view().a);
  ev(Event::TiltRight); done(); TEST_ASSERT_EQUAL(25, m.view().a);  // wraps
  ev(Event::FlipDown);
  TEST_ASSERT_TRUE(clip() == Clip::FocusStart);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::FocusWork);
  run(100);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "25") == 0);
  run(60000);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "24") == 0);
  run(24UL * 60000);
  TEST_ASSERT_TRUE(clip() == Clip::FocusDone);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::Break);
  run(100); TEST_ASSERT_TRUE(strcmp(m.view().text, "05") == 0);
  run(Settings().breakMs);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_alarm_during_focus_resumes_focus() {
  Alarm a; a.enabled = true; a.hour = 9; a.minute = 0; m.begin(a, T);
  at(8, 50); run(100);
  openFocus(); ev(Event::Tap); done();          // 25 min focus started
  TEST_ASSERT_TRUE(clip() == Clip::FocusWork);
  at(9, 0); run(100);
  TEST_ASSERT_TRUE(clip() == Clip::AlarmRing);
  ev(Event::Hold); done();
  TEST_ASSERT_TRUE(clip() == Clip::FocusWork);  // back to focus, timer still running
}

void test_stopwatch_counts_pauses_and_taps() {
  ev(Event::LongPress); for (int i = 0; i < 3; i++) { ev(Event::TiltRight); done(); } ev(Event::Tap);  // play
  TEST_ASSERT_TRUE(clip() == Clip::Stopwatch);
  run(59100);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "00:59") == 0);
  drain();
  run(1000);
  TEST_ASSERT_TRUE(clip() == Clip::StopwatchTap);
  TEST_ASSERT_EQUAL(1, countSound(Sound::StopwatchMinute));
  done();
  ev(Event::Tap);                               // pause
  run(30000);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "01:00") == 0);
  ev(Event::LongPress);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_set_alarm_starts_from_now() {
  at(10, 18, 30); run(100);
  openSetAlarm();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "10:18") == 0);
  ev(Event::Tap);                             // minutes
  ev(Event::TiltRight); done();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "10:20") == 0);  // snaps to the next 5-min mark
  ev(Event::LongPress);                       // leave
  openSetAlarm(); ev(Event::Tap);
  ev(Event::TiltLeft); done();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "10:15") == 0);
}

void test_set_alarm_before_sync_uses_saved_alarm() {
  openSetAlarm();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "07:30") == 0);
}

void test_minutes_snap_to_5() {
  Alarm a; a.enabled = true; a.hour = 6; a.minute = 17; m.begin(a, T);
  openSetAlarm();                              // no time yet: starts at the saved 06:17
  ev(Event::Tap);                              // minutes
  ev(Event::TiltRight); done();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "06:20") == 0);
  ev(Event::TiltRight); done();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "06:25") == 0);
  m.begin(a, T);
  openSetAlarm(); ev(Event::Tap);
  ev(Event::TiltLeft); done();
  TEST_ASSERT_TRUE(strcmp(m.view().text, "06:15") == 0);
}

void test_rings_in_for_a_time_already_past() {
  at(10, 18, 53); run(100);
  Alarm a; a.enabled = true; a.hour = 10; a.minute = 17; m.setAlarm(a);
  TEST_ASSERT_EQUAL(23 * 60 + 59, m.minutesUntilAlarm());  // 23 h 58 min 7 s, shown rounded up
  a.minute = 18; m.setAlarm(a);
  TEST_ASSERT_EQUAL(0, m.minutesUntilAlarm());              // this minute
  a.enabled = false; m.setAlarm(a);
  TEST_ASSERT_EQUAL(-1, m.minutesUntilAlarm());
}

// Found on hardware 2026-10-25: an input stamped a few ms after the tick time closed the menu at once.
void test_menu_stays_open_when_input_is_newer_than_tick() {
  run(100);
  m.onEvent(Event::LongPress, T + 5);   // the event's time is a little ahead of the next tick
  m.tick(W, T);
  TEST_ASSERT_TRUE(clip() == Clip::Menu);
  run(2000);
  TEST_ASSERT_TRUE(clip() == Clip::Menu);
}


// ---- Wi-Fi setup ("connect your dino"): menu slot 4
static void openWifi() { ev(Event::LongPress); for (int i = 0; i < 4; i++) { ev(Event::TiltRight); done(); } ev(Event::Tap); }

void test_wifi_menu_slot_starts_pairing() {
  drain();
  openWifi();
  TEST_ASSERT_TRUE(clip() == Clip::WifiPair); TEST_ASSERT_EQUAL(0, m.view().a);
  TEST_ASSERT_TRUE(m.takeWifiRequest() == WifiRequest::Start);
  TEST_ASSERT_TRUE(m.takeWifiRequest() == WifiRequest::None);   // read once
  TEST_ASSERT_TRUE(strcmp(m.stateName(), "wifi_setup") == 0);
  m.setWifiInfo("DINO-1234", "K7M2QX9PH4");
  TEST_ASSERT_EQUAL_STRING("DINO-1234", m.view().ssid); TEST_ASSERT_EQUAL_STRING("K7M2QX9PH4", m.view().code);
}

void test_wifi_pairing_phases_follow_the_session() {
  openWifi(); m.takeWifiRequest();
  const uint32_t s0 = m.view().serial;
  m.onWifi(WifiEvent::PhoneJoined, T);
  TEST_ASSERT_TRUE(clip() == Clip::WifiPair); TEST_ASSERT_EQUAL(1, m.view().a); TEST_ASSERT_TRUE(m.view().serial != s0);
  m.onWifi(WifiEvent::PhoneLeft, T);   TEST_ASSERT_EQUAL(0, m.view().a);
  m.onWifi(WifiEvent::PhoneJoined, T);
  m.onWifi(WifiEvent::Connecting, T);  TEST_ASSERT_EQUAL(2, m.view().a);
}

void test_wifi_connected_plays_happy_clip_with_time_then_home() {
  at(10, 24); run(100);
  openWifi(); m.takeWifiRequest(); drain();
  m.onWifi(WifiEvent::Connecting, T);
  m.onWifi(WifiEvent::Connected, T);
  TEST_ASSERT_TRUE(clip() == Clip::WifiDone);
  TEST_ASSERT_TRUE(strcmp(m.view().text, "10:24") == 0);          // "show the time"
  TEST_ASSERT_EQUAL(1, countSound(Sound::AlarmSet));
  ev(Event::LongPress);                                            // one-shot: input ignored
  TEST_ASSERT_TRUE(clip() == Clip::WifiDone);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  TEST_ASSERT_TRUE(m.takeWifiRequest() == WifiRequest::None);     // nothing to cancel: the module closes it itself
}

void test_wifi_failed_plays_sad_clip_then_back_to_pairing() {
  openWifi(); m.takeWifiRequest(); drain();
  m.onWifi(WifiEvent::PhoneJoined, T); m.onWifi(WifiEvent::Connecting, T);
  m.onWifi(WifiEvent::Failed, T);
  TEST_ASSERT_TRUE(clip() == Clip::WifiFail);
  TEST_ASSERT_EQUAL(1, countSound(Sound::Snooze));
  done();
  TEST_ASSERT_TRUE(clip() == Clip::WifiPair); TEST_ASSERT_EQUAL(1, m.view().a);   // the phone is still joined
  TEST_ASSERT_TRUE(strcmp(m.stateName(), "wifi_setup") == 0);
}

void test_wifi_long_press_cancels() {
  openWifi(); m.takeWifiRequest();
  ev(Event::LongPress);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  TEST_ASSERT_TRUE(m.takeWifiRequest() == WifiRequest::Cancel);
  openWifi(); m.takeWifiRequest();
  ev(Event::Hold);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  TEST_ASSERT_TRUE(m.takeWifiRequest() == WifiRequest::Cancel);
}

void test_wifi_tap_does_nothing_and_no_idle_timeout() {
  openWifi(); m.takeWifiRequest();
  ev(Event::Tap); ev(Event::DoubleTap); ev(Event::TiltLeft);
  TEST_ASSERT_TRUE(clip() == Clip::WifiPair);
  run(Settings().menuIdleMs * 6);                                  // the menus give up after 10 s, pairing does not
  TEST_ASSERT_TRUE(clip() == Clip::WifiPair);
}

void test_wifi_timeout_goes_home() {
  openWifi(); m.takeWifiRequest();
  m.onWifi(WifiEvent::TimedOut, T);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_wifi_timeout_during_sad_clip_goes_home_after_it() {
  openWifi(); m.takeWifiRequest();
  m.onWifi(WifiEvent::Connecting, T); m.onWifi(WifiEvent::Failed, T);
  m.onWifi(WifiEvent::TimedOut, T);                                // arrives while the sad clip plays
  TEST_ASSERT_TRUE(clip() == Clip::WifiFail);
  done();
  TEST_ASSERT_TRUE(clip() == Clip::Home);                          // not back to a pairing screen with the network gone
}

void test_wifi_news_is_ignored_when_not_pairing() {
  m.onWifi(WifiEvent::Connected, T);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
  m.onWifi(WifiEvent::PhoneJoined, T);
  TEST_ASSERT_TRUE(clip() == Clip::Home);
}

void test_alarm_during_wifi_setup_rings_and_closes_the_network() {
  at(10, 29); run(100);
  Alarm a; a.enabled = true; a.hour = 10; a.minute = 30; m.setAlarm(a);
  openWifi(); m.takeWifiRequest();
  at(10, 30); run(200);
  TEST_ASSERT_TRUE(clip() == Clip::AlarmRing);                    // utility is exact: the alarm never waits for pairing
  TEST_ASSERT_TRUE(m.takeWifiRequest() == WifiRequest::Cancel);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_home_shows_dashes_until_synced);
  RUN_TEST(test_double_tap_time_board_returns_home);
  RUN_TEST(test_menu_navigation_and_back);
  RUN_TEST(test_menu_times_out);
  RUN_TEST(test_menu_stays_open_when_input_is_newer_than_tick);
  RUN_TEST(test_clock_mode_toggle);
  RUN_TEST(test_set_alarm_and_ring_snooze_dismiss);
  RUN_TEST(test_set_alarm_starts_from_now);
  RUN_TEST(test_set_alarm_before_sync_uses_saved_alarm);
  RUN_TEST(test_minutes_snap_to_5);
  RUN_TEST(test_rings_in_for_a_time_already_past);
  RUN_TEST(test_alarm_once_per_day_and_not_without_time);
  RUN_TEST(test_ringing_gives_up_after_10_min);
  RUN_TEST(test_hold_turns_alarm_off);
  RUN_TEST(test_focus_pick_work_done_break);
  RUN_TEST(test_alarm_during_focus_resumes_focus);
  RUN_TEST(test_stopwatch_counts_pauses_and_taps);
  RUN_TEST(test_wifi_menu_slot_starts_pairing);
  RUN_TEST(test_wifi_pairing_phases_follow_the_session);
  RUN_TEST(test_wifi_connected_plays_happy_clip_with_time_then_home);
  RUN_TEST(test_wifi_failed_plays_sad_clip_then_back_to_pairing);
  RUN_TEST(test_wifi_long_press_cancels);
  RUN_TEST(test_wifi_tap_does_nothing_and_no_idle_timeout);
  RUN_TEST(test_wifi_timeout_goes_home);
  RUN_TEST(test_wifi_timeout_during_sad_clip_goes_home_after_it);
  RUN_TEST(test_wifi_news_is_ignored_when_not_pairing);
  RUN_TEST(test_alarm_during_wifi_setup_rings_and_closes_the_network);
  return UNITY_END();
}
