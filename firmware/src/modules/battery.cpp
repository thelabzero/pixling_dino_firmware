#include "battery.h"
#include <Wire.h>
#include <esp_sleep.h>
#include <string.h>
#include "config.h"
#include "../app/app.h"

// Survives deep sleep (RTC memory): set when the empty-battery shutdown started, so the next wake-up knows why.
RTC_DATA_ATTR static bool gLowSleep = false;

Battery::Battery() : cut_((uint32_t)BATTERY_CUTOFF_MV, (uint32_t)BATTERY_CUTOFF_HOLD_S * 1000UL, BATTERY_MISSING_MV) {}

static uint32_t pinMvAveraged() {
  uint32_t sum = 0;
  for (int i = 0; i < BATTERY_SAMPLES; i++) sum += analogReadMilliVolts(PIN_BATTERY_ADC);
  return sum / BATTERY_SAMPLES;
}

// Deep sleep with a timer (to look at USB and the voltage) and the touch pad (wake on touch).
static void deepSleepNow() {
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_TOUCH, ESP_GPIO_WAKEUP_GPIO_HIGH);
  esp_sleep_enable_timer_wakeup((uint64_t)BATTERY_SLEEP_CHECK_S * 1000000ULL);
  esp_deep_sleep_start();
}

// Runs before anything else. Only acts when we are in the low-battery sleep.
void Battery::guardAtBoot() {
  if (esp_reset_reason() != ESP_RST_DEEPSLEEP || !gLowSleep) return;
  pinMode(PIN_BATTERY_ADC, INPUT);
  const uint32_t mv = power::batteryMv(pinMvAveraged(), BATTERY_R_TOP_K, BATTERY_R_BOTTOM_K, BATTERY_CAL);
  if (mv > BATTERY_MAX_MV || mv >= BATTERY_RESUME_MV) { gLowSleep = false; return; }  // plugged in, or recovered: boot normally
  deepSleepNow();  // still empty: back to sleep (the OLED and the IMU were switched off before the first sleep and stay off)
}

// The cell is empty: save the log, switch the panel and the IMU off, and sleep. Nothing wakes the dino for a
// reason other than USB, a recovered voltage, or a touch that finds the battery still empty.
void Battery::shutdown(App& app) {
  Serial.printf("[battery] %lu mV for %d s: going to sleep to protect the cell. Plug in USB to wake it.\n",
                (unsigned long)millivolts(), (int)BATTERY_CUTOFF_HOLD_S);
  app.prepareShutdown();
  gLowSleep = true;
  Wire.beginTransmission(OLED_ADDR);  // SSD1306: 0x00 = commands; display off, charge pump off
  Wire.write((uint8_t)0x00); Wire.write((uint8_t)0xAE); Wire.write((uint8_t)0x8D); Wire.write((uint8_t)0x10);
  Wire.endTransmission();
  for (uint8_t addr : {(uint8_t)0x68, (uint8_t)0x69}) {  // MPU6050: PWR_MGMT_1 sleep bit
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x6B); Wire.write((uint8_t)0x40);
    Wire.endTransmission();
  }
  deepSleepNow();
}

bool Battery::begin(App& app) {
  app.setBatteryInfo(this);
  pinMode(PIN_BATTERY_ADC, INPUT);
  avg_.add((float)readOnce());  // first value straight away
  pct_.update(rawPercent(), usb());
  Serial.printf("[battery] ADC on GPIO%d, divider %.0fk / %.0fk\n", PIN_BATTERY_ADC, (double)BATTERY_R_TOP_K,
                (double)BATTERY_R_BOTTOM_K);
  printInfo(Serial);
  return true;
}

// Average several quick ADC reads: the 330k divider is a weak source, single reads are noisy.
uint32_t Battery::readOnce() {
  uint32_t sum = 0;
  for (int i = 0; i < BATTERY_SAMPLES; i++) sum += analogReadMilliVolts(PIN_BATTERY_ADC);  // calibrated mV
  lastPinMv_ = sum / BATTERY_SAMPLES;
  return power::batteryMv(lastPinMv_, BATTERY_R_TOP_K, BATTERY_R_BOTTOM_K, BATTERY_CAL);
}

void Battery::loop(App& app, uint32_t now) {
  if ((int32_t)(now - nextRead_) >= 0) {
    nextRead_ = now + BATTERY_READ_MS;
    avg_.add((float)readOnce());
    pct_.update(rawPercent(), usb());
    // Not while the alarm rings: an alarm is never late, and a sagging cell under the buzzer is not an empty one.
    if (cut_.update(millivolts(), usb(), now) && strcmp(app.machine().stateName(), "ringing") != 0) shutdown(app);
  }
  if (BATTERY_LOG_S > 0 && (int32_t)(now - nextLog_) >= 0) {
    if (nextLog_) printInfo(Serial);  // not at 0: begin() already printed
    nextLog_ = now + BATTERY_LOG_S * 1000UL;
  }
}

bool Battery::onKey(App&, char c) {
  if (c != 'b') return false;
  avg_.add((float)readOnce());
  printInfo(Serial);
  return true;
}

void Battery::printInfo(Print& out) const {
  const uint32_t mv = millivolts();
  if (mv < BATTERY_MISSING_MV) {
    out.printf("[battery] %lu mV at the pin -> %.2f V: no battery connected? (USB power only)\n",
               (unsigned long)lastPinMv_, mv / 1000.0);
    return;
  }
  if (mv > BATTERY_MAX_MV) {
    out.printf("[battery] %.2f V (pin %lu mV): above a Li-ion cell's 4.2 V max, so this is USB / charger voltage, not a battery\n",
               mv / 1000.0, (unsigned long)lastPinMv_);
    return;
  }
  out.printf("[battery] %.2f V (pin %lu mV), about %d %% (estimate; shown %d %%)\n", mv / 1000.0, (unsigned long)lastPinMv_,
             rawPercent(), percent());
}

void Battery::soakFill(soak::Record& r) const {
  const uint32_t mv = millivolts();
  r.battMv = (uint16_t)(mv > 65535 ? 65535 : mv);
  if (mv < BATTERY_MISSING_MV) r.flags |= soak::FL_NO_BATTERY;
  if (mv > BATTERY_MAX_MV) r.flags |= soak::FL_USB_VOLTAGE;
}
