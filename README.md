# dino

A small pixel dinosaur that lives on your desk, entirely in its face. It keeps your time, wakes you up, sits with you while you focus, and reacts when you pet, shake or pick it up. A Pixling.

No camera, no microphone, no cloud. The only network use is a daily Wi-Fi time sync.

- **How to use it (for owners):** open [`docs/how-to-use-dino.html`](docs/how-to-use-dino.html) in a browser.
- **How to build and flash it (this file).**
- **Status and known gaps:** [`docs/checkpoints.md`](docs/checkpoints.md). Project rules for contributors: [`CLAUDE.md`](CLAUDE.md).

Design source: `art/reference/dino-v1/` (50 clips, 416 frames; v1 ports the Face, Life and Utility groups, 32 clips / 251 frames).

---

## 1. What you need

| Part | Notes |
|---|---|
| ESP32-C3 SuperMini | The board that runs the firmware |
| SSD1306 0.96" OLED, 128x64, I2C | Address 0x3C |
| MPU6050 IMU | Address 0x68. Tilt, flip, shake, pick up |
| TTP223 touch module (HW-763) | Tap, double tap, long press, hold |
| Passive piezo buzzer | Sounds |
| Li-ion cell + two 330 kOhm resistors (optional) | Battery reading through a divider |
| USB-C data cable | A charge-only cable will not work |

Computer: macOS, Linux or Windows with **Python 3.9+** and **git**.

### Wiring

| Signal | ESP32-C3 pin |
|---|---|
| OLED + MPU6050 SDA | GPIO 8 |
| OLED + MPU6050 SCL | GPIO 9 |
| Touch OUT | GPIO 4 |
| Piezo (via about 100 Ohm to GND) | GPIO 10 |
| Battery divider middle | GPIO 3 |
| 3V3 / GND | to OLED, MPU6050, touch module |

The OLED and the IMU share the same two I2C wires. Full notes: [`docs/hardware.md`](docs/hardware.md). Pins are defined in `firmware/include/config.h`.

---

## 2. Get the code

```bash
git clone <your-repo-url> dino
cd dino
```

(No remote exists yet for this project: replace `<your-repo-url>` with the address you push it to.)

## 3. Install PlatformIO

Pick one.

**A. Command line, in a local virtual environment (recommended, nothing installed globally):**

```bash
cd firmware
python3 -m venv .venv
.venv/bin/pip install platformio pyserial pillow
```

Stay in `firmware/` for the rest of this guide and use `.venv/bin/pio` wherever it says `pio`. On Windows the path is `.venv\Scripts\pio.exe`.

**B. VS Code:** install the **PlatformIO IDE** extension, open the `firmware/` folder and use the toolbar (Build, Upload, Monitor).

The first build downloads the ESP32 toolchain and two display libraries, which takes a few minutes and needs internet.

## 4. Set your Wi-Fi and time zone

The clock and alarms need the real time, which the dino fetches from the internet over Wi-Fi (a 2.4 GHz network; the C3 has no 5 GHz).

```bash
cp include/secrets.example.h include/secrets.h
```

Edit `include/secrets.h`:

```c
#define WIFI_SSID "your-network-name"
#define WIFI_PASS "your-password"
```

`secrets.h` is git-ignored: never commit it.

Then check `include/config.h`, section 6: `TIME_TZ` is `"IST-5:30"` (India). Change it to your zone, for example `"GMT0"` or `"CET-1CEST,M3.5.0,M10.5.0/3"`.

**Without Wi-Fi the dino still runs, but the clock shows `--:--` and alarms never ring.** The time is synced once at start and then once a day.

## 5. Build

```bash
pio run -e dino_c3
```

Expect `[SUCCESS]`. This compiles only; nothing touches the board yet.

## 6. Flash (push the firmware to the board)

1. Plug the SuperMini into your computer with the USB-C cable.
2. Find the port: `pio device list`. On macOS it looks like `/dev/cu.usbmodem...`, on Linux `/dev/ttyACM0`, on Windows `COMx`.
3. Upload and watch the output:

```bash
pio run -e dino_c3 -t upload && pio device monitor -e dino_c3
```

The monitor runs at 115200 baud. Add `-f time` to put a time stamp on every line. Leave the dino sitting on its home screen and it shows its face.

**If upload fails or no port shows up:** put the board in download mode: hold the **BOOT** button, tap **RESET** (or plug the cable in while holding BOOT), release BOOT, then run the upload again. After a flash the port can change name: run `pio device list` again and reconnect the monitor.

## 7. First-run checklist

| You should see / hear | If not |
|---|---|
| The monitor lists each module and whether it was found | A missing module is reported and skipped; the dino keeps running without it |
| The dino's face on the OLED | Check SDA 8 / SCL 9 and address 0x3C |
| A tap on the touch pad gets a reaction | Check GPIO 4 and that the module outputs HIGH when touched (`TOUCH_ACTIVE_HIGH`) |
| Tilting the dino moves the menu | Check the IMU wiring; see the calibration step below |
| Short sounds on touch, menu and alarm | Check GPIO 10 and `BUZZER_VOLUME` |
| The clock shows the right time (long press, Clock, tap) | Wrong Wi-Fi details, a 5 GHz network, or `TIME_TZ` |

**No touch pad connected yet?** Everything can be driven from the Serial monitor: type `t` (tap), `d` (double tap), `l` (long press), `h` (hold), `,` and `.` (tilt left/right), `f` and `u` (flip down/up). Type `?` for the full key list and `i` for a status report.

To check that the alarm works end to end, type `a`: it sets an alarm for the start of the next minute.

## 8. Calibrate the IMU (once per unit)

`firmware/include/config.h` section 4 holds the calibration of the author's unit. Your IMU will have different offsets, so measure yours once it is mounted in its final position. The guided browser tool is in [`tools/imu_calibrator/`](tools/imu_calibrator/README.md) and the full guide is [`docs/imu-calibration.md`](docs/imu-calibration.md). In short:

```bash
pio run -e imu_calibrator -t upload
```

Close the serial monitor, open `tools/imu_calibrator/imu_calibrator.html` in Chrome or Edge (it uses Web Serial), follow the six poses, then paste the generated block over section 4 of `config.h`. Re-flash the main firmware with `pio run -e dino_c3 -t upload`.

## 9. Hardware check programs

Small stand-alone programs, one environment each:

| Env | Checks | Run |
|---|---|---|
| `play_dino` | Plays every dino clip in a loop and prints render time per clip | `pio run -e play_dino -t upload && pio device monitor -e play_dino` |
| `play_inputs` | I2C scan and live gesture counters on the screen | `pio run -e play_inputs -t upload && pio device monitor -e play_inputs` |
| `imu_calibrator` | Streams IMU data for the calibrator page | see section 8 |

## 10. Tuning

Everything tunable is in `firmware/include/config.h`: feature switches, pins, display brightness, gesture thresholds, touch timings, alarm and focus lengths, how fast the dino falls asleep. Change a value, rebuild, upload. Setting a `FEATURE_*` switch to 0 removes that module from the build.

## 11. For contributors: checks before you push code

Run from the repo root:

```bash
cd firmware && .venv/bin/pio test -e native && cd ..   # 65 desktop unit tests
python3 tools/face_check.py                            # every frame matches the design, double and float
node tools/imu_calibrator/test_page.js                 # calibrator page test
cd firmware && .venv/bin/pio run -e dino_c3            # chip build
```

Code layout and rules: [`firmware/README.md`](firmware/README.md) and [`firmware/CLAUDE.md`](firmware/CLAUDE.md). Decisions are in [`docs/decisions/`](docs/decisions/).

## 12. Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| `pio: command not found` | Use `.venv/bin/pio`, or install PlatformIO as in section 3 |
| Upload: "No serial data received" / port missing | Cable is charge-only, or the board needs download mode (section 6) |
| Monitor shows nothing | Board is on a new port name, or another monitor/calibrator page still holds the port |
| Clock `--:--`, alarm never rings | Wi-Fi not connected: check `secrets.h`, use 2.4 GHz, move closer. `WIFI_LOW_TX_POWER` is already on, which many SuperMini boards need |
| Wrong time by hours | `TIME_TZ` in `config.h` |
| Tilt is reversed or erratic | Re-run the IMU calibration with the IMU fixed in place |
| Reset loop in the monitor | Report the log. The known stack-overflow cause is fixed (loop stack is 16 KB, one clip is built at a time); keep a `Grid` (8 KB) off the stack in any new code |
| Dino reacts to nothing | Touch events: check wiring and `TOUCH_*` timings; or use the Serial keys in section 7 |

## 13. Known gaps (v1)

- Petting is a short tap; a firm double tap for RAWR is not wired yet.
- A held tilt can replay the tilt clip every 4 seconds.
- The shake threshold (1.7 g) is the inherited default and needs tuning on the board.
- Games and Friends (design groups G and F) are not built yet.
- The menu has 5 slots: Clock, Alarm, Focus, Stopwatch, Back.

More in [`docs/checkpoints.md`](docs/checkpoints.md).
