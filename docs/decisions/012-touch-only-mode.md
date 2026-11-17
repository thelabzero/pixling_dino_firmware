# ADR 012: Touch-only mode (no IMU; the menu is a list)

Status: code complete 2026-11-17, updated 2026-11-18 (zoomed 3-row menu, bigger clock, multi-tap) (envs `dino_touch_c3`, `dino_touch_wedge_c3`); NOT yet run on the board.

## Why
Use the whole dino with the touch pad alone: no tilting. Same functions (menu, alarm, focus, stopwatch, Wi-Fi, default face).

## Decisions
- **Tap and long tap:** tap = next, long tap = select. Hold does not exist in this mode: the touch timings in `config.h` (`TOUCH_ONLY`)
  move the hold out of reach (60 s); long tap = released after 700 ms, any length. The brain also maps a `Hold` (typed in the Serial
  monitor) to a long press. Fact: this removes the old 1.5-3 s window.
- **Taps are counted on the home screen only** (`TouchGestures::setGapMs`, set by the touch module from the brain's view each loop):
  on home the gap for the next tap is 350 ms (`TOUCH_MULTI_GAP_MS`) and up to 4 taps are counted (`TOUCH_MAX_TAPS`); in every menu the gap is 0,
  so a tap acts at once. Events `TripleTap`, `QuadTap` were added at the END of the event enum (numbers in older logs keep their meaning).
  The 4th tap is sent without waiting (nothing can follow it), so the wait only shows for 1-3 taps: a single tap (pet) answers about 0.35 s
  later than before on the home screen (estimate: tap length + 350 ms).
- **Home screen taps:** 1 = pet (unchanged), 2 = time board (unchanged), **3 = cute** (shy, heart eyes with a hop, wink; purr and trill),
  **4 = angry** (frown, grumpy with steam, shaking, sulk; growl). Brain: `life::Act::Cute / Angry`, `onCute / onAngry` (reaction pause, wake a
  sleeping dino first, never cut a playing clip short, not counted as petting). Frames come from `scenes::buildMood` (new frames of existing
  presets Shy / Love / Wink / Determined / Grumpy / Unimpressed): not in the design sheet, so `face_check.py` does not compare them.
- **Build flag, not a menu setting:** `-DTOUCH_ONLY=1` (envs above). It sets `FEATURE_IMU 0`, so no tilt, flip, pick-up or shake
  (assumption: the home screen loses its pick-up / shake reactions in this mode; tap, long tap, sleep and the rest stay).
- **Brain (`utility/machine.*`, pure C++, 10 new tests):** `Settings::touchOnly` and `Machine::translate()` rewrite the events before
  the normal logic runs, so nothing else changed. Menu, face picker, focus pick, set alarm: tap -> "next", long tap -> "select".
  Default-face warning: the keys swap (long tap = yes, tap = no). While ringing: tap = snooze, long tap = dismiss.
  Home, Wi-Fi, focus, break, stopwatch: unchanged (long tap leaves, as before).
- **Menu = a zoomed list, 3 rows at a time** (`Clip::Menu` / `MenuSwap` drawn by `menuList` / `menuListSwap` when `View::touchOnly`): 7 rows
  (Clock, Alarm, Focus, Stopwatch, Wi-Fi, Face, Back) in 2x text, the marked row inverted, no face, a scroll bar on the right edge. The window
  follows the mark (row 0 at the top, Back at the bottom, the others in the middle with a neighbour on each side) and scrolls in 4 quick steps
  (35 ms each). A tap during a scroll finishes it at once, so quick taps are never lost; a long tap during a scroll picks the row it scrolls to.
  The Clock row shows ON / OFF, the Alarm row its time or OFF (both in small text so label and value never collide).
- **Bigger clock (clock mode, both firmwares):** the HUD clock is the 3x5 digits drawn at 2x, 34 x 10 px (`hud::CLOCK_*`), bottom-right.
  Fact (desktop render of every home clip plus the two new reactions): nothing below row 46 reaches past x = 77-81, the clock starts at x = 90.
- **Set alarm has a third step** (touch-only): hours (tap +1) -> minutes (tap +5) -> ON / OFF (tap flips, long tap confirms). OFF
  replaces "hold = alarm off". Nothing is saved before that last step.
- **Lost in this mode:** the cards menu, and tilt / flip / pick-up / shake. Double tap (the time board) stays, on the home screen.
- **Known gaps:** the focus picker has no Back row (it gives up after 10 s, like every menu); the face picker leaves by choosing the
  current default. Alarm hours need up to 23 taps. Taps inside a menu are not counted, so the cute / angry reactions exist on the home screen only.
- Face check unchanged (251/251 both sheets): the list and the on/off step are new live clips, not design-sheet clips.
