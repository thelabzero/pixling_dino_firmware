# Tools guide

Python 3 + Pillow, Node for the sheet exporters. Run from the repo root.

| Tool | Does |
|---|---|
| `face_check.py` | Builds the face rig on the desktop (`face_native.cpp`, `live_native.cpp`) and compares every frame with the reference in `art/reference/dino-v1/`, in double and float. Run after any change in `firmware/src/face/` or `scenes/` |
| `export_dino_reference.js`, `dino_sheet_png.py` | Export frames, timing and contact sheets from the sheet's rig (`art/reference/dino-v1/rig.js`) |
| `gen_dino_assets.js` | Fonts and icons from the sheet → `firmware/src/face/face_assets.h` (generated: don't edit by hand) |
| `sound_preview.cpp` | Renders the dino sounds to WAV files for listening |
| `imu_calibrator/` | Browser IMU calibrator (Web Serial) + `test_page.js` (guide: `docs/imu-calibration.md`) |

Keep tools small, documented at the top of the file, and deterministic (same input → same output).
