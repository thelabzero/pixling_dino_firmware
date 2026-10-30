#include "battery.h"
#include "config.h"
#include "../app/app.h"

bool Battery::begin(App&) {
  pinMode(PIN_BATTERY_ADC, INPUT);
  avg_.add((float)readOnce());  // first value straight away
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

void Battery::loop(App&, uint32_t now) {
  if ((int32_t)(now - nextRead_) >= 0) {
    nextRead_ = now + BATTERY_READ_MS;
    avg_.add((float)readOnce());
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
  out.printf("[battery] %.2f V (pin %lu mV), about %d %% (estimate)\n", mv / 1000.0, (unsigned long)lastPinMv_,
             percent());
}
