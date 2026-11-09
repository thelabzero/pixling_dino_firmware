#include "soak_log.h"
#include <LittleFS.h>
#include <esp_system.h>
#include <time.h>
#include "config.h"
#include "../app/app.h"

using namespace soak;

static const char* kFile = "/soak.bin";

// Survives a software reset, a panic and a watchdog reset (RAM that the start-up code does not clear). Not a power
// cut: then the magic check fails and the flash log has to tell the story. Written every few seconds (RAM, no wear).
struct RtcMemo {
  uint32_t magic, uptimeS, heapMin, check;
  uint8_t state;
};
static RTC_NOINIT_ATTR RtcMemo gMemo;
static const uint32_t kMemoMagic = 0x50AC10C7;
static uint32_t memoCheck(const RtcMemo& m) { return m.magic ^ m.uptimeS ^ (m.heapMin * 3u) ^ m.state; }

static uint16_t clamp16(uint32_t v) { return (uint16_t)(v > 65535 ? 65535 : v); }

bool SoakLog::begin(App&) {
  policy_.sampleMs = (uint32_t)SOAK_SAMPLE_S * 1000UL;
  policy_.sampleFastMs = (uint32_t)SOAK_SAMPLE_FAST_S * 1000UL;
  policy_.flushMs = (uint32_t)SOAK_FLUSH_S * 1000UL;
  policy_.lowMv = SOAK_LOW_MV;
  policy_.criticalMv = SOAK_CRITICAL_MV;
  policy_.recoverMv = SOAK_RECOVER_MV;
  policy_.bufferRecords = SOAK_BUFFER_RECORDS > MAX_BUFFER ? MAX_BUFFER : SOAK_BUFFER_RECORDS;
  watch_ = BatteryWatch(policy_);

  if (!LittleFS.begin(true)) {  // true = format on the first use
    Serial.println("[soak] flash file system did not start: no log this run");
    return false;
  }
  fsOk_ = true;

  // What does the file already hold? The last good record gives the boot number and how long the previous run lived.
  uint32_t lastUptime = 0;
  {
    fs::File f = LittleFS.open(kFile, "r");
    if (f) {
      const size_t size = f.size();
      fileRecords_ = (uint32_t)((size + RECORD_BYTES - 1) / RECORD_BYTES);
      for (uint32_t back = 1; back <= 50 && back <= fileRecords_; back++) {  // newest first, skip torn ones
        uint8_t b[RECORD_BYTES];
        f.seek((fileRecords_ - back) * RECORD_BYTES);
        Record r;
        if (f.read(b, RECORD_BYTES) == RECORD_BYTES && decode(b, r)) {
          boot_ = (uint16_t)(r.boot + 1);
          lastUptime = r.uptimeS;
          prevRunKnown_ = true;
          break;
        }
      }
      f.close();
      if (size % RECORD_BYTES) {  // a write was cut: pad to the boundary so the next record is aligned
        fs::File a = LittleFS.open(kFile, "a");
        if (a) {
          uint8_t z[RECORD_BYTES] = {};
          a.write(z, RECORD_BYTES - size % RECORD_BYTES);
          a.close();
        }
      }
    }
  }

  resetReason_ = (uint8_t)esp_reset_reason();
  const bool memoOk = gMemo.magic == kMemoMagic && gMemo.check == memoCheck(gMemo) && resetReason_ != ESP_RST_POWERON;
  prevUptimeS_ = memoOk ? gMemo.uptimeS : lastUptime;  // RAM memo is newer than the last flushed sample
  gMemo = RtcMemo{kMemoMagic, 0, 0, 0, 0};
  gMemo.check = memoCheck(gMemo);

  Serial.printf("[soak] log: %lu records in flash, boot #%u, reset reason %s (%u)%s", (unsigned long)fileRecords_,
                (unsigned)boot_, resetName(resetReason_), (unsigned)resetReason_, resetIsAbnormal(resetReason_) ? " (ABNORMAL)" : "");
  if (prevRunKnown_) Serial.printf(", previous run lasted %lu s", (unsigned long)prevUptimeS_);
  Serial.println();
  return true;
}

void SoakLog::fill(App& app, Record& r) const {
  r.boot = boot_;
  r.uptimeS = millis() / 1000;
  app.soakFill(r);
  if (r.flags & FL_TIME_VALID) r.epoch = (uint32_t)time(nullptr);
  if (Serial) r.flags |= FL_HOST_OPEN;
}

void SoakLog::push(Record r, bool isEvent) {
  if (!fsOk_) return;
  if (full_ && !(r.type == Type::Event && r.code == EV_LOG_FULL)) return;
  if (pending_ >= policy_.bufferRecords) {
    if (dump_ == Dump::Off) flush();
    if (pending_ >= policy_.bufferRecords) return;  // a dump is running: drop rather than block
  }
  buf_[pending_++] = r;
  pendingEvent_ = pendingEvent_ || isEvent;
}

void SoakLog::event(App& app, uint8_t id, uint32_t arg) {
  Record r;
  r.type = Type::Event;
  r.code = id;
  r.arg = arg;
  r.battMv = clamp16(lastBattMv_);
  fill(app, r);
  push(r, true);
  if (arg) Serial.printf("[soak] event %s %lu\n", eventName(id), (unsigned long)arg);
  else Serial.printf("[soak] event %s\n", eventName(id));
}

void SoakLog::flush() {
  if (!pending_ || !fsOk_) return;
  fs::File f = LittleFS.open(kFile, "a");
  bool ok = false;
  if (f) {
    uint8_t b[RECORD_BYTES];
    ok = true;
    for (size_t i = 0; i < pending_ && ok; i++) {
      encode(buf_[i], b);
      ok = f.write(b, RECORD_BYTES) == RECORD_BYTES;
    }
    f.close();
  }
  if (ok) {
    fileRecords_ += (uint32_t)pending_;
  } else {
    Serial.println("[soak] could not write the log (flash full?): logging stops");
    full_ = true;
  }
  pending_ = 0;
  pendingEvent_ = false;
  lastFlushMs_ = millis();
  if (!full_ && fileRecords_ >= SOAK_MAX_RECORDS) {  // one last record says why the samples stop
    Record r;
    r.type = Type::Event;
    r.code = EV_LOG_FULL;
    r.boot = boot_;
    r.uptimeS = millis() / 1000;
    uint8_t b[RECORD_BYTES];
    encode(r, b);
    fs::File a = LittleFS.open(kFile, "a");
    if (a) { a.write(b, RECORD_BYTES); a.close(); }
    full_ = true;
    Serial.println("[soak] log is full: samples stop (dump it with w, erase with x x)");
  }
}

void SoakLog::sample(App& app, uint32_t now) {
  Record r;
  r.type = Type::Sample;
  fill(app, r);
  r.stackLeft = clamp16(uxTaskGetStackHighWaterMark(NULL));
  r.heapFree = ESP.getFreeHeap();
  r.heapMin = ESP.getMinFreeHeap();
  r.heapMaxBlock = ESP.getMaxAllocHeap();
  r.loopWorstMs = clamp16(app.takeLoopWorstMs());
  lastBattMv_ = r.battMv;

  const uint32_t mv = (r.flags & (FL_USB_VOLTAGE | FL_NO_BATTERY)) ? 0 : r.battMv;  // USB-only: no battery to watch
  const Event be = watch_.update(mv);
  if (be != EV_NONE) event(app, be, r.battMv);

  if (syncBaseKnown_) {
    if (r.syncOk != lastSyncOk_) event(app, EV_SYNC_OK, (uint32_t)(int32_t)r.driftS);
    if (r.syncFail != lastSyncFail_) event(app, EV_SYNC_FAIL, 0);
  }
  lastSyncOk_ = r.syncOk;
  lastSyncFail_ = r.syncFail;
  syncBaseKnown_ = true;

  push(r, false);
  Serial.printf("[soak] up %lu s, batt %u mV, heap %lu (min %lu, block %lu), stack %u B, loop worst %u ms, render worst %u ms, imu err %u, state %s\n",
                (unsigned long)r.uptimeS, (unsigned)r.battMv, (unsigned long)r.heapFree, (unsigned long)r.heapMin,
                (unsigned long)r.heapMaxBlock, (unsigned)r.stackLeft, (unsigned)r.loopWorstMs, (unsigned)r.renderWorstMs,
                (unsigned)r.imuErrors, utility::Machine::stateNameOf(r.state));
  nextSampleMs_ = now + sampleInterval(policy_, mv);
}

void SoakLog::loop(App& app, uint32_t now) {
  if (!fsOk_) return;

  if (!bootWritten_) {  // first loop: every module has started, so battery and state are known
    bootWritten_ = true;
    bootRec_ = Record();
    bootRec_.type = Type::Boot;
    bootRec_.code = resetReason_;
    bootRec_.arg = prevUptimeS_;
    fill(app, bootRec_);
    bootRec_.stackLeft = clamp16(uxTaskGetStackHighWaterMark(NULL));
    bootRec_.heapFree = ESP.getFreeHeap();
    bootRec_.heapMin = ESP.getMinFreeHeap();
    bootRec_.heapMaxBlock = ESP.getMaxAllocHeap();
    lastBattMv_ = bootRec_.battMv;
    push(bootRec_, true);
    nextSampleMs_ = now + 5000;  // a first sample soon, so a short run still leaves one
  }

  // Loop stall: one pass took too long. The alarm is exact only while this stays quiet.
  if (app.lastLoopGapMs() > SOAK_STALL_MS && (int32_t)(now - lastStallMs_) >= 10000) {
    lastStallMs_ = now;
    event(app, EV_LOOP_STALL, app.lastLoopGapMs());
  }

  const uint8_t st = app.machine().stateCode();
  if (st != lastState_) {
    if (lastState_ != 255 && !strcmp(utility::Machine::stateNameOf(st), "ringing")) event(app, EV_ALARM_RING, 0);
    lastState_ = st;
  }

  if ((int32_t)(now - nextSampleMs_) >= 0) sample(app, now);

  // RAM memo for the next boot (cheap, no flash).
  if ((int32_t)(now - lastRtcMs_) >= 0) {
    lastRtcMs_ = now + 5000;
    gMemo.magic = kMemoMagic;
    gMemo.uptimeS = now / 1000;
    gMemo.heapMin = ESP.getMinFreeHeap();
    gMemo.state = st;
    gMemo.check = memoCheck(gMemo);
  }

  // A Serial monitor opened: say so, and hand over the whole log once per boot.
  const bool host = (bool)Serial;
  if (host && !hostWas_) {
    if (!hostLogged_) { hostLogged_ = true; event(app, EV_HOST_CONNECTED, 0); }  // the USB flag flickers: once per boot
#if SOAK_AUTO_DUMP
    if (!autoDumped_) { autoDumped_ = true; startDump(app); }
#endif
  }
  hostWas_ = host;

  if (dump_ != Dump::Off) pumpDump();
  else if (flushDue(policy_, pending_, pendingEvent_, now, lastFlushMs_, lastBattMv_)) flush();
}

// ---- dump: a few lines per loop, only while a monitor is open (a USB write with nobody listening would block) ----

void SoakLog::startDump(App&) {
  if (!fsOk_ || dump_ != Dump::Off) return;
  if (!Serial) { Serial.println("[soak] no Serial monitor open: nothing to send to"); return; }
  flush();
  dumpFile_ = LittleFS.open(kFile, "r");
  dumpSlots_ = dumpFile_ ? (uint32_t)((dumpFile_.size()) / RECORD_BYTES) : 0;
  dumpIdx_ = dumpBad_ = 0;
  dump_ = Dump::Header;
}

void SoakLog::pumpDump() {
  if (!Serial) { endDump(true); return; }
  for (int lines = 0; lines < 12 && Serial.availableForWrite() >= 230; lines++) {
    if (dump_ == Dump::Header) {
      Serial.printf("SOAKDUMP BEGIN version=1 firmware=%s slots=%lu boot_now=%u uptime_now_s=%lu%s\n", FIRMWARE_VERSION,
                    (unsigned long)dumpSlots_, (unsigned)boot_, (unsigned long)(millis() / 1000), full_ ? " log_full=1" : "");
      Serial.print("SOAKDUMP STATES ");
      for (uint8_t i = 0; i < 17; i++) Serial.printf("%s%u=%s", i ? "," : "", (unsigned)i, utility::Machine::stateNameOf(i));
      Serial.println();
      Serial.printf("SOAKDUMP COLUMNS %s\n", csvHeader() + 2);
      dump_ = Dump::Rows;
      continue;
    }
    if (dump_ == Dump::Rows) {
      uint8_t b[RECORD_BYTES];
      Record r;
      if (!dumpFile_ || dumpFile_.read(b, RECORD_BYTES) != RECORD_BYTES) { dump_ = Dump::Footer; continue; }
      if (!decode(b, r)) { dumpBad_++; continue; }
      char line[220];
      if (formatCsv(r, dumpIdx_++, line, sizeof(line))) Serial.println(line);
      continue;
    }
    if (dump_ == Dump::Footer) {
      Serial.printf("SOAKDUMP END records=%lu bad=%lu\n", (unsigned long)dumpIdx_, (unsigned long)dumpBad_);
      endDump(false);
      return;
    }
  }
}

void SoakLog::endDump(bool aborted) {
  if (dumpFile_) dumpFile_.close();
  dump_ = Dump::Off;
  if (aborted) Serial.println("[soak] dump stopped: monitor closed");
}

void SoakLog::erase() {
  if (dump_ != Dump::Off) endDump(true);
  pending_ = 0;
  pendingEvent_ = false;
  LittleFS.remove(kFile);
  fileRecords_ = 0;
  full_ = false;
  if (bootWritten_) push(bootRec_, true);  // the file always starts with this run's boot record
  Serial.println("[soak] log erased");
}

bool SoakLog::onKey(App& app, char c) {
  if (c == 'w' || c == 'W') {
    if (!fsOk_) Serial.println("[soak] no flash file system");
    else startDump(app);
    return true;
  }
  if (c == 'x' || c == 'X') {
    const uint32_t now = millis();
    if (eraseArmedMs_ && (int32_t)(now - eraseArmedMs_) < 5000) { eraseArmedMs_ = 0; erase(); }
    else { eraseArmedMs_ = now ? now : 1; Serial.println("[soak] press x again within 5 s to erase the log"); }
    return true;
  }
  return false;
}

void SoakLog::printInfo(Print& out) const {
  if (!fsOk_) { out.println("[soak] off (no flash file system)"); return; }
  out.printf("[soak] %lu records in flash + %u waiting, boot #%u, flash used %lu of %lu B, last write %lu s ago%s\n",
             (unsigned long)fileRecords_, (unsigned)pending_, (unsigned)boot_, (unsigned long)LittleFS.usedBytes(),
             (unsigned long)LittleFS.totalBytes(), (unsigned long)((millis() - lastFlushMs_) / 1000), full_ ? ", FULL" : "");
}
