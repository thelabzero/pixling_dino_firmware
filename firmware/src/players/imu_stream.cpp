// MPU6050 raw stream for the browser IMU calibrator (tools/imu_calibrator/imu_calibrator.html).
// PlatformIO hardware tool (was an Arduino sketch). Pins and I2C speed come from include/config.h.
//   cd firmware && pio run -e imu_calibrator -t upload
// Then CLOSE any serial monitor, open the HTML page in Chrome/Edge and press Connect.
//
// Output (one line each, 115200 baud):
//   I,<addr>,<whoami>,<accel LSB per g>,<gyro LSB per dps>,<rate Hz>   info, sent at boot and on '?'
//   S,<addr list>                                                     I2C scan result
//   D,<t_ms>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<tempC*100>                raw sample (int16 counts)
//   E,<message>                                                       error
// Commands (send a line): '?' info + rescan, 'R50' / 'R100' / 'R200' sample rate in Hz.
// No library needed. The protocol is unchanged from the Arduino sketch, so the page works as before.

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Wiring comes from include/config.h (PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ_HZ).
static const int PIN_SDA = PIN_I2C_SDA;
static const int PIN_SCL = PIN_I2C_SCL;
static const uint32_t I2C_HZ = I2C_FREQ_HZ;

// ---- sensor ranges (the HTML page reads the scale from the I line) ----
// ACCEL_CONFIG: 0x00 = +-2 g (16384 LSB/g), 0x08 = +-4 g (8192), 0x10 = +-8 g (4096)
// GYRO_CONFIG:  0x00 = +-250 dps (131),     0x08 = +-500 (65.5), 0x10 = +-1000 (32.8)
const uint8_t ACCEL_CFG = 0x08;  const float ACCEL_LSB = 8192.0f;
const uint8_t GYRO_CFG  = 0x08;  const float GYRO_LSB  = 65.5f;
const uint8_t DLPF_CFG  = 0x03;  // ~44 Hz low-pass

uint8_t mpuAddr = 0;
uint8_t whoAmI = 0;
uint16_t rateHz = 100;
uint32_t nextSampleUs = 0;

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(mpuAddr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(mpuAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(mpuAddr, (uint8_t)1) != 1) return 0;
  return Wire.read();
}

void scanI2C() {
  Serial.print("S");
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf(",0x%02X", a);
  }
  Serial.println();
}

bool startMpu() {
  mpuAddr = 0;
  for (uint8_t a : {(uint8_t)0x68, (uint8_t)0x69}) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { mpuAddr = a; break; }
  }
  if (!mpuAddr) return false;
  writeReg(0x6B, 0x80);  // reset
  delay(100);
  writeReg(0x6B, 0x01);  // wake, gyro clock
  delay(10);
  writeReg(0x1A, DLPF_CFG);
  writeReg(0x1B, GYRO_CFG);
  writeReg(0x1C, ACCEL_CFG);
  writeReg(0x19, 0x00);  // sample rate divider (we pace reads ourselves)
  whoAmI = readReg(0x75);
  return true;
}

void printInfo() {
  scanI2C();
  if (!mpuAddr) { Serial.println("E,MPU6050 not found at 0x68/0x69 - check VCC, GND, SDA, SCL"); return; }
  Serial.printf("I,0x%02X,0x%02X,%.1f,%.1f,%u\n", mpuAddr, whoAmI, ACCEL_LSB, GYRO_LSB, rateHz);
}

void handleCommands() {
  static char buf[16];
  static uint8_t n = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      buf[n] = 0;
      if (n) {
        if (buf[0] == '?') { if (!mpuAddr) startMpu(); printInfo(); }
        else if (buf[0] == 'R' || buf[0] == 'r') {
          int hz = atoi(buf + 1);
          if (hz >= 10 && hz <= 500) { rateHz = hz; printInfo(); }
        }
      }
      n = 0;
    } else if (n < sizeof(buf) - 1) buf[n++] = c;
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(I2C_HZ);
  startMpu();
  printInfo();
  nextSampleUs = micros();
}

void loop() {
  handleCommands();
  if (!mpuAddr) {  // keep trying, so a loose wire can be fixed live
    static uint32_t lastTry = 0;
    if (millis() - lastTry > 2000) { lastTry = millis(); if (startMpu()) printInfo(); }
    return;
  }
  if ((int32_t)(micros() - nextSampleUs) < 0) return;
  nextSampleUs += 1000000UL / rateHz;

  Wire.beginTransmission(mpuAddr);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(mpuAddr, (uint8_t)14) != 14) {
    Serial.println("E,read failed");
    nextSampleUs = micros() + 100000;  // back off 100 ms
    return;
  }
  int16_t v[7];
  for (int i = 0; i < 7; i++) { int hi = Wire.read(); int lo = Wire.read(); v[i] = (int16_t)((hi << 8) | lo); }
  int tempC100 = (int)((v[3] / 340.0f + 36.53f) * 100);
  Serial.printf("D,%lu,%d,%d,%d,%d,%d,%d,%d\n", (unsigned long)millis(), v[0], v[1], v[2], v[4], v[5], v[6], tempC100);
}
