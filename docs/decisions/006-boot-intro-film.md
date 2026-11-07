# ADR 006: Boot-up film

Status: accepted 2026-11-07 (owner chose film C after previewing A, B and C on the board with `env:play_boot`).
Note: the parked platform plan (app, looks pipeline, device identity) was going to use 006-008; renumber those when they are written.

## Decision
- **Film C "zoom"** is the boot-up animation: the name "Pixling" writes in, the i's dot hops, zoom 2x/4x/8x into the dot, the dot becomes Dino, zoom 2x/4x into its face, the face grows into the live face and blinks once; the live face (real rig, `Ex::Idle`) then hands over to the home screen. 4.9 s (fact: sum of the step holds, `test_boot_film`). Films A (born, with the pixel person) and B (Dino hops onto the i) stay in `src/boot/` and in `env:play_boot`; `BOOT_INTRO_FILM` in `config.h` picks one (0 A, 1 B, 2 C).
- **Plays on every boot** (power-on, reset, wake from deep sleep). Owner's choice over "cold power-on only". Risk (assumption): a crash loop replays the film each time; `FEATURE_BOOT_INTRO 0` switches it off.
- **A tap (Tap, DoubleTap, LongPress, Hold) skips** to the live-face step, which still holds for its own 0.5 s. The Screen module swallows every input while the film plays (`Module::swallowsInput`), so the skipping tap does not also reach the brain (no accidental pet or time board). IMU events during the film are ignored, not skips.
- **Alarm after the film.** Owner's choice. In practice nothing is late: the clock is unsynced for the first seconds after power-on (`WallTime.valid` is false until the first Wi-Fi sync), so no alarm can be due inside the 4.9 s window after a cold start (assumption for a reset with an already-synced clock: the time source reports valid only after a sync in this run, see `WifiTime::timeValid`). Timers, focus and stopwatch are not running at boot. This keeps "utility is exact" true.
- **Design rules kept:** whole 2x2-pixel cells, one colour, zoom only by whole-number pixel zoom (no blur), hold between moments, no flash faster than 3 per second (shortest hold 60 ms is a slide step, not a flash of the whole screen).

## How it is built
- `src/boot/boot_film.*`: frames as data (a step table per film) and one renderer (pure C++, desktop tests).
- `src/boot/intro_player.h`: the clock (cumulative schedule, stale steps skipped, skip to the last step); pure, tested.
- `Screen` owns the film while it plays (non-blocking, millis-driven), then restarts the normal clip. Replay with serial key `B`.
- The last frames are drawn by the face rig through a hook, so the handover to the home face is exact.

## Open / consequences
- Fact: 111 native tests pass, all envs build, flash 76.2% (was 75.6%; the film bitmaps and step tables cost about 7 KB).
- Not yet seen inside the real firmware on the board (the preview env was): `docs/boot-film-checklist.md`.
- "Every boot" includes wake from deep sleep; if that feels slow, switch to cold power-on only (needs the reset reason).
- The "face grows" frames (two) approximate the live eyes; the film's own dot face is a 2x2 hole at 4x4 px.
