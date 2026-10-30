# Checkpoints (dino)

| # | Checkpoint | Done when | Status | Date |
|---|---|---|---|---|
| D0 | Repo and firmware base | Copy builds; native tests pass | done (56/56 tests) | 2026-10-29 |
| D1 | Reference locked | Frames + timing exported from the sheet's rig | done (50 clips, 416 frames) | 2026-10-29 |
| D2 | Face rig in C++ | R, L, U frames match the reference 100% | done: 32 clips, 251 frames match in double and float (`python3 tools/face_check.py`). G and F frames come with D6/D7 | 2026-10-29 |
| D3 | Scenes as data | Clips play on desktop; ms per frame measured on the C3 | done on hardware 2026-10-30: 32 clips, 2 loops, no resets. Render 10-216 ms/frame (see `docs/debug-play-dino.md`); pre-render/bake needed in D4 | 2026-10-30 |
| D4 | Brain and modules | Dino life tables, sounds, utility menu, tests | code complete 2026-10-30, not yet run on the board: render/stack fixed, dino sounds (heard OK), dino life brain (16 tests), live utility screens (18 clips) and pipelined Screen module wired in; 65 native tests pass, chip build 69.6% flash | |
| D5 | Hardware check and soak | Gestures 18/20, buzzer, battery, 24 h soak | todo | |
| D6 | Games | Meteor dodge, Snack snap first | todo | |
| D7 | Friends | ESP-NOW pairing, phrases, 2P games | todo | |

## Known risks

- Face rig render cost on the C3 is unmeasured (measured later: 8-216 ms/frame, see D3). Fallback: pre-bake 416 frames (~416 KB).

## Known gaps after D4 (see also docs/debug-play-dino.md)
- Pet = short Tap (the sheet says hold 0.6 s: no such touch event yet). Firm double tap -> RAWR not wired (DoubleTap shows the time).
- Tilt "level" event missing: a held tilt can replay the tilt clip every 4 s.
- Shake threshold is the inherited default (1.7 g, 3 peaks / 1 s); the sheet says 1.2 g, 3 jerks / 600 ms. Tune `GESTURE_SHAKE_*` on the board.
- Voice timing is counted from when the home clip is built, not from what the OLED shows.
- A frame render (up to ~100-200 ms for glasses frames) runs inside the loop and can delay the alarm tick by that much.
- Menu has 5 slots (Clock, Alarm, Focus, Stopwatch, Back); Games/Friends/Messages/Settings come later. Stopwatch icon is drawn in code (not in the sheet).
