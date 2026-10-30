# play_dino bring-up: crash analysis and results (2026-10-30)

## Crash: root cause
`TG1WDT_SYS_RST` right after the banner, repeating. Cause: **stack overflow in the loop task.**
`scenes::buildDino()` had a 59,360 B stack frame (measured with `riscv32-esp-elf-g++ -fstack-usage`).
It built every clip's frames (hundreds of FaceO/FaceP temporaries; -Os does not reuse the slots) and only then
dropped the ones not asked for. The Arduino loop task has 8 KB. Saved PC pointed into `_panic_handler`: it panicked,
and the watchdog fired while the panic tried to print over USB CDC (hence no backtrace).
Desktop never showed it (8 MB stack).

## Wrong first guess (kept for the record)
First fix moved three 8 KB Grids to the heap and raised the stack to 16 KB. Real, but not the main cause: the
frame was still 25 KB after only-the-requested-clip building.

## Fixes applied
1. `buildDino(..., onlyKey)` builds only the requested clip (59 KB -> 25 KB frame); FaceO passed by reference.
2. Big grids on the heap (`clockshift`).
3. Player: `SET_LOOP_TASK_STACK_SIZE(48 * 1024)`.

## Hardware results (ESP32-C3, float rig, 2 full loops, 0 resets, 0 late frames)
- Render time per frame: typical 10-60 ms; heaviest: `focus` avg 163 ms (glasses), `focusdone` worst 216 ms, `expressions` worst 169 ms,
  `shake` worst 72 ms. ~100x slower than the desktop (50-70 us). Chip is fine for a 1-2 fps utility UI, **not** for rendering live inside a 60-150 ms frame.
- Stack high-water: ~37 KB of 48 KB used (10.6 KB left) while building the biggest clips. 25 KB estimate was low (printf + render add).
- Heap: min free 172 KB (`clockshift`), otherwise 240-256 KB.
- "late 0" is misleading: the player renders, then waits the hold. A 60 ms blink with a 35 ms render actually lasts ~95 ms.

## Open issues and fix plan
| # | Issue | Fix | Where |
|---|---|---|---|
| 1 | buildDino frame 25 KB, ~37 KB peak | Split into one function per clip (or build frames lazily from a table of FaceO), then run with default 8 KB stack | D4 |
| 2 | Render 10-216 ms | Pre-render the next frame while the current one is held (double buffer); for blinks/short holds (<=150 ms) pre-bake; consider caching the face layer; heaviest cost is per-pixel `rr()`/`area()` over full eye boxes (float software maths) | D4 |
| 3 | Glasses frames 163-216 ms | Draw glasses with integer line/rect code (same pixels) or pre-bake the 4 focus frames | D4 |
| 4 | Pupil edge needed double maths (4 frames differ in float) | Already handled; keep an eye on it if face.cpp maths is optimised (re-run face_check) | done |
| 5 | Panic output lost over USB CDC | Add `-DCORE_DEBUG_LEVEL`/`esp32_exception_decoder` filter in monitor; keep `-fstack-usage` check in the workflow | tooling |
| 6 | Visual match on the real OLED not checked by software | Owner to eyeball the clips against `art/reference/dino-v1/dino_contact_*.png` | owner |

## Run 2 (2026-10-30, owner): pipelined player, default 8 KB stack, builder split per clip
- 2 passes, no resets. Builder worst stack frame 59 KB -> 1.8 KB (split per clip, frames pushed one by one, in-place FaceO setters).
- Render 8-216 ms; `display()` 25 ms (I2C). 31 of 32 clips `late 0`.
- Late: `focus` (render avg 173 ms, blink frame holds 160 ms) and `focuspick` (one 150 ms frame, render 169 ms). Cause: glasses outline `edge()` tested each pixel 5x.
- Stack headroom on 8 KB: min 1296 B (menu clip). Too thin for the real firmware: use `SET_LOOP_TASK_STACK_SIZE(16 * 1024)` in main.
- Fix applied (not yet measured on the board): `ring()/edge()` test each pixel once with rolling rows (`borderPass` in face.h), identical pixels (face_check 251/251).
