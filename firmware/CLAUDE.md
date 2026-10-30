# Firmware guide

PlatformIO project for the ESP32-C3 SuperMini (Arduino framework). Build: `pio run -e dino_c3` (or `.venv/bin/pio`). Flash and monitor are done by the user by hand. Serial at 115200.

## Rules

- **Non-blocking always.** No `delay()` longer than 1 ms in the loop, no busy waits. Everything runs from `millis()` and an event queue.
- **Pins and tuning live in `include/config.h`.** Keep `docs/hardware.md` in sync when a pin changes.
- **Behaviour is data.** Reactions, weights, cooldowns and timings live in tables, not in `if` chains.
- **Dino is drawn by the face rig** (`src/face/`), with clips as frame data (`src/scenes/`), not from bitmaps.
- **Flash wear:** write NVS only on meaningful events, never per frame or per sensor sample.
- **No flashes faster than 3 per second.**
- **Utility is exact:** alarms and timers never wait for a mood or animation.
- **Stack:** a `Grid` is 8 KB and must not live on the stack; build one clip at a time.
- Pure logic goes in plain C++ with no Arduino calls, so it can be unit-tested on the desktop (`test/`, `[env:native]`).

## Architecture

Read `README.md` in this folder first: layout, how to add or remove a feature, checks.

```
include/config.h   every pin, feature switch (FEATURE_*) and tuning value
src/
  main.cpp         wiring only: config -> utility::Settings, create switched-on modules, app.add(), app.begin()
  app/             App (the loop: modules -> brain -> sounds / alarm changes -> modules) + Module interface
  modules/         one file pair per feature
  utility/         the brain (menu, alarm, focus, stopwatch). Pure C++, desktop tests
  life/            dino's day. Pure C++, desktop tests
  face/ scenes/    face rig + clips as frame data. Pure C++, checked by tools/face_check.py
  input/           events, gestures, IMU calibration. Pure C++, desktop tests
  drivers/         MPU6050, Wi-Fi time
  sound/           dino sounds + tone player
  timekeeping/     sync schedule, formatting. Pure C++, desktop tests
  players/         stand-alone hardware checks, one env each (play_dino, play_inputs, imu_stream)
```

- A new feature = a new `Module` in `src/modules/` + a `FEATURE_*` switch and its values in `config.h` + its lines in `main.cpp`. Modules never call each other; they go through the App.
- No tuning numbers in code: put them in `config.h` (or, for the pure brain, in `utility::Settings`).
- Comment every file's purpose at the top, and every non-obvious choice.

After any change in `input/`, `utility/`, `life/` or `timekeeping/`, run `pio test -e native`. After any change in `face/` or `scenes/`, run `python3 tools/face_check.py` from the repo root; it must say all frames match.

## Hardware gotchas

- Only GPIO0-5 wake the C3 from deep sleep.
- OLED and MPU6050 share I2C (SDA 8, SCL 9).
- SuperMini needs `ARDUINO_USB_CDC_ON_BOOT=1` for serial over USB (set in platformio.ini).
