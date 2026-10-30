// Input check (plan steps 1-2): I2C scan, calibrated MPU6050, IMU gestures, and touch
// typed in the Serial monitor (TTP223 not wired yet, ADR 011).
//   pio run -e play_inputs -t upload && pio device monitor -e play_inputs
//
// Serial (115200), one letter:
//   t d l h   touch: tap, double tap, long press, hold
//   r         reset counters     s   rescan I2C
//   p         raw stream on/off  ?   help + counts
// Calibration values live in include/config.h (from tools/imu_calibrator).

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "drivers/mpu6050.h"
#include "input/events.h"
#include "input/imu_calibration.h"
#include "input/imu_gestures.h"
#include "input/serial_touch.h"

using input::Event;
using input::Vec3;

static Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
static Mpu6050 imu;
static input::ImuCorrector corrector;
static input::ImuGestures gestures;

static bool haveDisplay = false, haveImu = false, rawOn = false;
static Vec3 accel{0, 0, 1}, gyro{0, 0, 0};  // latest calibrated sample
static uint16_t counts[(int)Event::Count];
static Event lastEvent = Event::None;
static uint32_t lastEventAt = 0;

// ---------------------------------------------------------------- setup helpers
static input::ImuCalibration calibrationFromConfig() {
  input::ImuCalibration c;
  c.accOffsetG = {IMU_ACC_OFFSET_X, IMU_ACC_OFFSET_Y, IMU_ACC_OFFSET_Z};
  c.accScale = {IMU_ACC_SCALE_X, IMU_ACC_SCALE_Y, IMU_ACC_SCALE_Z};
  c.gyroBiasDps = {IMU_GYRO_BIAS_X, IMU_GYRO_BIAS_Y, IMU_GYRO_BIAS_Z};
  return c;
}

static input::GestureConfig gesturesFromConfig() {
  input::GestureConfig g;
  g.up = {IMU_UP_X, IMU_UP_Y, IMU_UP_Z};
  g.front = {IMU_FRONT_X, IMU_FRONT_Y, IMU_FRONT_Z};
  return g;
}

static const char* deviceName(uint8_t a) {
  if (a == 0x3C || a == 0x3D) return "OLED";
  if (a == 0x68 || a == 0x69) return "MPU6050";
  return "?";
}

// Prints every address that answers; returns how many.
static uint8_t scanI2C(uint8_t* found, uint8_t max) {
  uint8_t n = 0;
  Serial.printf("I2C scan (SDA %d, SCL %d):\n", PIN_I2C_SDA, PIN_I2C_SCL);
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() != 0) continue;
    Serial.printf("  0x%02X %s\n", a, deviceName(a));
    if (n < max) found[n++] = a;
  }
  if (!n) Serial.println("  nothing - check power, GND, SDA, SCL");
  return n;
}

static bool startImu() {
  for (uint8_t a : {(uint8_t)0x68, (uint8_t)0x69}) {
    if (!imu.begin(Wire, a)) continue;
    Serial.printf("MPU6050 at 0x%02X, WHO_AM_I 0x%02X\n", a, imu.whoAmI());
    return true;
  }
  Serial.println("MPU6050 not found: touch letters still work");
  return false;
}

static void showScan(const uint8_t* found, uint8_t n) {
  if (!haveDisplay) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("I2C scan");
  if (!n) display.println("nothing found");
  for (uint8_t i = 0; i < n && i < 5; i++) display.printf("0x%02X %s\n", found[i], deviceName(found[i]));
  display.setCursor(0, 56);
  display.print(haveImu ? "IMU ok" : "IMU NOT FOUND");
  display.display();
}

// ---------------------------------------------------------------- events + serial
static void onEvent(Event e) {
  counts[(int)e]++;
  lastEvent = e;
  lastEventAt = millis();
  Serial.printf("[%9.3f s] %-11s #%u\n", millis() / 1000.0, input::eventName(e), counts[(int)e]);
}

static void printHelp() {
  Serial.println("t d l h = tap, double tap, long press, hold | r reset | s scan | p raw | ? help");
  Serial.print("counts:");
  for (int i = 1; i < (int)Event::Count; i++) Serial.printf(" %s=%u", input::eventName((Event)i), counts[i]);
  Serial.println();
}

static void handleSerial() {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    const Event touch = input::touchFromChar(c);
    if (touch != Event::None) { onEvent(touch); continue; }
    switch (c) {
      case 'r': memset(counts, 0, sizeof(counts)); Serial.println("counters reset"); break;
      case 's': { uint8_t f[8]; scanI2C(f, 8); break; }
      case 'p': rawOn = !rawOn; break;
      case '?': printHelp(); break;
      default: break;
    }
  }
}

// ---------------------------------------------------------------- screen
static const char* shortName(Event e) {
  switch (e) {
    case Event::DoubleTap: return "DBL TAP";
    case Event::LongPress: return "LONG";
    case Event::TiltLeft: return "TILT L";
    case Event::TiltRight: return "TILT R";
    case Event::FlipDown: return "FLIP DN";
    case Event::FlipUp: return "FLIP UP";
    case Event::PickUp: return "PICK UP";
    case Event::PutDown: return "PUT DN";
    default: return input::eventName(e);
  }
}

static const char* stateName(const input::ImuState& s) {
  if (s.faceDown) return "FACE DOWN";
  if (s.held) return "HELD";
  if (s.resting) return "RESTING";
  return s.still ? "still" : "moving";
}

static void drawScreen() {
  if (!haveDisplay) return;
  const input::ImuState& s = gestures.state();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (haveImu) {
    display.printf("R%+4d P%+4d %.2fg", (int)s.rollDeg, (int)s.pitchDeg, s.accelMag);
    display.setCursor(0, 9);
    display.print(stateName(s));
    // level bubble: +-45 deg reaches the edge
    const int x0 = 100, size = 27, half = size / 2, reach = half - 3;
    display.drawRect(x0, 0, size, size, SSD1306_WHITE);
    const int dx = constrain((int)(s.rollDeg / 45.0f * reach), -reach, reach);
    const int dy = constrain((int)(s.pitchDeg / 45.0f * reach), -reach, reach);
    display.fillCircle(x0 + half + dx, half + dy, 2, SSD1306_WHITE);
  } else {
    display.print("no IMU - touch only");
  }
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print(millis() - lastEventAt < 2000 ? shortName(lastEvent) : "--");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("L%u R%u Dn%u Up%u Sh%u", counts[(int)Event::TiltLeft], counts[(int)Event::TiltRight],
                 counts[(int)Event::FlipDown], counts[(int)Event::FlipUp], counts[(int)Event::Shake]);
  display.setCursor(0, 48);
  display.printf("Pick%u Put%u", counts[(int)Event::PickUp], counts[(int)Event::PutDown]);
  display.setCursor(0, 56);
  display.printf("T%u D%u Lp%u H%u", counts[(int)Event::Tap], counts[(int)Event::DoubleTap],
                 counts[(int)Event::LongPress], counts[(int)Event::Hold]);
  display.display();
}

// ---------------------------------------------------------------- main
void setup() {
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);  // give the USB monitor a moment
  Serial.println("\nDino play_inputs");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_FREQ_HZ);
  uint8_t found[8];
  const uint8_t n = scanI2C(found, 8);

  haveDisplay = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (haveDisplay) display.setTextColor(SSD1306_WHITE);
  else Serial.println("OLED did not start: Serial only");
  haveImu = startImu();

  corrector = input::ImuCorrector(calibrationFromConfig());
  gestures.setConfig(gesturesFromConfig());

  showScan(found, n);
  delay(3000);  // time to read the scan on the OLED
  printHelp();
}

void loop() {
  static uint32_t nextSample = 0, nextDraw = 0, nextRaw = 0;
  const uint32_t now = millis();
  handleSerial();

  if (haveImu && (int32_t)(now - nextSample) >= 0) {
    nextSample = now + IMU_SAMPLE_MS;
    if (imu.read(accel, gyro)) {
      corrector.apply(accel, gyro, now);
      gestures.update({accel, gyro, now});
      for (Event e; (e = gestures.next()) != Event::None;) onEvent(e);
    }
  }

  if (rawOn && (int32_t)(now - nextRaw) >= 0) {
    nextRaw = now + 100;
    const input::ImuState& s = gestures.state();
    const Vec3 b = corrector.gyroBias();
    Serial.printf("a %+.3f %+.3f %+.3f |%.3f| g  gyro %+6.1f %+6.1f %+6.1f  bias %+.2f %+.2f %+.2f%s  roll %+5.1f pitch %+5.1f\n",
                  accel.x, accel.y, accel.z, s.accelMag, gyro.x, gyro.y, gyro.z, b.x, b.y, b.z,
                  corrector.learning() ? " (learning)" : "", s.rollDeg, s.pitchDeg);
  }

  if ((int32_t)(now - nextDraw) >= 0) {
    nextDraw = now + 66;  // ~15 fps is plenty for a test screen
    drawScreen();
  }
  delay(1);
}
