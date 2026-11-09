# ADR 007: Soak log (the dino's black box)

Status: accepted 2026-11-09 (owner: run the battery until it is dead, plug in, read everything back).

## Decision
- The dino keeps its own log **in flash** (LittleFS file `/soak.bin`), not only on Serial. Reason (fact from Run 1): serial lines are lost on reset or when USB is unplugged, arrive up to 8 s late, and nothing recorded *why* a reset happened. A battery run has nobody watching a monitor, so the log must survive a dead cell and be read afterwards.
- **48-byte fixed records with a CRC8** (`src/soak/soak_record.*`, pure, `test_soak`). Three kinds: `boot` (reset reason, boot number, how long the previous run lasted), `sample` (one a minute: battery mV, free heap, lowest heap, largest free block, stack left, longest loop gap, worst render, IMU errors, Wi-Fi syncs and drift, state, flags) and `event` (battery_low / critical / recovered, loop_stall, alarm_ring, sync_ok / fail, host_connected, log_full). A torn last write fails its CRC and is skipped; the file stays aligned.
- **Flash wear:** records are buffered in RAM (16) and written every 10 min while healthy; events flush at once; below the low-battery line (3500 mV) every record flushes, so the last minutes before the cell dies are on flash. Samples go to 15 s below 3300 mV. About 69 KB a day at one sample a minute (fact: 48 B x 1440); the file stops at 12,000 records (576 KB, about 8 days) with a `log_full` event.
- **Reset reason** comes from `esp_reset_reason()`. A small RAM memo (`RTC_NOINIT`) carries the last uptime across panics and watchdog resets; after a power cut or dead battery the last flushed sample says how far it got.
- **Dump:** key `w`, or by itself the first time a Serial monitor connects after a boot (`SOAK_AUTO_DUMP`). Plain text, `SOAKDUMP BEGIN ... END`, one CSV line per record; non-blocking (a few lines per loop, only while a monitor is open). `x` twice erases. `tools/soak_dump.py` saves a dump to `soak_logs/`, `tools/soak_report.py` analyses it.
- The log never changes behaviour: no low-battery face or mute (not asked for); it only records.

## Known limits (labelled)
- **No multimeter, so `BATTERY_CAL` is still 1.00 (fact).** Absolute millivolts may be a few percent off (estimate). Runtime in hours, the shape of the curve and the order of events are still valid. Voltage thresholds (3500 / 3300 mV) are generic Li-ion values (assumption).
- A cell with a protection board cuts power abruptly (assumption: about 2.5-3.0 V); the last record is then whatever was flushed last (at most 60 s old below 3500 mV).
- No current sensor: the log gives hours of runtime, not mA. Any mA figure would be an estimate.
- The "measured %" in the report treats elapsed time as charge used, which holds only if the load stayed constant (assumption).
- LittleFS on the default partition table (`spiffs`, 1.4 MB): not yet run on the board. First boot formats it (a second or two).
- USB-only power cannot be told from "charging on a charger module" by software; the log flags a pin voltage above `BATTERY_MAX_MV` as USB / charger voltage.

## Consequences
- Fact: 120 native tests pass (9 new), `dino_c3` builds with no warnings, flash 80.6% (was 76.2%; LittleFS and the module cost about 55 KB).
- Firmware 0.3.1. New keys `w`, `x`.
