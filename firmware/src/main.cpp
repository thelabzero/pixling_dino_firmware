// Dino firmware: entry point.
//
// main.cpp only wires things together: it turns config.h into settings, creates the
// modules that are switched on (config.h, section 1) and hands them to the App.
// What each part does:
//   app/        the core: brain + modules + the loop that connects them
//   modules/    one file pair per feature (screen, life, IMU, touch, battery, buzzer, Wi-Fi time, alarm storage, console, sound log)
//   utility/    the brain: menu, alarm, focus, stopwatch (pure C++, desktop-tested)
//   life/       the dino's day: idle, pet, RAWR, shake, sleep and wake (pure C++, desktop-tested)
//   face/ scenes/  drawing the dino and the utility screens (pure C++, checked against the design by tools/face_check.py)
//   input/ power/ drivers/ timekeeping/   gestures, battery maths, hardware drivers, time-sync logic
// Guide: firmware/README.md.
#include <Arduino.h>

// The default loop stack is 8 KB; the screen pipeline and the clip builders need headroom (measured: docs/debug-play-dino.md).
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#include <Wire.h>
#include "config.h"
#include "app/app.h"
#include "modules/screen.h"
#if FEATURE_ALARM_STORE
#include "modules/alarm_store.h"
#endif
#if FEATURE_WIFI_TIME
#include "modules/wifi_clock.h"
#endif
#if FEATURE_IMU
#include "modules/imu_input.h"
#endif
#if FEATURE_SERIAL_CONSOLE
#include "modules/serial_console.h"
#endif
#if FEATURE_BUZZER
#include "modules/buzzer.h"
#endif
#if FEATURE_SOUND_LOG
#include "modules/sound_log.h"
#endif
#if FEATURE_TOUCH
#include "modules/touch_input.h"
#endif
#if FEATURE_LIFE
#include "modules/life.h"
#endif
#if FEATURE_BATTERY
#include "modules/battery.h"
#endif

// config.h (section 7) -> the brain's settings.
static utility::Settings settingsFromConfig() {
  utility::Settings s;
  s.stageMs = (uint32_t)ALARM_STAGE_S * 1000UL;
  s.ringMaxMs = (uint32_t)ALARM_RING_MAX_MIN * 60000UL;
  s.snoozeMs = (uint32_t)ALARM_SNOOZE_MIN * 60000UL;
  s.breakMs = (uint32_t)BREAK_MIN * 60000UL;
  s.menuIdleMs = (uint32_t)MENU_IDLE_S * 1000UL;
  s.burnInShiftMs = (uint32_t)BURN_IN_SHIFT_MIN * 60000UL;
  s.alarmMinuteStep = ALARM_MINUTE_STEP;
  static const int focus[] = FOCUS_CHOICES_MIN;
  const int n = (int)(sizeof(focus) / sizeof(focus[0]));
  for (int i = 0; i < utility::Settings::MAX_FOCUS; i++) s.focusMinutes[i] = i < n ? focus[i] : 0;
  return s;
}

static App app(settingsFromConfig());

// ---- Modules. Start order = the order of app.add() in setup().
#if FEATURE_ALARM_STORE
static AlarmStore alarmStore;   // first: restores the saved alarm
#endif
#if FEATURE_WIFI_TIME
static WifiClock wifiClock;
#endif
#if FEATURE_IMU
static ImuInput imuInput;
#endif
#if FEATURE_TOUCH
static TouchInput touchInput;
#endif
#if FEATURE_BATTERY
static Battery battery;
#endif
#if FEATURE_SERIAL_CONSOLE
static SerialConsole console;
#endif
#if FEATURE_BUZZER
static Buzzer buzzer;
#endif
#if FEATURE_LIFE
static Life life_;
#endif
#if FEATURE_SOUND_LOG
static SoundLog soundLog;
#endif
static Screen screen;           // last: draws what the others set up

void setup() {
  Serial.begin(SERIAL_BAUD);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);  // shared I2C bus: OLED + MPU6050
  Wire.setClock(I2C_FREQ_HZ);

#if FEATURE_ALARM_STORE
  app.add(alarmStore);
#endif
#if FEATURE_WIFI_TIME
  app.add(wifiClock);
#endif
#if FEATURE_IMU
  app.add(imuInput);
#endif
#if FEATURE_TOUCH
  app.add(touchInput);
#endif
#if FEATURE_BATTERY
  app.add(battery);
#endif
#if FEATURE_SERIAL_CONSOLE
  app.add(console);
#endif
#if FEATURE_BUZZER
  app.add(buzzer);
#endif
#if FEATURE_LIFE
  app.add(life_);
#endif
#if FEATURE_SOUND_LOG
  app.add(soundLog);
#endif
  app.add(screen);

  app.begin();
}

void loop() {
  app.loop();
  delay(1);  // let the Wi-Fi / USB tasks run; everything else is driven by millis()
}
