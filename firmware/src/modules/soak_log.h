// Soak log (ADR 007): the dino's black box. Every minute it records a health sample (battery mV, heap, lowest heap,
// stack, longest loop gap, render time, IMU errors, Wi-Fi syncs, state) and every notable event (boot with its reset
// reason, low battery, loop stall, alarm, sync, ...) into a file in flash (LittleFS), so a long run on a battery with
// nobody watching can be read afterwards. The file survives a reset, a dead battery and unplugging USB.
//
//   w      print the whole log over Serial (also done by itself when a Serial monitor first connects after a boot)
//   x x    erase the log (press x twice within 5 s)
//
// Record format and decisions (when to flush, battery thresholds) are pure code in src/soak/, desktop-tested.
// tools/soak_dump.py captures a dump; tools/soak_report.py analyses it.
#pragma once
#include <FS.h>
#include "../app/module.h"
#include "../soak/soak_record.h"

class SoakLog : public Module {
 public:
  const char* name() const override { return "soak"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  bool onKey(App& app, char c) override;
  const char* keyHelp() const override { return "w = dump the soak log, x x = erase it"; }
  void printInfo(Print& out) const override;

 private:
  void push(soak::Record r, bool isEvent);          // stamp it (boot, uptime, clock) and add it to the RAM buffer
  void event(App& app, uint8_t id, uint32_t arg);
  void flush();                                     // RAM buffer -> flash
  void sample(App& app, uint32_t now);
  void fill(App& app, soak::Record& r) const;
  void startDump(App& app);
  void pumpDump();
  void endDump(bool aborted);
  void erase();

  static const size_t MAX_BUFFER = 64;
  soak::Policy policy_;
  soak::BatteryWatch watch_{soak::Policy()};
  soak::Record buf_[MAX_BUFFER];
  size_t pending_ = 0;
  bool pendingEvent_ = false;
  uint32_t lastFlushMs_ = 0, nextSampleMs_ = 0, lastStallMs_ = 0, lastRtcMs_ = 0;
  uint32_t lastBattMv_ = 0;
  uint32_t fileRecords_ = 0;      // records in the file (aligned 48-byte slots)
  uint16_t boot_ = 1;
  uint8_t resetReason_ = 0;
  uint32_t prevUptimeS_ = 0;
  bool prevRunKnown_ = false;
  bool bootWritten_ = false, full_ = false, fsOk_ = false;
  soak::Record bootRec_;
  uint8_t lastState_ = 255, lastSyncOk_ = 0, lastSyncFail_ = 0;
  bool syncBaseKnown_ = false;
  bool hostWas_ = false, autoDumped_ = false, hostLogged_ = false;

  // dump in progress (non-blocking: a few lines per loop)
  enum class Dump : uint8_t { Off, Header, Rows, Footer } dump_ = Dump::Off;
  fs::File dumpFile_;
  uint32_t dumpIdx_ = 0, dumpBad_ = 0, dumpSlots_ = 0;
  uint32_t eraseArmedMs_ = 0;
};
