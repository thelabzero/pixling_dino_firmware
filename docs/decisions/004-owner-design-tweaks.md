# 004 · Owner-approved tweaks to the design sheet
**Status:** accepted (owner). The sheet (`art/reference/dino-v1/Deskimon Dino Sheet.html`) stays as delivered. Where the owner changes a
clip, the same edit is made in `art/reference/dino-v1/rig.js` and in `firmware/src/scenes/dino_clips.cpp`, the reference is
regenerated (`node tools/export_dino_reference.js && python3 tools/dino_sheet_png.py`) and `tools/face_check.py` must still match.
So `rig.js` is "sheet + these tweaks", not a copy of the sheet.

| Date | Clip | Change | Why (seen on the OLED) |
|---|---|---|---|
| 2026-10-30 | U12 Breathing break | Eyes at 35 px no longer rise to y 24 and the brow no longer lifts: frames use y 27 / 26 / 26 / 26 and brow lift 0 | Brows were cut off at the top of the screen in the IN and HOLD frames |
| 2026-10-30 | U4 Menu | Small face 1 px left (x 23, all menu frames incl. tilt slide/swap/pop); the `<` arrow 2 px right (x 52) | Eyes touched the `<` arrow; wanted a small gap between face and card |

Not done (note): the Friends heart and Games sparkle effects still sit close to the `<` arrow. Games picker (G, not ported yet) shares the arrow code in the sheet and should get the same x 23 face when it is ported.
