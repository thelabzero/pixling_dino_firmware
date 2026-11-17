# Power saving: board checklist (firmware 0.4.0)

Flash and monitor by hand: `cd firmware && .venv/bin/pio run -t upload && .venv/bin/pio device monitor`. Mark each line.

## Battery icon
- [ ] After boot the film ends, then a battery icon with a number shows top-left for about 8 s, then disappears.
- [ ] Tap the dino: the icon shows again for about 8 s.
- [ ] On USB: a bolt is shown and stays (no number; expected).
- [ ] Unplug USB: the number matches `b` in the monitor (the monitor shows the raw % and the shown %).
- [ ] The number never goes up on battery; it does not jump when the buzzer sounds.

## Screen power
- [ ] Untouched for 60 s on home: the screen dims (monitor: `i` shows `power idle-dim`).
- [ ] Asleep (`z` to force): dims, then after 30 s the panel switches off (`[screen] panel off`, `i` shows `CPU 80 MHz`).
- [ ] Touch, pick up, tilt or shake while off: the panel comes back within about 0.3 s, no flash of an old frame, CPU back to 160.
- [ ] Menu, alarm, focus and stopwatch are never dimmed or off.
- [ ] Alarm set one minute ahead while the panel is OFF: the panel wakes and the alarm rings at the minute (+- 1 s). Check `alarm_ring` in the soak log.
- [ ] A Wi-Fi sync (`s`) while the panel is off still works.
- [ ] USB serial and `tools/soak_dump.py` still work with the panel off.

## Clock option
- [ ] Menu > CLOCK, tap: time shows bottom-right on the home screen, moves 1 px every 3 min, correct minute.
- [ ] Menu > CLOCK again: gone. With the time not synced yet nothing is drawn.

## Low-battery sleep (do this last)
- [ ] Soak run from a full charge: the log should end with a `low_battery_sleep` event near 3.3 V, not a dead cell.
- [ ] Plug in USB: it wakes within about 30 s (boot film, normal).
- [ ] Touch while asleep and still empty: nothing happens, it sleeps again.

## Compare
- [ ] Baseline run (0.3.1, idle, from full charge): ____ h to `battery_low`. This run: ____ h. Aim: at least double.
