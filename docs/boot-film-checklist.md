# Boot film: board checklist

Flash: `cd firmware && .venv/bin/pio run -t upload && .venv/bin/pio device monitor`

| # | Check | Expect |
|---|---|---|
| 1 | Plug in power | Serial: `[screen] boot film: C zoom...`; name writes in, dot hops, zooms, Dino, face zoom, live face, blink; then the home screen. About 4.9 s |
| 2 | Press reset | Plays again (every boot) |
| 3 | Tap during the film | `[screen] boot film skipped`, live face shows 0.5 s, then home. The tap does NOT show the time board or pet |
| 4 | Serial key `B` | Replays the film, any time |
| 5 | Serial `i` right after boot | free heap and stack look normal (stack left > 2 KB) |
| 6 | Smoothness | Any visible stutter? Note which step (Serial `i` shows render times) |
| 7 | Alarm set, reboot a minute before it | Alarm still rings on time after the film |
| 8 | Change `BOOT_INTRO_FILM` to 0 or 1, flash | The other films play the same way |
