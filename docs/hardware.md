# Hardware

## Phase 1 bill of materials

| Part | Notes |
|---|---|
| ESP32-C3 SuperMini | Prototype. Production: ESP32-C3-MINI-1 module on a custom PCB with a 32.768 kHz crystal |
| SSD1306 0.96" OLED, 128×64, I²C | Address 0x3C |
| MPU6050 IMU | I²C address 0x68 (AD0 low); INT pin for wake-on-motion |
| TTP223 touch module | Digital output |
| Piezo buzzer (proposed, ADR 005) | Driven by PWM |
| Li-ion/LiPo + USB-C charging | Production target: single cell ~800–1200 mAh **[Estimate]** |

## Pinout (ESP32-C3 SuperMini)

| Signal | GPIO | Status |
|---|---|---|
| I²C SDA (OLED + IMU) | 8 | Confirmed (working sketch) |
| I²C SCL (OLED + IMU) | 9 | Confirmed (working sketch) |
| Touch OUT (HW-763, TTP223-type) | 4 | Wired 2026-10-25. GPIO0–5 can wake from deep sleep |
| Battery voltage sense | 3 (ADC1) | Wired 2026-10-25: Battery+ → 330k → GPIO3 → 330k → GND; battery − and ESP32 GND common |
| MPU6050 INT | 0 | Planned: must be GPIO0–5 |
| Piezo buzzer (passive) | 10 | Chosen 2026-10-25: GPIO10 → ~100 Ω (82 Ω fitted) → piezo → GND. No boot/JTAG/wake role. Louder: NPN transistor from 5 V |

**[Fact]** On the ESP32-C3 only GPIO0–5 can wake the chip from deep sleep. GPIO2, 8 and 9 are strapping pins: keep their pull-ups as the board has them.

## Battery (as given by the owner, 2026-11-09)

- Cell: 3.7 V, 450 mAh LiPo, marked **KP411525** (batch code given: R41166073). Charged through a charger module (blue light = full).
- **Unknown:** whether the cell has a protection board; the charger module's charge current (a TP4056 module defaults to about 1 A, over 2C for this cell: check it, about 0.5C = 225 mA is the usual safe choice). Cut-off about 3.0 V is the typical LiPo value (assumption: not read from a datasheet).
- Soak run 1 (idle on the home screen, 2026-11-09, `docs/soak-checklist.md`): **4 h 23 min** from a first reading of 3,943 mV to 2,811 mV (uncalibrated reading). Estimated average draw about 65-105 mA (estimate: capacity divided by runtime; no current sensor). The cell was run below 3.0 V: add a low-battery cutoff.

## Known hardware notes

- SuperMini has a power LED and LDO that waste current in deep sleep: battery numbers measured on it don't represent the product.
- OLED burn-in: keep the screen off when idle; drift static content.
- The C3 has no camera interface and no capacitive touch peripheral **[Fact]**: hence the external TTP223, and any camera module would need its own chip (ESP32-S3).

## Wiring status (update after each check)

| Date | Check | Result |
|---|---|---|
| 2026-10-18 | OLED on SDA 8 / SCL 9 | works (scene 4a played) |
| 2026-10-18 | MPU6050 on the same I²C bus | found at 0x68, WHO_AM_I 0x68. Calibrated with tools/imu_calibrator: Z offset ~0.3 g (large but corrected), gyro bias up to 6.4 °/s |
