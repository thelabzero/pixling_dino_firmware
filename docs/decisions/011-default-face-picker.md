# ADR 011: Choose dino's default face (menu > Face)

Status: code complete 2026-11-12 (both firmwares, `dino_c3` and `dino_wedge_c3`); NOT yet run on the board.

## Why
The owner wants to set the Pixling's default face on the device itself: long press, a menu card with a face icon, tilt through
faces, tap to choose, a warning that it becomes the default, tap again to fix it.

## Decisions
- **New menu card "FACE"** between Wi-Fi and Back (slot 5; Back moves to slot 6, `MENU_SLOTS` 7). Icon: a small face in a rounded
  box, drawn in code like the stopwatch and Wi-Fi icons.
- **Flow** (brain: `utility/machine.*`, pure C++, 8 new tests): card tap -> picker opens on the saved default -> tilt = next / previous
  face (wraps, "ear flick" shift) -> tap = warning "MAKE THIS YOUR DEFAULT? TAP AGAIN: YES / HOLD: NO" -> tap = saved (happy clip
  "NEW DEFAULT FACE SAVED", the happy warble) and home with the new face; hold = back to the picker; long press / hold in the picker =
  leave without changes; 10 s untouched = give up without changes (like the other menus). Tapping the face that already is the default
  just goes home (nothing to confirm). An alarm always wins (utility is exact).
- **What "default face" means:** the face dino rests on. `face::resolve(Ex::Idle)` now draws the chosen preset (`setDefaultFace`), so the
  home idle, its blinks and every utility screen that used the idle face follow it. All other presets (happy, sleepy, rawr...) stay exact,
  and the picker draws its candidates with `resolveExact`. Until a face is chosen the default is Idle itself, so `face_check.py` and
  `face_check_wedge.py` are unchanged (251/251 on both).
- **10 faces on offer** (`scenes::FACES`): CALM (the classic idle), HAPPY, MISCHIEF, SMUG, CURIOUS, SLEEPY, BORED (unimpressed), DETERMINED,
  SURPRISED, SHY. Chosen as calm resting faces: none has a looping effect (steam, tears, sparkles, glasses) and all blink. Other presets
  stay reachable only through the moods that use them. Assumption: the owner can add or drop faces by editing that table and
  `Machine::FACE_CHOICES` (a static_assert keeps them equal).
- **Saved in flash** by a new `FaceStore` module (NVS namespace "dino", key `face`, one byte = index), written only when the user confirms;
  `FEATURE_FACE_STORE` in `config.h` (0 = the choice lasts until reset). An out-of-range stored value falls back to 0.
- **New states go last** in the brain's state enum (`face_pick`, `face_confirm`, `face_done`), so soak-log numbers 0-16 keep their meaning;
  the soak dump header now lists 20 states.
- Same picker in both firmwares; it draws whichever face style that firmware has. The boot film is not affected (it never used the default face).

## Checked (2026-11-12)
Fact: 140 native tests pass (35 in the utility machine, 8 new); `face_check.py` and `face_check_wedge.py` 251/251; `dino_c3`, `dino_wedge_c3`,
`play_dino`, `play_dino_wedge`, `play_boot` build with no warnings; flash 81.8 %, RAM 21.6 %. I rendered the picker, warning and saved
screens for all 10 faces in both styles on the desktop and checked nothing touches the text.

## Not checked
On the board: the tilt feel, whether a face looks right on the real panel at 0.65, and that the saved face survives a power cut.
Steps: long press > tilt to FACE > tap > tilt through the faces > tap > "make this your default?" > tap again > unplug and plug in.
