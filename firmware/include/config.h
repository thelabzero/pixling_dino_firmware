// Dino: every pin, feature switch and tuning value in one place.
// Change a value here, rebuild, upload. Nothing else in the code holds a number you should
// need to tune. The pinout is also in docs/hardware.md: keep both in sync.
//
// Sections:  1 features   2 pins   3 display   4 IMU   5 gestures   6 time   7 utility   8 debug
#pragma once

// =====================================================================================
// 1. Features: 1 = on, 0 = off. A feature that is off is not built at all (saves flash),
//    and the dino still runs without it. To add one: "Adding a feature" in firmware/README.md.
// =====================================================================================
#define FEATURE_IMU             1   // MPU6050: tilt and flip (menu, set alarm, dismiss)
#define FEATURE_WIFI_TIME       1   // Wi-Fi time sync; off = clock shows --:-- and alarms never ring
#define FEATURE_SERIAL_CONSOLE  1   // keys in the Serial monitor: touch stand-in + test commands
#define FEATURE_BUZZER          1   // passive piezo on PIN_BUZZER: the dino's sounds
#define FEATURE_SOUND_LOG       1   // also print each sound as "[sound] name"
#define FEATURE_ALARM_STORE     1   // keep the alarm in flash (NVS) across power cuts
#define FEATURE_LIFE            1   // the dino's day on the home screen; off = the plain idle loop
#define FEATURE_TOUCH           1   // HW-763 / TTP223 touch pad: tap, double tap, long press, hold
#define FEATURE_BATTERY         1   // battery voltage through the divider on PIN_BATTERY_ADC

// =====================================================================================
// 2. Pins (ESP32-C3 SuperMini)
// =====================================================================================
#define PIN_I2C_SDA      8  // OLED + MPU6050 share this bus
#define PIN_I2C_SCL      9
#define I2C_FREQ_HZ      400000
#define PIN_TOUCH        4  // HW-763 touch OUT (GPIO0-5 can wake from deep sleep)
#define PIN_BUZZER      10  // passive piezo: GPIO10 -> ~100 ohm -> piezo -> GND. No boot/JTAG/wake role
#define PIN_BATTERY_ADC  3  // battery divider midpoint (ADC1)
// Planned, not wired yet:
#define PIN_IMU_INT      0  // MPU6050 INT; must be GPIO0-5

// =====================================================================================
// 3. Display (SSD1306 0.96" 128x64, I2C)
// =====================================================================================
#define SCREEN_W     128
#define SCREEN_H     64
#define OLED_ADDR    0x3C
#define OLED_RESET   -1
#define SCREEN_DAY_CONTRAST    0xCF   // 0-255
#define SCREEN_NIGHT_CONTRAST  0x01   // while the dino sleeps (the SSD1306 dims only a little: off is the real saving)

// =====================================================================================
// 4. IMU (MPU6050): calibration of THIS unit
//    Measured 2026-10-30 with tools/imu_calibrator (dino unit). Re-run it if the IMU is re-mounted
//    or replaced: guide in docs/imu-calibration.md.
// =====================================================================================
#define IMU_SAMPLE_MS 20    // 50 Hz

// Accel: corrected = (raw_g - OFFSET) / SCALE.
#define IMU_ACC_OFFSET_X  0.0865f
#define IMU_ACC_OFFSET_Y -0.0228f
#define IMU_ACC_OFFSET_Z  0.0152f
#define IMU_ACC_SCALE_X   0.9944f
#define IMU_ACC_SCALE_Y   0.9943f
#define IMU_ACC_SCALE_Z   0.9986f
// Gyro bias (deg/s): starting value only; re-learned at run time while still.
#define IMU_GYRO_BIAS_X  -2.6196f
#define IMU_GYRO_BIAS_Y   1.3727f
#define IMU_GYRO_BIAS_Z   0.6541f

// Mounting, in sensor axes (after the accel correction above): chip up ~6 deg off sensor Z,
// screen faces sensor +X.
#define IMU_UP_X     0.1081f
#define IMU_UP_Y    -0.0142f
#define IMU_UP_Z     0.9940f
#define IMU_FRONT_X  0.9878f
#define IMU_FRONT_Y  0.1145f
#define IMU_FRONT_Z -0.1058f

// =====================================================================================
// 5. Gestures (starting values; tune once the IMU is rigidly mounted, plan step 2)
// =====================================================================================
#define GESTURE_TILT_ON_DEG      25     // tilt further than this = tilt left / right
#define GESTURE_TILT_OFF_DEG     12     // back under this = tilt released
#define GESTURE_TILT_HOLD_MS     120    // must stay tilted this long
#define GESTURE_TILT_REPEAT_MS   700    // repeats while held (0 = once per tilt)
#define GESTURE_FLIP_HOLD_MS     1000   // face-down this long = flip
#define GESTURE_SHAKE_PEAK_G     1.7f   // acceleration peak that counts towards a shake
#define GESTURE_SHAKE_PEAKS      3      // this many peaks within 1 s = shake

// =====================================================================================
// 5b. Touch pad (HW-763 / TTP223). Timings from docs/interaction-spec.md, except hold (see below).
// =====================================================================================
#define TOUCH_ACTIVE_HIGH     1      // TTP223 default: OUT = HIGH while touched
#define TOUCH_DEBOUNCE_MS     30
#define TOUCH_TAP_MAX_MS      300    // shorter = tap
#define TOUCH_DOUBLE_GAP_MS   400    // second tap within this = double tap
#define TOUCH_LONG_MS         1500   // released after this (before hold) = long press: menu, back
#define TOUCH_HOLD_MS         3000   // still touching at this = hold: alarm off, dismiss.
                                     // Spec says 2 s; 3 s leaves a 1.5 s window for long press.

// =====================================================================================
// 5c. Battery (single Li-ion cell through a divider: Battery+ -> R_TOP -> pin -> R_BOTTOM -> GND)
// =====================================================================================
#define BATTERY_R_TOP_K      330.0f  // kOhm
#define BATTERY_R_BOTTOM_K   330.0f  // kOhm
#define BATTERY_CAL          1.00f   // multimeter reading / the board's reading, to correct the ADC
#define BATTERY_SAMPLES      16      // ADC reads averaged per reading
#define BATTERY_READ_MS      2000    // one reading every 2 s (smoothed)
#define BATTERY_LOG_S        60      // print the battery this often (0 = only on "b" / "i")
#define BATTERY_MISSING_MV   2500    // below this: no battery connected
#define BATTERY_MAX_MV       4350    // above this it is not a Li-ion cell: USB / charger voltage on the divider

// =====================================================================================
// 5d. Buzzer. The sounds themselves are tables in src/sound/dino_sounds.cpp.
// =====================================================================================
#define BUZZER_VOLUME        100     // 0-100 (0 = silent). Scales every sound; a piezo on 3.3 V is not loud, start at 100

// =====================================================================================
// 6. Time (Wi-Fi time sync, ADR 010)
//    Wi-Fi name/password: include/secrets.h (copy secrets.example.h; git-ignored).
// =====================================================================================
#define TIME_TZ                 "IST-5:30"        // POSIX time zone. Others: "GMT0", "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1            "pool.ntp.org"
#define NTP_SERVER_2            "time.google.com"
#define TIME_SYNC_INTERVAL_H    24                // re-sync once a day
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define NTP_TIMEOUT_MS          20000             // first answer can take >10 s (seen 2026-10-19)
#define WIFI_LOW_TX_POWER       1                 // many SuperMini boards only connect at reduced TX power

// =====================================================================================
// 7. Utility: alarm, focus, menu (owner decisions: docs/plan-c2-c3.md, step 8)
// =====================================================================================
#define ALARM_STAGE_S        20            // each ring stage (gentle -> pawing -> standing) lasts this long
#define ALARM_RING_MAX_MIN   10            // nobody answers: stop ringing after this
#define ALARM_SNOOZE_MIN     5             // tap while ringing = ring again after this
#define ALARM_MINUTE_STEP    5             // set alarm: one tilt on the minutes = this many minutes
#define FOCUS_CHOICES_MIN    {25, 45, 60}  // focus lengths offered, in order (max 4, each 1-99)
#define BREAK_MIN            5             // break after a finished focus
#define MENU_IDLE_S          10            // menu / set alarm / focus pick go home after this without input
#define BURN_IN_SHIFT_MIN    3             // clock mode: the picture moves 1 px this often

// =====================================================================================
// 7b. Dino's life (home screen: the design sheet's Life clips). Times in seconds / ms.
// =====================================================================================
#define LIFE_SLEEP_AFTER_S     300    // no input this long -> falls asleep, screen dims once asleep
#define LIFE_RAWR_MIN_S        120    // idle quirk (rawr) every 2-4 min
#define LIFE_RAWR_MAX_S        240
#define LIFE_OVERPET_TOUCHES   4      // this many touches ...
#define LIFE_OVERPET_WINDOW_S  8      // ... within this long = over-petted
#define LIFE_OVERPET_LOCKOUT_S 10     // then touches only get a glance for this long
#define LIFE_REACT_MIN_MS      120    // pause before reacting to a touch (reads as noticing)
#define LIFE_REACT_MAX_MS      300
#define LIFE_TILT_COOLDOWN_S   4      // the tilt clip does not repeat faster than this

// =====================================================================================
// 8. Debug (Serial monitor)
// =====================================================================================
#define SERIAL_BAUD          115200
#define LOG_INPUT            1             // print each input event: "[in] TAP (home)"
#define LOG_STATE            1             // print each state change: "[state] home -> menu at 10:02:11"
#define LOG_TOUCH_RAW        1             // print every touch down / up with its length (for tuning)
#define LOG_LIFE             1             // print each life behaviour: "[life] pet"
