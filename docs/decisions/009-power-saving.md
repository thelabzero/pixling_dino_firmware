# ADR 009: Power saving, battery icon, clock mode

Status: code complete 2026-11-09 (firmware 0.4.0); NOT yet run on the board: `docs/power-checklist.md`.

## Why
The first battery run (soak log, 2026-11-09) lasted 4 h 23 min idle on a 450 mAh cell and ended at about 2.8 V
(uncalibrated), which is hard on the cell. The firmware never switched the panel off, never slept the CPU, and had no
way to show the charge. (Fact: soak run; the split of the current between parts is not measured.)

## Decisions
- **Screen follows a power mode** (`power/power_policy`): full -> idle-dim after `SCREEN_DIM_AFTER_S` 60 s untouched ->
  while asleep night-dim -> panel off after `SCREEN_OFF_AFTER_S` 30 s asleep. Touch, pick-up, tilt, shake and any alarm
  or utility screen wake it. Alarms and utility screens are always full brightness (utility is exact).
- **Panel off = display off + charge pump off**, CPU 160 -> 80 MHz while off (`POWER_CPU_*`). On the C3 the APB bus stays
  80 MHz, so I2C and USB are unaffected (assumption, check on the board).
- **Dino sleeps after 180 s** (was 300). Contrast 0xAF (was 0xCF). Both in `config.h`.
- **Battery icon + percentage** drawn over the home screen's top-left corner (`power/hud`), straight into the SSD1306
  buffer: the face frames and `face_check` are untouched. Shown for 8 s after any touch, always at or below 20 %, and as a
  bolt while USB is in. Only on the awake home screen. Not always on, to spare the pixels (burn-in).
- **While USB is in the divider reads the USB voltage, not the cell**, so no percentage can be shown while charging, only
  the bolt. (Fact: boot records read about 4.8 V on USB.)
- **Percent curve is now this cell's measured idle discharge** (soak run), 0 % = the 3.30 V cutoff. The old generic curve
  read about 150 mV high. Two runs from the blue light started at 3.94 and 3.83 V, so a full cell reads about 3.9 V on this board (the ADC under-reads; assumption: 6-9 %). 100 % is 3.96 V in these readings.
- **The shown percent never goes up on battery** (`PercentFilter`), so a loud sound's voltage sag does not bounce it.
- **Empty-battery shutdown** (`BATTERY_CUTOFF_MV` 3300 for 30 s, never while an alarm rings): soak event
  `low_battery_sleep`, flush the log, OLED and IMU off, deep sleep. It wakes every 30 s to look at USB and the voltage,
  and boots normally when USB is plugged in or the rested cell is above 3650 mV. A touch while still empty goes back to sleep.
- **Clock option (menu > CLOCK) now does something:** it was only a flag nobody drew. It now shows the time (24 h, 3x5
  digits) in the bottom-right corner of the home screen, moved by the burn-in shift. Needs the Wi-Fi sync to have run once.

## Not done, on purpose
- **Light sleep between loops:** the Arduino build has no power management, so a `delay(1)` loop cannot be put to light
  sleep without a deadline scheduler in the App; riskier for the alarm. Later, if the measured saving justifies it.
- **IMU low-power mode / slower sampling:** the gesture thresholds were tuned at 50 Hz (`play_inputs` 18/20); changing
  the rate needs a re-run of that check. The MPU6050 is only switched off in the empty-battery sleep.
- **Hardware:** MPU INT to GPIO0 (motion wake) and removing the SuperMini power LED stay owner decisions.

## Consequences
- Fact: 132 native tests pass, all envs build, face check passes, flash 81.3 %.
- Estimate (not measured): panel off should be the largest idle saving. Measure with a soak run from a full charge.
- Risk: a first wake from panel-off may show the previous frame for one loop; deep-sleep wake behaviour on this board is untested.
