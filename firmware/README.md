# Dino firmware

The firmware for dino, the pixel dinosaur on your desk: an ESP32-C3 SuperMini with a 0.96" OLED and an MPU6050. Dino keeps the time (Wi-Fi), wakes you with an alarm, runs focus sessions and a stopwatch, and lives entirely in its face, drawn live by the face rig.

## Quick start

1. Install [VS Code](https://code.visualstudio.com/) with the **PlatformIO** extension, or the PlatformIO command line. The project's own copy is `.venv/bin/pio` (create it with `python3 -m venv .venv && .venv/bin/pip install platformio pyserial pillow`).
2. Wi-Fi for the clock: on the device, long press > **Wi-Fi** and follow the screen ("connect your dino", ADR 005). For development you can instead copy `include/secrets.example.h` to `include/secrets.h` and fill in your network (a network saved on the device wins; `secrets.h` is never committed).
3. Connect the board over USB-C, then from this folder:

```
pio run -e dino_c3 -t upload && pio device monitor -e dino_c3
```

Add `-f time` to the monitor to put a time stamp on every line. On start-up the monitor lists every module and whether it was found.

## Using it

| Do | What happens |
|---|---|
| Power on / reset | The Pixling boot film (about 5 s), then the home face. A tap skips it. `B` in the monitor replays it |
| Long press | Menu: Clock, Alarm, Focus, Stopwatch, Wi-Fi, Back. Tilt to move, tap to choose |
| Double tap | Time board |
| Menu → Alarm | Set alarm: starts at the current time. Tilt = ±1 h, tap, tilt = ±5 min, tap = save. Hold = alarm off |
| Alarm ringing | Tap = snooze 5 min. Hold or flip face-down = dismiss. Stops by itself after 10 min |
| Menu → Focus | Tilt to pick the length, tap or flip face-down to start. Long press = give up. A break follows |
| Menu → Stopwatch | Tap = pause/resume, long press = exit |
| Menu → Wi-Fi | "Connect your dino": join the `DINO-xxxx` network on your phone with the password on the screen, pick your Wi-Fi on the page that opens. Long press = cancel. Gives up after 5 min |

On the home screen dino lives its day (times and weights in `config.h` and `src/life/`): it wakes up, idles with small quirks, naps, gets drowsy in the evening and sleeps at night with the screen dimmed. Taps, shakes, pick-ups and tilts get a reaction.

The boot film is switched by `FEATURE_BOOT_INTRO` and `BOOT_INTRO_FILM` in `config.h`. The soak log (flash black box for long runs, keys `w` dump and `x x` erase) is `FEATURE_SOAK_LOG`.

Touch can also be typed in the Serial monitor. Press `?` to list every key (tap, double tap, long press, hold, tilt, flip, test alarm, info, buzzer audition...). The touch pad (GPIO4) works alongside these keys.

## Changing settings

**Everything you might tune is in [`include/config.h`](include/config.h)**: feature switches (`FEATURE_*`, off = not built at all), pins, display, IMU calibration (re-measure with `tools/imu_calibrator`, [guide](../docs/imu-calibration.md)), gestures, time, utility timings and debug output. Change the value, rebuild, upload.

## How the code is organised

```
include/config.h      all settings
include/secrets.h     your Wi-Fi (git-ignored; copy secrets.example.h)
src/
  main.cpp            wiring only: config -> settings, create the switched-on modules, start the App
  app/                the loop (modules -> brain -> sounds / alarm changes -> modules) + Module interface
  modules/            one file pair per feature: screen, life, imu_input, touch_input, battery,
                      buzzer, wifi_clock, alarm_store, serial_console, sound_log, soak_log
  utility/            the brain: menu, alarm, focus, stopwatch. Pure C++, desktop-tested
  life/               dino's day: phases, behaviour choice, reactions. Pure C++, desktop-tested
  face/ scenes/       the face rig and every screen as frame data. Pure C++, checked by tools/face_check.py
  input/              events, touch + IMU gestures, IMU calibration. Pure C++, desktop-tested
  power/              battery voltage -> charge. Pure C++, desktop-tested
  sound/              dino sounds as tables + the tone player. Pure C++, desktop-tested
  boot/               the boot-up film (ADR 006): frame data, renderer, IntroPlayer clock. Pure C++, desktop-tested; played by modules/screen
  soak/               soak-log record format and decisions (ADR 007). Pure C++, desktop-tested
  drivers/            MPU6050, Wi-Fi time sync, Wi-Fi setup portal
  timekeeping/        sync schedule, time formatting. Pure C++, desktop-tested
  wifi/               Wi-Fi setup rules: credential checks, form decoding, the pairing session. Pure C++, desktop-tested
  players/            stand-alone hardware checks (one env each)
test/                 desktop unit tests
```

Modules never call each other. They only talk to the App, so adding or removing one does not touch the rest.

## Adding a feature

1. Write a `Module` in `src/modules/` (see `src/app/module.h`; `sound_log.h` is the smallest example). Override only what you need: `begin()` (return `false` if hardware is missing), `loop()` (never block), `onSound()`, `onVoice()`, `onKey()`.
2. Add a `FEATURE_*` switch and its values to `include/config.h`.
3. Plug it in in `src/main.cpp`, inside `#if FEATURE_*`. The order of `app.add()` is the start order.
4. Logic worth testing goes in pure C++ with a test in `test/`.

To remove a feature, set its switch to `0`; to delete it for good, remove its files, its lines in `main.cpp` and its section in `config.h`.

## Changing dino's sounds

The sounds are tables in `src/sound/dino_sounds.cpp`: each step is {start Hz, end Hz, ms, volume}. Listen before uploading: from the repo root

```
c++ -std=c++17 -O2 -Ifirmware/src -o /tmp/sound_preview tools/sound_preview.cpp firmware/src/sound/*.cpp
```

On the device, `p` plays them one by one. Overall loudness: `BUZZER_VOLUME` in `config.h`.

## Checking your changes

| After changing | Run | Expect |
|---|---|---|
| `utility/`, `life/`, `input/`, `sound/`, `timekeeping/`, `wifi/`, `boot/`, `soak/` | `pio test -e native` | all tests pass (120) |
| `face/`, `scenes/` | `python3 tools/face_check.py` (repo root) | all frames match (double and float) |
| anything | `pio run -e dino_c3` | builds with no warnings |

Hardware checks, one small program each (flash and watch the monitor by hand):

| Env | Checks |
|---|---|
| `play_dino` | every dino clip in a loop, with render times |
| `play_boot` | the three boot film candidates, with timing (keys 1 2 3, a, l, s) |
| `play_inputs` | I2C scan, IMU gestures with on-screen counters |
| `imu_calibrator` | streams the IMU for `tools/imu_calibrator` |

```
pio run -e play_inputs -t upload && pio device monitor -e play_inputs
```

## Rules

- **Non-blocking**: no `delay()` over 1 ms in the loop, no waiting on hardware.
- **Utility is exact**: alarms and timers follow the clock, never animation progress.
- **Flash wear**: write NVS only when something meaningful changes.
- **No flashes faster than 3 per second.**
- **Stack**: a `Grid` is 8 KB; keep it off the stack. The loop task has 16 KB (`main.cpp`).

More: `CLAUDE.md` (rules for Claude Code), `../docs/hardware.md` (wiring), `../docs/checkpoints.md` (status).
