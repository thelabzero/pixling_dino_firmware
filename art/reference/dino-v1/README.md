# Dino face sheet v1 (reference)
Ground truth, do not edit. Regenerate: `node tools/export_dino_reference.js && python3 tools/dino_sheet_png.py`.
| File | What |
|---|---|
| `Deskimon Dino Sheet.html` | The Claude Design sheet (source) |
| `rig.js` | Drawing code extracted from it; `build(time, friend)` returns every clip |
| `dino_frames.bin` | 416 frames, 128x64 1-bit, 1 byte per pixel |
| `dino_timing.json` | Per clip and frame: id, trigger, hold ms, note (time 10:24, friend SAM) |
| `dino_contact_R/L/U.png` | 3x contact sheets for the groups v1 ports (G and F are not ported: regenerate them with `dino_sheet_png.py`) |

NOTE: `rig.js` is the sheet's code **plus the owner-approved tweaks in `docs/decisions/004-owner-design-tweaks.md`**. The HTML is untouched.
