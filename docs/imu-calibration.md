# IMU calibration guide (MPU6050)

How to measure the IMU's errors, load the values into the firmware, and check that gestures work.
Do this **after the IMU is rigidly mounted in its final position**: any re-mounting means calibrating again.

**Status (2026-10-18):** the values in `config.h` are **provisional**. They were measured while the IMU was not rigidly fixed. Redo everything below once the mounting is final (plan step 2 is paused until then).

---

## 1. Why calibrate (what we measured)

| Error | This unit (provisional) | Datasheet typical | Effect if not corrected |
|---|---|---|---|
| Accel Z offset | +0.30 g | ±0.08 g | Standing still reads 1.30 g, so "resting" is never detected: pick up / put down never fire, flip is unreliable |
| Accel X/Y offset | +0.03 / −0.003 g | ±0.05 g | Tilt angles off by 1–2° |
| Accel scale | 0.99–1.01 | ±3% | Small; tilt angles slightly off |
| Gyro bias | −1.5 / +2.9 / −6.4 °/s | ±20 °/s | The dino "feels" itself turning when still; bias also changes with temperature |
| Mounting | chip flat, screen faces sensor −Y, turned ~5° | — | Tilt left/right and flip need to know which way is up and front |

The firmware also **re-learns the gyro bias** whenever the dino has been still for 1.5 s (`input/imu_calibration`), so warm-up drift is handled. The accel values and mounting are fixed and come only from this procedure.

---

## 2. Before you start: mounting checklist

- [ ] IMU board glued or screwed **flat** to the body (hot glue under the board, or double-sided foam tape). Pressing on it with a finger must not move it.
- [ ] Wires to the IMU have slack and are tied down: a pulled wire must not tilt the board.
- [ ] Final position: the same place it will stay in the enclosure. Moving it later = calibrate again.
- [ ] Power on for **2 minutes** before step 1 (gyro bias changes while the chip warms up).
- [ ] A flat, steady table, plus a book or box to prop the device on its sides.

---

## 3. Tools

Everything is in `tools/imu_calibrator/`:

| File | What |
|---|---|
| `firmware/src/players/imu_stream.cpp` (env `imu_calibrator`) | Small PlatformIO program: streams raw IMU data over USB at 100 Hz. No libraries. Pins from `config.h`. |
| `imu_calibrator.html` | Browser page: live view, the three calibration steps, generates the values. Needs **Chrome or Edge** (Web Serial). Has a Demo button to try it without hardware. |

---

## 4. Procedure

### 4.1 Flash the stream firmware
1. From `firmware/`: `pio run -e imu_calibrator -t upload` (the env sets up USB serial; nothing to configure).
2. Pins and I2C speed come from `include/config.h` (`PIN_I2C_SDA 8`, `PIN_I2C_SCL 9`).
3. **Close the Serial Monitor** and any `pio device monitor`: only one program can use the port.

### 4.2 Connect the page
1. Open `imu_calibrator.html` in Chrome or Edge → **Connect** → choose the ESP32-C3 port (`usbmodem…` on Mac).
2. Board messages should show the I2C scan (`0x3C`, `0x68`) and `MPU at 0x68, WHO_AM_I 0x68`.
3. Rate (top of the Live card) about **100 Hz**. Move the device: the 3D board and graphs follow.

### 4.3 Step 1: gyro bias (3 s)
- Device on the table, hands off. Press **Capture gyro bias**.
- If it says "moved during capture", wait and retry (don't touch the table or cable).

### 4.4 Step 2: accelerometer, 6 positions (2 s each)
Each axis (X, Y, Z) must point straight up once and straight down once.

**How to know which button:** rest the device in a position, look at **Live → accel**. One axis reads about **+1** or **−1**, the others about 0. Press that button (≈ +1 on Y → **+Y up**; ≈ −1 on Z → **−Z up**).

For the current jar build (IMU flat on top, OLED on the front):

| Device position | Expected |
|---|---|
| Standing normally | +Z up |
| Upside down (hold gently, lid on the table) | −Z up |
| Lying on its front (OLED on the table) | one of ±Y |
| Lying on its back | the opposite ±Y |
| On its left side | one of ±X |
| On its right side | the opposite ±X |

Prop it so it can't roll. The page rejects a capture if it moved or if the wrong axis is up. A warning "N° off vertical" above 5° means: sit it squarer and redo that one.

### 4.5 Step 3: mounting (1.5 s each)
- **(a)** device standing normally, as it sits on a desk.
- **(b)** device lying **face-down** (screen towards the table).

### 4.6 Check quality (Quality card, and Live with "show calibrated values" ticked)

| Check | Good | If not |
|---|---|---|
| \|accel\| at rest, any position | 1.000 ± 0.010 g | Redo step 2; check the device was still and square |
| Max error over the 6 poses | < 10 mg | Redo the worst pose |
| Gyro at rest (calibrated) | within ±0.3 °/s | Redo step 1 after warm-up |
| Device tilt reading when upright | about 0° | Redo step 3 (a) |
| Tilting the top to the right | "device tilt" goes **+** | Redo step 3; check (b) was really face-down |

### 4.7 Save
- **Save JSON** → store as `tools/imu_calibrator/results/<YYYY-MM-DD>_<unit>.json` and commit it, so every calibration has a record.
- **Copy for config.h** for the next section.

---

## 5. Load the values into the firmware

The Result box already holds the exact `#define` lines. Press **Copy for config.h** and paste them over the
`IMU_ACC_OFFSET_*`, `IMU_ACC_SCALE_*`, `IMU_GYRO_BIAS_*`, `IMU_UP_*`, `IMU_FRONT_*` lines in `firmware/include/config.h`
(section 4). Same names, same layout, so it is a straight replace. A step you have not done yet comes out as a comment
("keep the existing ... lines"), so a partial run never overwrites values you did not measure.
(`IMU_RIGHT` is not needed: the firmware computes it as up x front. The gyro bias is only a starting value; the firmware keeps learning it.)

Then update the comment above the values with the date and "final mounting", re-flash the normal firmware (`pio run -e dino_c3 -t upload`) and commit.

How the firmware uses them:
```
MPU6050 raw → drivers/mpu6050 (counts → g, °/s)
            → input/imu_calibration  (offset, scale, gyro bias + re-learning while still)
            → input/imu_gestures     (up/front → tilt, flip, pick up, put down, shake)
```

---

## 6. Verify on the device (`play_inputs`)

```
cd firmware
pio run -e play_inputs -t upload
pio device monitor -e play_inputs
```

1. Type `p` (raw stream). Device still and upright:
   - `|a|` ≈ 1.000
   - gyro ≈ 0.0
   - roll and pitch ≈ 0
   - after 1.5 s the line shows `(learning)`
2. OLED state shows **RESTING** after about 1 s on the table.
3. Level bubble: tilting the top right moves the dot right.

Type `p` again to stop the stream.

---

## 7. Gesture test (20 tries each)

Rules so the counts mean something:
- One gesture type at a time. Type `r` to reset counters before each type.
- After each try, return to normal and **pause 1 s**.
- **Pick up:** wait until the OLED says **RESTING** before lifting. **Put down:** hold still in the hand first, then place it and leave it 1 s.
- **Tilt:** tilt about 30°, then come back. Holding a tilt longer than 0.7 s repeats the event (menu scrolling); don't count those repeats. (Planned: the log will tag them `(repeat)`.)
- **Shake:** short shake, stop, wait 2 s. Continuous shaking currently fires every ~2 s. (Planned: one event per shaking session.)
- **False triggers:** leave it 10 min on the desk while typing next to it. Expected: nothing.

Record results in `docs/checkpoints.md` (Input checks table). Pass = at least 18/20 each and no false triggers.

**First run, 2026-10-18 (IMU not rigid, provisional):**
- flip: 20/20 clean
- tilt and shake: detected, but counts inflated by repeats
- pick up / put down: too few tries to judge

---

## 8. When to calibrate again

- The IMU or the screen is moved, re-glued or replaced.
- A new enclosure or board revision.
- \|accel\| at rest is no longer within 1.000 ± 0.010 g.
- Every production unit (product: this becomes a factory step; tracked for C5).

## 9. Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Page: no data after Connect | Monitor still open, or the wrong firmware is flashed | Close other monitors; re-flash with `pio run -e imu_calibrator -t upload` |
| `MPU6050 not found` | Wiring | VCC, GND, SDA → 8, SCL → 9; check the scan |
| WHO_AM_I not 0x68 | Clone or MPU6500-family chip | Usually fine; the values still work |
| "Moved during capture" every time | Table vibration, cable pulling, IMU loose | Steady surface, slack cable, fix the IMU rigidly |
| Big accel offset (> 0.2 g) | Chip stress (solder or mounting) or loose board | Calibration corrects it; if it changes between runs, the board is moving |
| Gestures fire on their own | Thresholds too low, or loose IMU | Fix mounting first, then tune `GestureConfig` in `input/imu_gestures.h` |
