# Dino: project guide for Claude

Read this first in every session. Keep it short and current: update "Where we are" when a checkpoint changes.

## Where we are

- **Brand:** Pixling (characters made of pixels). Dino is a Pixling, a small desk dinosaur that lives entirely in its face. The design sheet still says "Deskimon" (old brand, dropped): use "dino".
- **Origin:** standalone side project. Firmware, tools and tests share one base (ADR 003); the character layer (face, scenes, life, sounds) is dino's own.
- **Phase:** v1 = Face + Life + Utility (sheet groups R, L, U). Games (G) and Friends (F) come later.
- **Checkpoint:** D0-D3 done. D4 code complete (dino face, life brain, live utility screens, sounds, pipelined screen), 65 native tests + face check pass, chip build ok; NOT yet run on the board (flash `dino_c3`). Known gaps and next steps: `docs/checkpoints.md`.
- **Design ground truth:** `art/reference/dino-v1/` (the sheet and its exported frames). Never hand-edit.
- **Last updated:** 2026-10-29

## What Dino is

A silent pixel dino on a 0.96" OLED. Two slab eyes with a cut-out pupil, brow ridges, nostrils, one fang. No body, no room. On utility screens the face shrinks (scale 0.55, x24 y31) and content sits beside it. No camera, no mic, no cloud.

## Hardware

ESP32-C3 SuperMini, SSD1306 128x64 (I2C SDA 8 / SCL 9, 0x3C), MPU6050, TTP223 touch GPIO4, piezo GPIO10, battery divider GPIO3. Pinout: `docs/hardware.md`.

## Map of the repo

| Path | What |
|---|---|
| `firmware/src/face/`, `firmware/src/scenes/dino_clips.*` | Dino face rig and clips (ported from the sheet). Checked by `tools/face_check.py`. |
| `firmware/` | PlatformIO project; read `firmware/README.md` and `firmware/CLAUDE.md`. |
| `tools/` | Face check, asset generators, IMU calibrator. |
| `art/reference/dino-v1/` | The design sheet and exported reference frames. |
| `docs/` | Ground rules, checkpoints, decisions. |

## Rules for every change

1. Check `docs/ground-rules.md` first. 2. Reuse the existing structure; add, don't rewrite. 3. Pure logic in plain C++ with tests (`pio test -e native`). 4. Any change in `face/` or `scenes/` must pass `tools/face_check.py`. 5. Record decisions in `docs/decisions/`. 6. Update `docs/checkpoints.md`. 7. Label claims: fact, estimate, assumption.

## Commands

`firmware/.venv/bin/pio` works if `pio` is not installed. `pio test -e native` for tests; `pio run -t upload && pio device monitor` for the board.
