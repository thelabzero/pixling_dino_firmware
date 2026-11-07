# D4 board checklist (dino_c3)

D4 closes when the main firmware has run on the board and the items below pass. Flash and monitor by hand; paste the monitor output of anything that fails and we fix only that. Known gaps (pet hold, double tap, tilt level, voice timing, shake threshold) are not part of this run: they go to D5.

```
cd firmware
cp include/secrets.example.h include/secrets.h     # once; fill in Wi-Fi (git-ignored)
.venv/bin/pio run -e dino_c3 -t upload && .venv/bin/pio device monitor -e dino_c3 -f time
```

Serial keys (type in the monitor; `?` lists them): `t` tap, `d` double tap, `l` long press, `h` hold, `,` `.` tilt, `f` `u` flip, `a` alarm at next minute, `i` info, `b` battery, `n` rawr now, `v` each life clip, `z` fall asleep now, `s` Wi-Fi sync, `p` next sound, `m` mute.

| # | Check | Pass when |
|---|---|---|
| 1 | Boot | Banner, every module `on` (none `NOT FOUND`), no reset loop. Press `i`: free heap and stack left are printed; **stack left stays above 2 KB** after you have visited menu, focus and alarm (16 KB loop stack is set for main) |
| 2 | Home | Dino face shows and idles (blink, glance). `v` plays each life clip in turn: compare with `art/reference/dino-v1/dino_contact_*.png` |
| 3 | Timing | `i` shows screen render last / worst. Note the worst value. No visible stutter in blinks. The glasses frames (focus) were the slow ones |
| 4 | Time | After Wi-Fi sync, `d` shows the time board; the time is right (IST). Without Wi-Fi it must say time not known and not crash |
| 5 | Touch pad | Touch the pad: monitor prints `[touch] up after N ms` and `[in] TAP`. Tap, long press (1.5 s) and hold (3 s) each give the right event |
| 6 | Menu | Long press opens the menu; tilt (or `,` `.`) moves, tap chooses; 10 s idle returns home |
| 7 | Alarm | `a`: rings at the next minute; the three stages escalate; tap = snooze 5 min; hold or flip face-down = dismiss. Alarm survives a reset (`[alarm] restored`) |
| 8 | Focus | Menu, Focus, pick 25, tap: timer runs; long press gives up; face-down starts one |
| 9 | Stopwatch | Tap pauses and resumes; long press exits |
| 10 | Sounds | `p` steps through the sounds (heard OK before); `m` mutes. Sounds play with the screen, not instead of it |
| 11 | Life | `z` falls asleep and the screen dims; any touch wakes it. `n` plays rawr. Shake, pick up and tilt each get a reaction |
| 12 | Battery | `b` prints a plausible voltage (or says no battery / USB only) |
| 13 | Soak | Leave it 30 min on the home screen: no reset, heap steady (`i` at start and end) |

Result log: add a dated "Run 1" section here with the pass/fail per row, the worst render time, stack left and heap.

## Run 1 (2026-11-07, owner, ~11 min on USB power, no battery)

Facts from the monitor log. Firmware 0.3.0. No resets.

| # | Result | Evidence |
|---|---|---|
| 1 Boot | pass | free heap 215-221 KB, stack left min 9,792 B of 16 KB |
| 2 Home | clips ran, look not reported | `v` stepped all 8 life clips (glance .. fall_asleep). Owner still to compare with the contact sheets |
| 3 Timing | pass on numbers | home render last 38-41 ms, worst 74 ms. No stutter reported |
| 4 Time | pass | first sync 2.6 s, then Wi-Fi off, IST correct; double tap shows the time board |
| 5 Touch | pass | tap, double tap, long press (1.6-2.6 s), hold (3.97 s) all fired |
| 6 Menu | pass | long press opens it, tilt moves, tap chooses Focus and Stopwatch. Idle timeout: menu 19:02:24 -> home 19:02:34 (10 s, as set). Set alarm opened from the menu; long press leaves it |
| 7 Alarm | partly | `a` rang at 18:46:00, tap = snooze, rang again 5 min later, twice. Stage 2 came exactly 20 s after stage 1 (18:57:13 -> 18:57:33, no touch). Flip face-down dismissed it (`dismiss` sound, home after 4 s). Hold-dismiss and restore after reset not tested |
| 8 Focus | pass | pick, start by tap, long press gives up; hold in focus_work also went home (18:59:35). Face-down start works: `FLIP_DOWN (focus_pick)` -> focus_start -> focus_work (19:04:20). Focus finishing and the break were not run (25 min) |
| 9 Stopwatch | pass | tap pause/resume, long press exits |
| 10 Sounds | pass | all 21 sounds played; pitch sweep run; `m` muted and un-muted (twice) |
| 11 Life | pass | rawr, pet, overpet, shake, pickup, tilt, fall asleep (screen dimmed, contrast 1) seen. Asleep at 19:10 -> tap at 19:12:39 -> `[life] wake` -> idle. Idle quirk rawr fired by itself (19:08, 19:16), self-sleep after 5 min (19:17) |
| 12 Battery | pass | 4.7-4.9 V flagged as USB voltage, no cell connected (as designed) |
| 13 Soak | pass | 18:43 -> 19:39 (56 min), no reset. Free heap 215,284-221,200 B at 7 readings (no downward trend), stack left 9,792 B every time, IMU read errors 0, battery line every minute (one late line at 19:35, same serial-delay effect) |

Findings:
- **Loudest pitch: 4500 Hz** (owner, by ear, from the `r` sweep). Sounds today sit at 1-4 kHz, mostly 1.3-2.4 kHz.
- **Log lines arrive late** (state lines up to 8 s after their own `at hh:mm:ss`). Cause (assumption, fits every case): the C3's USB serial holds a short last line until the next print pushes it out. The loop is not stalled: the stamp inside the line is device time. Alarm stage 2 looked 38 s late for the same reason (the second and third alarms stepped at exactly 20 s, 18:51:38 -> 18:51:58 and 18:57:13 -> 18:57:33).
- **PICK_UP fires while tilting** (home and menus) and tilt repeats while the board rests tilted: gesture tuning, D5.
