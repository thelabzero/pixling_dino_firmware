# IMU calibrator (MPU6050, browser)

Measures accel offset/scale, gyro bias and how the IMU is mounted, and prints the exact `#define` lines for `firmware/include/config.h`.

1. Flash the stream firmware (PlatformIO, pins come from `config.h`):
   `cd firmware && pio run -e imu_calibrator -t upload`
   Then close any serial monitor.
2. Open `imu_calibrator.html` in Chrome or Edge, press Connect.
3. Do steps 1-3 on the page. It guides you: a checklist at the top, a blue banner that always says what to do next, and in step 2 a live hint that names which of the six buttons matches the face the board is resting on. Redoing step 2 clears step 3 (it depends on it). The Result box fills in as you go. Press **Copy for config.h** and paste it over the
   `IMU_ACC_*`, `IMU_GYRO_BIAS_*`, `IMU_UP_*`, `IMU_FRONT_*` lines in `firmware/include/config.h` (section 4).
   A step you have not done yet is printed as a comment ("keep the existing ... lines"), so pasting never overwrites
   values you did not measure.
4. Flash the normal firmware again: `pio run -e dino_c3 -t upload`.

Re-run it whenever the IMU is re-mounted or replaced. Gyro bias is also re-learned by the firmware while the dino is still.

Desktop test (no hardware): `node tools/imu_calibrator/test_page.js` runs the page's own script against a simulated sensor
and checks the numbers, the layout against `config.h`, and that the block compiles.

Full guide (mounting checklist, loading values, verifying, gesture test): `docs/imu-calibration.md`.
