// Soak log core: see soak_record.h.
#include "soak_record.h"
#include <stdio.h>
#include <string.h>

namespace soak {

uint8_t crc8(const uint8_t* d, size_t n) {  // CRC-8, polynomial 0x07
  uint8_t c = 0;
  for (size_t i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
  }
  return c;
}

namespace {
// Little-endian writers/readers: the layout is the same on the chip and on the desktop.
void put16(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
void put32(uint8_t* p, uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
uint16_t get16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
uint32_t get32(const uint8_t* p) { return get16(p) | ((uint32_t)get16(p + 2) << 16); }
}  // namespace

// Layout (48 bytes):
//  0 magic  1 type  2 boot(2)  4 uptimeS(4)  8 epoch(4)  12 battMv(2)  14 stackLeft(2)
// 16 heapFree(4)  20 heapMin(4)  24 heapMaxBlock(4)  28 loopWorstMs(2)  30 renderWorstMs(2)
// 32 imuErrors(2)  34 syncOk  35 syncFail  36 driftS(2)  38 state  39 code  40 arg(4)  44 flags(2)  46 0  47 crc8
void encode(const Record& r, uint8_t o[RECORD_BYTES]) {
  memset(o, 0, RECORD_BYTES);
  o[0] = MAGIC;
  o[1] = (uint8_t)r.type;
  put16(o + 2, r.boot);
  put32(o + 4, r.uptimeS);
  put32(o + 8, r.epoch);
  put16(o + 12, r.battMv);
  put16(o + 14, r.stackLeft);
  put32(o + 16, r.heapFree);
  put32(o + 20, r.heapMin);
  put32(o + 24, r.heapMaxBlock);
  put16(o + 28, r.loopWorstMs);
  put16(o + 30, r.renderWorstMs);
  put16(o + 32, r.imuErrors);
  o[34] = r.syncOk;
  o[35] = r.syncFail;
  put16(o + 36, (uint16_t)r.driftS);
  o[38] = r.state;
  o[39] = r.code;
  put32(o + 40, r.arg);
  put16(o + 44, r.flags);
  o[47] = crc8(o, RECORD_BYTES - 1);
}

bool decode(const uint8_t i[RECORD_BYTES], Record& r) {
  if (i[0] != MAGIC || i[1] < 1 || i[1] > 3) return false;
  if (crc8(i, RECORD_BYTES - 1) != i[47]) return false;
  r.type = (Type)i[1];
  r.boot = get16(i + 2);
  r.uptimeS = get32(i + 4);
  r.epoch = get32(i + 8);
  r.battMv = get16(i + 12);
  r.stackLeft = get16(i + 14);
  r.heapFree = get32(i + 16);
  r.heapMin = get32(i + 20);
  r.heapMaxBlock = get32(i + 24);
  r.loopWorstMs = get16(i + 28);
  r.renderWorstMs = get16(i + 30);
  r.imuErrors = get16(i + 32);
  r.syncOk = i[34];
  r.syncFail = i[35];
  r.driftS = (int16_t)get16(i + 36);
  r.state = i[38];
  r.code = i[39];
  r.arg = get32(i + 40);
  r.flags = get16(i + 44);
  return true;
}

const char* resetName(uint8_t reason) {  // esp_reset_reason_t values
  static const char* const N[] = {"unknown", "power_on", "external_pin", "software", "panic", "interrupt_wdt",
                                  "task_wdt", "other_wdt", "deep_sleep", "brownout", "sdio", "usb", "jtag",
                                  "efuse", "power_glitch", "cpu_lockup"};
  return reason < sizeof(N) / sizeof(N[0]) ? N[reason] : "?";
}

bool resetIsAbnormal(uint8_t r) {
  return r == 4 || r == 5 || r == 6 || r == 7 || r == 9 || r == 14 || r == 15;  // panic, wdts, brownout, glitch, lockup
}

const char* eventName(uint8_t id) {
  switch (id) {
    case EV_BATTERY_LOW: return "battery_low";
    case EV_BATTERY_CRITICAL: return "battery_critical";
    case EV_BATTERY_RECOVERED: return "battery_recovered";
    case EV_SYNC_OK: return "sync_ok";
    case EV_SYNC_FAIL: return "sync_fail";
    case EV_ALARM_RING: return "alarm_ring";
    case EV_LOOP_STALL: return "loop_stall";
    case EV_HOST_CONNECTED: return "host_connected";
    case EV_LOG_FULL: return "log_full";
    case EV_DUMP_STARTED: return "dump_started";
    default: return "?";
  }
}

const char* typeName(Type t) {
  switch (t) {
    case Type::Boot: return "boot";
    case Type::Sample: return "sample";
    case Type::Event: return "event";
    default: return "?";
  }
}

const char* csvHeader() {
  return "R,idx,type,boot,uptime_s,epoch,batt_mv,heap_free,heap_min,heap_maxblk,stack_left,loop_worst_ms,"
         "render_worst_ms,imu_err,sync_ok,sync_fail,drift_s,state,code,name,arg,flags";
}

size_t formatCsv(const Record& r, uint32_t index, char* buf, size_t n) {
  const char* name = r.type == Type::Boot ? resetName(r.code) : r.type == Type::Event ? eventName(r.code) : "";
  const int len = snprintf(buf, n, "R,%lu,%s,%u,%lu,%lu,%u,%lu,%lu,%lu,%u,%u,%u,%u,%u,%u,%d,%u,%u,%s,%lu,%u",
                           (unsigned long)index, typeName(r.type), (unsigned)r.boot, (unsigned long)r.uptimeS,
                           (unsigned long)r.epoch, (unsigned)r.battMv, (unsigned long)r.heapFree,
                           (unsigned long)r.heapMin, (unsigned long)r.heapMaxBlock, (unsigned)r.stackLeft,
                           (unsigned)r.loopWorstMs, (unsigned)r.renderWorstMs, (unsigned)r.imuErrors,
                           (unsigned)r.syncOk, (unsigned)r.syncFail, (int)r.driftS, (unsigned)r.state,
                           (unsigned)r.code, name, (unsigned long)r.arg, (unsigned)r.flags);
  return (len > 0 && (size_t)len < n) ? (size_t)len : 0;
}

uint32_t sampleInterval(const Policy& p, uint32_t mv) {
  return (mv > 0 && mv < p.criticalMv) ? p.sampleFastMs : p.sampleMs;
}

bool flushDue(const Policy& p, size_t pending, bool hasEvent, uint32_t now, uint32_t last, uint32_t mv) {
  if (pending == 0) return false;
  if (hasEvent || pending >= p.bufferRecords) return true;
  if (mv > 0 && mv < p.lowMv) return true;  // the cell may die any second: no waiting
  return (int32_t)(now - last) >= (int32_t)p.flushMs;
}

BatteryWatch::BatteryWatch(const Policy& p) : p_(p) {}

Event BatteryWatch::update(uint32_t mv) {
  if (mv == 0) return EV_NONE;
  if (level_ == 3) {  // charging: stay quiet until the voltage is back above the low line
    if (mv >= p_.lowMv) level_ = 0;
    return EV_NONE;
  }
  if (level_ < 2 && mv < p_.criticalMv) {
    level_ = 2;
    lowest_ = mv;
    return EV_BATTERY_CRITICAL;
  }
  if (level_ == 0 && mv < p_.lowMv) {
    level_ = 1;
    lowest_ = mv;
    return EV_BATTERY_LOW;
  }
  if (level_ > 0) {
    if (mv < lowest_) lowest_ = mv;
    if (mv >= lowest_ + p_.recoverMv) {  // came back up: charging
      level_ = mv >= p_.lowMv ? 0 : 3;
      lowest_ = 0;
      return EV_BATTERY_RECOVERED;
    }
  }
  return EV_NONE;
}

}  // namespace soak
