Desktop unit tests for pure logic (gesture timing, attention states, reaction tables) go here from C2.
They will run with a `[env:native]` PlatformIO environment, without hardware.

Run: `cd firmware && pio test -e native`. Current suites: `test_imu_gestures`, `test_imu_calibration`, `test_serial_touch`, `test_time_sync`, `test_utility_machine`.
