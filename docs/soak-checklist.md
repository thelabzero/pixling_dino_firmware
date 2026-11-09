# Soak checklist: battery run to empty (firmware 0.3.1)

Goal: charge full, run on the battery until it dies, plug in, read the whole log. The dino records everything itself (ADR 007); you only start it and collect it. Flash and monitor are done by hand.

## 0. Flash and check the log works (5 min, USB)
```
cd firmware
.venv/bin/pio run -e dino_c3 -t upload && .venv/bin/pio device monitor -e dino_c3 -f time
```
- Boot prints `[soak] log: 0 records in flash, boot #1, reset reason power_on` (first boot formats the flash file system: a second or two). `[app] soak on` in the module list.
- Wait 5 s: a `[soak] up ...` line. Press `i`: a `[soak] N records in flash` line. Press `w`: a `SOAKDUMP BEGIN ... SOAKDUMP END` block with a few rows.
- Check the Wi-Fi time syncs (the log stamps each record with the real clock only after it).
- Press `x`, then `x` again within 5 s: `[soak] log erased`. **Do this so the run starts with a clean log.**
- Anything wrong: paste the monitor output and we fix only that.

## 1. The run
1. Charge until the charger's light is **blue** (full). Note the time.
2. Unplug USB. The dino must be on the battery only (with USB the log would show USB voltage).
3. Power on. Boot film plays, home face shows. Wi-Fi must be in range for the first minute so the clock syncs (then the log has real times).
4. Leave it alone on the home screen, **do not touch it**: this is the idle baseline. Write down the start time.
5. Leave it until the screen is dead. (A second run with taps, alarms and sounds can follow; keep it separate.)

## 2. Collect
1. Plug USB into the Mac (charger on). The dino boots from USB.
2. Close any serial monitor, then from the repo root:
```
firmware/.venv/bin/python tools/soak_dump.py
python3 tools/soak_report.py soak_logs/<the file it saved>
```
3. Send me the report (or the file). Do **not** erase the log until we have analysed it (`x` twice erases).

## What the report answers
| Question | Where |
|---|---|
| How many hours did one charge last (idle)? | 2. Battery: "Longest discharge" |
| Is the dino's battery percent right? | 2. the % columns compared |
| Did it reset on its own, and why? | 1. Runs and resets (reset reason per boot) |
| Does memory leak over the run? | 3. Memory (trend per hour, lowest heap, stack) |
| Did the loop stall (alarm exactness)? | 4. Timing |
| What happened near the end? | 6. Events (battery_low, battery_critical, last samples) |

## Pass lines (set before the run)
- No abnormal reset (panic, watchdog, brownout) until the battery is nearly empty. A brownout at the very end is expected and is data, not a failure.
- Lowest-heap trend flat (above about -1 KB/hour) and stack left above 2 KB.
- No loop stall over 300 ms while idle (anything the report lists is worth a look).
- The last record is no more than 60 s before the end of the run (shows the log kept up).

## Known limits
No multimeter, so the voltage is uncalibrated (`BATTERY_CAL` 1.00): hours and curve shape are reliable, absolute mV may be a few % off (estimate). See ADR 007.
