# Touch-only mode: board checklist (env dino_touch_c3)

Flash and monitor by hand: `cd firmware && .venv/bin/pio run -e dino_touch_c3 -t upload && .venv/bin/pio device monitor -e dino_touch_c3`.
Boot line should say `Dino Touch firmware 0.4.0-touch` and no `[imu]` line. Mark each line.

## Gestures
- [ ] A tap reacts at once (no pause before the dino reacts). Two quick taps are two taps.
- [ ] A touch of about 0.3-0.7 s does nothing. A touch longer than 0.7 s acts when you lift your finger, however long it is (try 5 s).
- [ ] (`LOG_TOUCH_RAW` prints each touch with its length: tune `TOUCH_LONG_MS` in `config.h` if 700 ms feels wrong.)

## Home screen taps (taps are counted here, so a single tap answers about 0.35 s late)
- [ ] 1 tap: the dino is petted. 2 taps: the time board. 3 taps: cute (shy, heart eyes, wink). 4 taps: angry (steam, shaking).
- [ ] The 4th tap reacts without the extra wait. 4 quick taps never end up as 2 + 2.
- [ ] Asleep: 3 or 4 taps wake it first. A clip already playing is not cut short.
- [ ] If counting feels too slow or too fast, tune `TOUCH_MULTI_GAP_MS` in `config.h` (350).

## Clock mode
- [ ] Menu > Clock ON: the time shows bottom-right, clearly bigger than before (34 x 10 px), nothing touches the face in any clip (also during cute / angry).
- [ ] It moves 1 px every 3 min and stays inside the screen.

## Menu list
- [ ] Long tap on home: 3 rows at a time in large text, the first row inverted, a bar on the right edge.
- [ ] Tap: the list scrolls one row (short scroll), the mark ends in the middle row; at Clock the mark is on top, at Back at the bottom. Wraps from Back to Clock. Menu move sound each time.
- [ ] Tap quickly 5 times: every tap counts, none is lost.
- [ ] The label and the value (ON / OFF, alarm time) never overlap; "STOPWATCH" fits on its row.
- [ ] Clock row shows ON / OFF; long tap on it flips it and goes home (the big time shows bottom-right when ON).
- [ ] Alarm row shows OFF, or the alarm time once one is set.
- [ ] Stopwatch row: long tap starts it; tap pauses / starts; long tap leaves.
- [ ] Back row (or 10 s untouched): home.

## Alarm
- [ ] Alarm row, long tap: tap = hours +1, long tap -> minutes (tap = +5), long tap -> "ALARM ON".
- [ ] Tap flips ON / OFF. Long tap on ON: saved, happy clip, "+HH:MM" note. Long tap on OFF: alarm off, home, off sound.
- [ ] Alarm set one minute ahead rings. Tap = snooze; long tap = dismiss. Still set after a reset.

## Focus, face, Wi-Fi
- [ ] Focus: tap cycles 25 / 45 / 60, long tap starts, long tap in the work screen gives up.
- [ ] Face: tap = next face (only ">" shown), hint "LONG TAP: PICK", long tap = warning "LONG TAP: YES / TAP: NO".
      Tap = back to the picker (nothing saved); long tap = saved, still the default after a reset.
- [ ] Wi-Fi row: setup screen opens; long tap leaves.
- [ ] Nothing in the monitor mentions tilt, flip or shake; no resets.
