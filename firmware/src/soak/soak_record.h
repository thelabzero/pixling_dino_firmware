// Soak log core (ADR 007): the fixed-size record the dino keeps in flash during a long run, how it is
// packed, checked and printed as one CSV line, and the small decisions around it (when to flush to flash,
// how often to sample, when the battery counts as low). Pure C++ (no Arduino), desktop-tested in test/test_soak.
//
// Why fixed 48-byte records: a power cut can tear the last write. With a fixed size and a CRC the reader
// skips a torn record and stays aligned on the next one.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace soak {

constexpr size_t RECORD_BYTES = 48;
constexpr uint8_t MAGIC = 0xD5;

enum class Type : uint8_t { None = 0, Boot = 1, Sample = 2, Event = 3 };

// Event ids (Record::code for Type::Event). Keep the numbers: they are in logs already written.
enum Event : uint8_t {
  EV_NONE = 0,
  EV_BATTERY_LOW = 1,        // arg = mV
  EV_BATTERY_CRITICAL = 2,   // arg = mV
  EV_BATTERY_RECOVERED = 3,  // voltage came back up (charging started); arg = mV
  EV_SYNC_OK = 4,            // Wi-Fi time sync worked; arg = drift in seconds (signed)
  EV_SYNC_FAIL = 5,
  EV_ALARM_RING = 6,         // the alarm started ringing
  EV_LOOP_STALL = 7,         // one loop took too long; arg = ms
  EV_HOST_CONNECTED = 8,     // a Serial monitor opened
  EV_LOG_FULL = 9,           // the file reached its limit: samples stop
  EV_DUMP_STARTED = 10,
};

// Sample flags (Record::flags).
enum Flag : uint16_t {
  FL_USB_VOLTAGE = 1 << 0,   // battery pin reads above a Li-ion cell's maximum: USB / charger voltage
  FL_HOST_OPEN = 1 << 1,     // a Serial monitor was open
  FL_TIME_VALID = 1 << 2,    // the wall clock had been synced
  FL_NO_BATTERY = 1 << 3,    // battery pin reads below BATTERY_MISSING_MV
};

struct Record {
  Type type = Type::None;
  uint16_t boot = 0;          // boot number since the log was last erased
  uint32_t uptimeS = 0;       // seconds since this boot
  uint32_t epoch = 0;         // UTC seconds (0 = clock not synced)
  uint16_t battMv = 0;
  uint16_t stackLeft = 0;     // bytes, loop task high-water mark
  uint32_t heapFree = 0;
  uint32_t heapMin = 0;       // lowest free heap since boot
  uint32_t heapMaxBlock = 0;  // largest free block now (fragmentation)
  uint16_t loopWorstMs = 0;   // longest gap between two loops since the last sample
  uint16_t renderWorstMs = 0; // worst screen render since boot
  uint16_t imuErrors = 0;     // MPU6050 read errors since boot
  uint8_t syncOk = 0, syncFail = 0;
  int16_t driftS = 0;         // last Wi-Fi clock drift, seconds
  uint8_t state = 0;          // utility::Machine state code
  uint8_t code = 0;           // Boot: reset reason; Event: Event id
  uint32_t arg = 0;           // Boot: previous run's last uptime (s); Event: its value
  uint16_t flags = 0;
};

uint8_t crc8(const uint8_t* data, size_t n);

void encode(const Record& r, uint8_t out[RECORD_BYTES]);
// False if the magic or CRC is wrong (a torn or empty record).
bool decode(const uint8_t in[RECORD_BYTES], Record& r);

// Names for the CSV (the reset reason is ESP-IDF's esp_reset_reason_t number).
const char* resetName(uint8_t reason);
const char* eventName(uint8_t id);
const char* typeName(Type t);

// True for resets that mean something went wrong (panic, watchdogs, brownout...).
bool resetIsAbnormal(uint8_t reason);

// "R,idx,type,boot,..." one line, no newline. Returns the length (0 if the buffer is too small).
// csvHeader() is the matching column line.
const char* csvHeader();
size_t formatCsv(const Record& r, uint32_t index, char* buf, size_t n);

// ---- decisions ----

struct Policy {
  uint32_t sampleMs = 60000;       // one sample per minute...
  uint32_t sampleFastMs = 15000;   // ...every 15 s once the battery is critical
  uint32_t flushMs = 600000;       // RAM -> flash every 10 min while healthy
  uint32_t lowMv = 3500;
  uint32_t criticalMv = 3300;
  uint32_t recoverMv = 150;        // this far above the lowest reading counts as charging
  size_t bufferRecords = 16;
};

// How long to wait before the next sample.
uint32_t sampleInterval(const Policy& p, uint32_t battMv);

// Should the RAM buffer be written to flash now? Events flush at once (they are rare and the
// reason for the log); below the low-battery line every record flushes, because the cell may die at any time.
bool flushDue(const Policy& p, size_t pending, bool pendingHasEvent, uint32_t nowMs, uint32_t lastFlushMs,
              uint32_t battMv);

// Watches the battery voltage and says when it crosses the low / critical line or starts recovering.
class BatteryWatch {
 public:
  explicit BatteryWatch(const Policy& p);
  // Returns EV_BATTERY_LOW, EV_BATTERY_CRITICAL, EV_BATTERY_RECOVERED, or EV_NONE. Ignore 0 (no reading yet).
  Event update(uint32_t mv);
  bool low() const { return level_ >= 1; }

 private:
  Policy p_;
  int level_ = 0;          // 0 ok, 1 low, 2 critical, 3 charging (recovered, still under the low line)
  uint32_t lowest_ = 0;    // lowest reading while low
};

}  // namespace soak
