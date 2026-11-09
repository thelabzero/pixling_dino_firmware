<p align="center"><img src="art/readme/banner.svg" alt="dino, a Pixling: the pixel Dino next to the Pixling wordmark" width="640"></p>

# dino

A small pixel dinosaur that lives on your desk, entirely in its face. It keeps your time, wakes you up, sits with you while you focus, and reacts when you pet, shake or pick it up. A **Pixling**.

No camera, no microphone, no cloud. The only network use is a daily Wi-Fi time sync, plus a short-lived setup network of its own when you choose **Wi-Fi** in the menu.

| | |
|---|---|
| **Using it** | [Live owner's guide](https://thelabzero.github.io/pixling-dino-guide/) (animated, with the real screens) |
| **Building it** | this page |
| **Status and known gaps** | [`docs/checkpoints.md`](docs/checkpoints.md) |
| **Contributing** | [`CLAUDE.md`](CLAUDE.md), [`firmware/README.md`](firmware/README.md), decisions in [`docs/decisions/`](docs/decisions/) |
| **Design source** | `art/reference/dino-v1/` (the sheet; the firmware ports its Face, Life and Utility groups) |
| **Firmware** | 0.3.1. Face, life, clock, alarm, focus, stopwatch, boot film and soak log have run on the board. **Wi-Fi setup is code-complete but not yet run on the board** ([checklist](docs/wifi-setup-checklist.md)) |

**On this page:** [Parts](#1-parts-and-wiring) · [Quick start](#2-quick-start) · [Wi-Fi](#3-connect-to-wi-fi) · [First run](#4-first-run) · [Controls](#5-controls) · [Tuning](#6-tuning-and-calibration) · [Tools](#7-tools-and-checks) · [Troubleshooting](#8-troubleshooting) · [Known gaps](#9-known-gaps)

---

## 1. Parts and wiring

| Part | Notes | Pin (ESP32-C3) |
|---|---|---|
| ESP32-C3 SuperMini | Runs the firmware | |
| SSD1306 0.96" OLED, 128x64 | I2C, address 0x3C | SDA 8, SCL 9 |
| MPU6050 IMU | I2C, address 0x68. Tilt, flip, shake, pick up | same SDA / SCL |
| TTP223 touch module (HW-763) | Tap, double tap, long press, hold | GPIO 4 |
| Passive piezo buzzer | Sounds. Via about 100 Ohm to GND | GPIO 10 |
| Li-ion cell + two 330 kOhm resistors | Optional. Battery reading through a divider | GPIO 3 (divider middle) |
| USB-C **data** cable | A charge-only cable will not work | |

3V3 and GND go to the OLED, IMU and touch module. Full notes: [`docs/hardware.md`](docs/hardware.md). Pins live in `firmware/include/config.h`.

## 2. Quick start

You need **Python 3.9+** and **git** (macOS, Linux or Windows).

```bash
git clone https://github.com/thelabzero/pixling_dino_firmware.git dino
cd dino/firmware
python3 -m venv .venv
.venv/bin/pip install platformio pyserial pillow     # Windows: .venv\Scripts\pip.exe, pio.exe
.venv/bin/pio run -e dino_c3                         # build (first time downloads the toolchain)
.venv/bin/pio run -e dino_c3 -t upload && .venv/bin/pio device monitor -e dino_c3
```

The monitor runs at 115200 baud; add `-f time` for time stamps. Prefer VS Code? Install the **PlatformIO IDE** extension, open `firmware/` and use the toolbar.

<details>
<summary>Upload fails or no port shows up</summary>

Find the port with `pio device list` (`/dev/cu.usbmodem...` on macOS, `/dev/ttyACM0` on Linux, `COMx` on Windows). If the upload still fails, put the board in download mode: hold **BOOT**, tap **RESET** (or plug the cable in while holding BOOT), release BOOT, upload again. After a flash the port can change name.
</details>

## 3. Connect to Wi-Fi

The clock and alarms need the real time, fetched over Wi-Fi at start and then once a day. Use a **2.4 GHz** network (the C3 has no 5 GHz). Everything except the clock and alarm works without Wi-Fi: until it is connected once, the clock shows `--:--` and alarms never ring.

Set it up on the dino with your phone, no app and no rebuild:

1. **Long press** the pad, **tilt** to the **Wi-Fi** card, **tap**.
2. The dino shows a network name (`DINO-1234`) and a password (`XXXXX XXXXX`).
3. On your phone, join that network with that password. A page **Connect your Dino** opens by itself (or go to `http://192.168.4.1`).
4. Pick your home Wi-Fi, enter its password, tap **Connect**.
5. The dino shows **CONNECTED** with a heart, then the time. Your Wi-Fi is saved.

**Long press** cancels at any time. Nobody finishes within 5 minutes: it gives up. A wrong password gives a sad dino and the pairing screen returns. An alarm that comes due always wins.

<details>
<summary>Security, developer shortcut, time zone</summary>

- The setup network is WPA2 and its password is random every time and shown **only on the dino's screen**.
- Your Wi-Fi password is stored on the dino **as plain text in flash** (no flash encryption), only after a successful connect. Do not use a password you reuse for sensitive things.
- The phone may briefly lose the dino's network when the dino joins your router. The dino's screen is the source of truth.
- **Developers:** copy `firmware/include/secrets.example.h` to `secrets.h` and fill in `WIFI_SSID` / `WIFI_PASS` (git-ignored, never commit). A network saved through the menu wins.
- **Time zone:** `TIME_TZ` in `firmware/include/config.h`, section 6 (default `"IST-5:30"`). Setup timings: `WIFI_SETUP_*`, same section.
- Design: [ADR 005](docs/decisions/005-wifi-setup-portal.md).
</details>

## 4. First run

Every power-on plays the **boot film** (about 5 s): the name Pixling writes itself in pixels, the dot hops, the picture zooms into it, the dot becomes Dino, and the face grows into the live face. **A tap skips it** (and does not also pet the dino).

<p align="center"><img src="art/readme/moods.svg" alt="Dino's four faces in the boot film: eyes open, half blink, blink, happy" width="560"></p>

| You should see / hear | If not |
|---|---|
| Monitor lists each module and whether it was found | A missing module is skipped, the dino keeps running |
| Boot film, then Dino's face | Check SDA 8 / SCL 9 and address 0x3C |
| A tap on the pad gets a reaction | Check GPIO 4 and `TOUCH_ACTIVE_HIGH` |
| Tilting moves the menu | Check IMU wiring, then [calibrate](#6-tuning-and-calibration) |
| Sounds on touch, menu and alarm | Check GPIO 10 and `BUZZER_VOLUME` |
| Menu has 6 cards: Clock, Alarm, Focus, Stopwatch, **Wi-Fi**, Back | Rebuild and upload the current firmware |
| Right time on the Clock card | Not connected yet ([section 3](#3-connect-to-wi-fi)), 5 GHz network, or `TIME_TZ` |

**No touch pad yet?** Drive everything from the monitor: `t` tap, `d` double tap, `l` long press, `h` hold, `,` `.` tilt, `f` `u` flip, `a` alarm at the next minute (quick end-to-end alarm test), `i` status, `?` all keys.

<details>
<summary>Boot film options</summary>

- Replay: `B` in the monitor. Off or another film: `FEATURE_BOOT_INTRO` and `BOOT_INTRO_FILM` (0 born, 1 dino, 2 zoom) in `config.h`.
- Preview all three without the full firmware: `pio run -e play_boot -t upload && pio device monitor -e play_boot` (keys `1` `2` `3`, `a` all, `l` loop, `s` timing).
- Frames are data in `firmware/src/boot/`. It never delays an alarm: the clock is not synced yet right after power-on. See [ADR 006](docs/decisions/006-boot-intro-film.md) and [`docs/boot-film-checklist.md`](docs/boot-film-checklist.md).
</details>

## 5. Controls

| Do | What happens |
|---|---|
| Tap | Pet the dino. Chooses in menus. Snoozes a ringing alarm (5 min) |
| Double tap | Time board |
| Long press (1.5 s) | Opens the menu. Back / give up elsewhere |
| Hold (3 s) | Dismisses the alarm. Alarm off while setting it |
| Tilt | Moves the menu, changes alarm hours / minutes and focus length |
| Flip face-down | Dismisses the alarm. On the Focus screen, starts the session |
| Shake, pick up | The dino reacts |
| Left alone 5 min | The dino falls asleep and the screen dims. Any touch wakes it |

Set alarm starts at the current time: tilt = hour, tap, tilt = minutes (5-minute steps), tap = save. Focus offers 25, 45 or 60 minutes, then a 5-minute break. More in the [live guide](https://thelabzero.github.io/pixling-dino-guide/).

## 6. Tuning and calibration

**Everything tunable is in `firmware/include/config.h`**: `FEATURE_*` switches (0 = not built at all), pins, brightness, gesture and touch timings, alarm and focus lengths, how fast the dino sleeps. Change, rebuild, upload.

**Calibrate the IMU once per unit** (the file holds the author's unit). Mount it in its final position, then:

```bash
pio run -e imu_calibrator -t upload
```

Close the monitor, open `tools/imu_calibrator/imu_calibrator.html` in Chrome or Edge (Web Serial), follow the six poses, paste the result over section 4 of `config.h`, re-flash `dino_c3`. Guide: [`docs/imu-calibration.md`](docs/imu-calibration.md).

## 7. Tools and checks

**Hardware check programs** (one env each: `pio run -e <env> -t upload && pio device monitor -e <env>`):

| Env | Checks |
|---|---|
| `play_dino` | Every dino clip in a loop, with render time |
| `play_inputs` | I2C scan and live gesture counters |
| `play_boot` | The three boot films |
| `play_shard` | The shard-face expressions (experiment) |

**Soak log** (battery tests and long runs). The dino keeps its own log in flash: a health sample a minute (battery, memory, loop timing, Wi-Fi) and events (boots with reset reason, low battery, stalls). It survives resets and a dead battery. Monitor: `w` prints it, `x` twice erases it.

```bash
firmware/.venv/bin/python tools/soak_dump.py             # fetch the log over USB into soak_logs/
python3 tools/soak_report.py soak_logs/<saved file>      # runs, resets, battery curve, memory trend
```

Procedure: [`docs/soak-checklist.md`](docs/soak-checklist.md), design: [ADR 007](docs/decisions/007-soak-log.md). Battery voltage is uncalibrated until `BATTERY_CAL` is set with a multimeter.

**Before you push code** (from the repo root):

```bash
cd firmware && .venv/bin/pio test -e native && cd ..   # 120 desktop unit tests
python3 tools/face_check.py                            # every frame matches the design
node tools/imu_calibrator/test_page.js                 # calibrator page test
cd firmware && .venv/bin/pio run -e dino_c3            # chip build
```

## 8. Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| `pio: command not found` | Use `.venv/bin/pio` |
| "No serial data received" / port missing | Charge-only cable, or download mode ([section 2](#2-quick-start)) |
| Monitor shows nothing | New port name, or another monitor or the calibrator page holds the port |
| Clock `--:--`, alarm never rings | Wi-Fi not set up: [section 3](#3-connect-to-wi-fi). 2.4 GHz only, move closer. Type `i` to see if a network is saved |
| No `DINO-1234` in the phone's list | Wait a few seconds; look for `[wifi] setup network ... is up` in the monitor; leave the screen (long press) and start again |
| Joined `DINO-1234` but no setup page | Go to `http://192.168.4.1`; turn mobile data off for a moment |
| "NO LUCK / CHECK THE PASSWORD" | Wrong password, 5 GHz only, or out of range. Try again |
| Page never says "Connected" but the dino does | Expected: trust the dino's screen |
| Wrong time by hours | `TIME_TZ` in `config.h` |
| Tilt reversed or erratic | Re-run the IMU calibration with the IMU fixed in place |
| Boot film plays on every reset | Intended. Tap to skip, or `FEATURE_BOOT_INTRO 0`. If it repeats by itself the board is resetting: check the reset reason in the first monitor lines (`[soak] log: ... reset reason ...`) |
| Reset loop | Report the log. Keep any `Grid` (8 KB) off the stack in new code (loop stack is 16 KB) |
| Dino reacts to nothing | Check wiring and `TOUCH_*` timings, or use the monitor keys |

## 9. Known gaps

- Petting is a short tap; a firm double tap for RAWR is not wired.
- A held tilt can replay the tilt clip every 4 s. The shake threshold (1.7 g) needs tuning on the board.
- Games and Friends (design groups G and F) are not built. The menu has 6 slots.
- **Wi-Fi setup has not been run on the board** ([checklist](docs/wifi-setup-checklist.md)). Its screens and icon are drawn in code, not in the design sheet.
- Open from the first board run ([`docs/d4-board-checklist.md`](docs/d4-board-checklist.md)): alarm hold-dismiss, alarm kept after a reset, how the clips compare with the sheet. The monitor can show a line late (USB serial holds a short last line); the loop is not stalled.

Full status: [`docs/checkpoints.md`](docs/checkpoints.md).
