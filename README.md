# dino

A small pixel dinosaur that lives on your desk, entirely in its face. It keeps your time, wakes you up, sits with you while you focus, and reacts when you pet, shake or pick it up. A Pixling.

No camera, no microphone, no cloud. The only network use is a daily Wi-Fi time sync (and a short-lived setup network of its own when you choose **Wi-Fi** in the menu, see section 4).

- **How to use it (for owners):** [**open the live guide**](https://thelabzero.github.io/pixling-dino-guide/) (animated, with the dino's real screens).
- **How to build and flash it (this file).**
- **Status and known gaps:** [`docs/checkpoints.md`](docs/checkpoints.md). Project rules for contributors: [`CLAUDE.md`](CLAUDE.md).
- **Firmware 0.3.0.** Face, life, clock, alarm, focus and stopwatch have run on the board ([`docs/d4-board-checklist.md`](docs/d4-board-checklist.md)). **Wi-Fi setup ("connect your dino") is code-complete and tested on the desktop but not yet run on the board** ([`docs/wifi-setup-checklist.md`](docs/wifi-setup-checklist.md), design: [ADR 005](docs/decisions/005-wifi-setup-portal.md)).

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

## 4. Connect your dino to Wi-Fi

The clock and alarms need the real time, which the dino fetches from the internet over Wi-Fi once at start and then once a day. Wi-Fi must be a **2.4 GHz** network (the ESP32-C3 has no 5 GHz). **Everything except the clock and alarm works without Wi-Fi**; until it is connected once, the clock shows `--:--` and alarms never ring.

You set it up on the dino itself, with your phone. No app and no rebuild:

1. **Long press** the touch pad, **tilt** to the **Wi-Fi** card (the Wi-Fi logo, between Stopwatch and Back), **tap**.
2. The dino looks for Wi-Fi and says **"CONNECT YOUR DINO TO SHOW THE TIME"**. A second screen shows a network name (`DINO-1234`) and a **password** (10 letters and digits, shown as `XXXXX XXXXX`). The two screens repeat.
3. On your phone, open Wi-Fi settings, join `DINO-1234` and type the password from the dino's screen. The dino now says **"FINISH ON YOUR PHONE"**.
4. A page called **Connect your Dino** opens by itself. If it does not, open a browser and go to `http://192.168.4.1`.
5. Pick your home Wi-Fi from the list (or type its name), enter its password, tap **Connect**.
6. The dino shows **CONNECTING**, then **CONNECTED** with a heart, then the time. Your Wi-Fi is saved on the dino and the setup network switches itself off a few seconds later.

**Long press** at any time cancels. If nobody finishes within **5 minutes** the dino gives up and the setup network goes off. A wrong password gives a sad dino ("CHECK THE PASSWORD") and the pairing screen comes back: just try again. An alarm that comes due always wins over the setup.

**What to know about it** (details in [ADR 005](docs/decisions/005-wifi-setup-portal.md)):

- The setup network is encrypted (WPA2) and its password is random on every setup and appears **only on the dino's screen**, so only someone holding the dino can join it.
- Your Wi-Fi password is stored on the dino **as plain text in flash** (the chip has no flash encryption) and **only after a successful connect**. Anyone with the dino and a USB cable could read it: do not use a password you reuse elsewhere for sensitive things.
- Your phone may briefly lose the dino's network when the dino joins your router. If the page does not say "Connected", look at the dino's screen: it is the source of truth.

**For developers:** copy `include/secrets.example.h` to `include/secrets.h` and fill in `WIFI_SSID` / `WIFI_PASS` to build a network into the firmware (`secrets.h` is git-ignored: never commit it). A network saved through the menu wins over `secrets.h`.

**Time zone:** `include/config.h`, section 6: `TIME_TZ` is `"IST-5:30"` (India). Change it to your zone, for example `"GMT0"` or `"CET-1CEST,M3.5.0,M10.5.0/3"`. Setup-network timings are in the same section (`WIFI_SETUP_*`).

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
| Long press shows a Wi-Fi card between Stopwatch and Back | Rebuild and upload the current firmware (the menu has 6 cards) |
| Menu, Wi-Fi: the dino says "CONNECT YOUR DINO", then a network name and password | Look at the monitor for `[wifi] setup network ... is up`; see troubleshooting |
| The clock shows the right time (long press, Clock, tap) | Not connected yet (section 4), a 5 GHz network, or `TIME_TZ` |

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
cd firmware && .venv/bin/pio test -e native && cd ..   # 95 desktop unit tests (incl. the Wi-Fi setup logic)
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
| Clock `--:--`, alarm never rings | Wi-Fi not connected yet: long press > Wi-Fi (section 4). Use a 2.4 GHz network and move closer. `WIFI_LOW_TX_POWER` is already on, which many SuperMini boards need. Type `i` in the monitor: it says whether a network is saved |
| Wi-Fi setup: no `DINO-1234` in the phone's Wi-Fi list | Wait a few seconds after the pairing screen appears; check the monitor for `[wifi] setup network ... is up`; if it is missing, leave the pairing screen (long press) and start it again |
| Joined `DINO-1234` but the setup page does not open | Open a browser and go to `http://192.168.4.1`. Turn mobile data off for a moment (some phones prefer it over a network with no internet) |
| The dino says "NO LUCK / CHECK THE PASSWORD" | Wrong Wi-Fi password, the network is 5 GHz only, or it is out of range. The monitor says `could not join that network` (it never prints the password). Try again |
| The page never says "Connected", but the dino shows CONNECTED | Expected: the phone can lose the dino's network at that moment. Trust the dino's screen |
| Wrong time by hours | `TIME_TZ` in `config.h` |
| Tilt is reversed or erratic | Re-run the IMU calibration with the IMU fixed in place |
| Reset loop in the monitor | Report the log. The known stack-overflow cause is fixed (loop stack is 16 KB, one clip is built at a time); keep a `Grid` (8 KB) off the stack in any new code |
| Dino reacts to nothing | Touch events: check wiring and `TOUCH_*` timings; or use the Serial keys in section 7 |

## 13. Known gaps (v1)

- Petting is a short tap; a firm double tap for RAWR is not wired yet.
- A held tilt can replay the tilt clip every 4 seconds.
- The shake threshold (1.7 g) is the current default and needs tuning on the board.
- Games and Friends (design groups G and F) are not built yet.
- The menu has 6 slots: Clock, Alarm, Focus, Stopwatch, Wi-Fi, Back.
- **Wi-Fi setup has not been run on the board yet** (code complete, 30 new desktop tests, the setup page's script checked in a browser against a mock). Checks: [`docs/wifi-setup-checklist.md`](docs/wifi-setup-checklist.md).
- The Wi-Fi setup screens and icon are drawn in code and are not in the design sheet.
- Still open from the first board run ([`docs/d4-board-checklist.md`](docs/d4-board-checklist.md)): alarm hold-dismiss, alarm kept after a reset, how the clips compare with the sheet. The monitor can show a log line late (the C3's USB serial holds a short last line until the next print); the firmware loop is not stalled.

More in [`docs/checkpoints.md`](docs/checkpoints.md).
