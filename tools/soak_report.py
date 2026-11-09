#!/usr/bin/env python3
"""Turn a soak dump (tools/soak_dump.py) into a report: python3 tools/soak_report.py soak_logs/soak_<...>.csv

Every number is read from the log; anything that is a guess is labelled "estimate" or "assumption".
Sections: runs and resets, battery discharge (hours to each voltage, the dino's own % curve vs. the measured one),
memory (heap trend per hour, lowest heap, stack), timing (longest loop gap, stalls, render), Wi-Fi sync, events.
"""
import sys, csv, datetime, statistics, collections

IST = datetime.timezone(datetime.timedelta(hours=5, minutes=30))
def load_curve():
    """The firmware's own curve (firmware/src/power/battery_level.cpp: 21 mV points = 0, 5 ... 100 %), or None."""
    try:
        import re, pathlib
        src = (pathlib.Path(__file__).resolve().parent.parent / "firmware/src/power/battery_level.cpp").read_text()
        body = re.search(r"MV\[\]\s*=\s*\{([^}]*)\}", src).group(1)
        mvs = [int(x) for x in re.findall(r"\d{4}", body)]
        return [(mv, i * 5) for i, mv in enumerate(mvs)] if len(mvs) == 21 else None
    except Exception:
        return None


def fw_percent(curve, mv):
    if not curve:
        return None
    if mv <= curve[0][0]:
        return 0
    for (a, pa), (b, pb) in zip(curve, curve[1:]):
        if mv < b:
            return pa + (pb - pa) * (mv - a) / (b - a)
    return 100


def parse(path):
    header, rows, meta = None, [], {}
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.rstrip("\n")
        if line.startswith("SOAKDUMP BEGIN"):
            meta["begin"] = line
        elif line.startswith("SOAKDUMP COLUMNS"):
            header = line.split(" ", 2)[2].split(",")
        elif line.startswith("SOAKDUMP STATES"):
            meta["states"] = dict(kv.split("=") for kv in line.split(" ", 2)[2].split(","))
        elif line.startswith("SOAKDUMP END"):
            meta["end"] = line
        elif line.startswith("R,") and header:
            rows.append(dict(zip(header, line.split(",")[1:])))
    for r in rows:
        for k, v in list(r.items()):
            if k not in ("type", "name"):
                try:
                    r[k] = int(v)
                except ValueError:
                    pass
    return meta, rows


def fmt_t(secs):
    return f"{secs // 3600}h{(secs % 3600) // 60:02d}m"


def clock(r):
    return datetime.datetime.fromtimestamp(r["epoch"], IST).strftime("%d %b %Y %H:%M IST") if r["epoch"] else "clock not synced"


def slope(xs, ys):  # least squares, y per x
    if len(xs) < 3:
        return None
    mx, my = statistics.fmean(xs), statistics.fmean(ys)
    den = sum((x - mx) ** 2 for x in xs)
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / den if den else None


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    meta, rows = parse(sys.argv[1])
    if not rows:
        sys.exit("no records in that file")
    if "end" not in meta:
        print("WARNING: the dump has no END line: it was cut short\n")
    states = meta.get("states", {})
    curve = load_curve()
    runs = collections.OrderedDict()
    for r in rows:
        runs.setdefault(r["boot"], []).append(r)

    print(f"# Soak report: {sys.argv[1]}\n")
    print(f"Source: dump of the dino's flash log. {meta.get('begin', '')}")
    print(f"Records: {len(rows)} in {len(runs)} run(s) (a run = one boot). Samples every 60 s (15 s once the battery is critical).\n")

    print("## 1. Runs and resets (fact)\n")
    print("| boot | started | reset reason | batt at start | lasted (last record) | ended how |")
    print("|---|---|---|---|---|---|")
    abnormal = 0
    for b, rs in runs.items():
        first = rs[0]
        last = max(rs, key=lambda r: r["uptime_s"])
        reason = first["name"] if first["type"] == "boot" else "(boot record missing)"
        if reason in ("panic", "interrupt_wdt", "task_wdt", "other_wdt", "brownout", "power_glitch", "cpu_lockup"):
            abnormal += 1
        nxt = runs.get(b + 1)
        if nxt and nxt[0]["type"] == "boot":
            prev_up = nxt[0]["arg"]
            ended = f"next boot says it reached {fmt_t(prev_up)} ({nxt[0]['name']})"
        else:
            ended = "last run (still going, or the cell died after its last write)"
        print(f"| {b} | {clock(first)} | {reason} | {first['batt_mv']} mV | {fmt_t(last['uptime_s'])} | {ended} |")
    print(f"\nAbnormal resets (panic, watchdog, brownout): **{abnormal}**\n")

    samples = [r for r in rows if r["type"] == "sample"]
    ev = [r for r in rows if r["type"] == "event"]

    print("## 2. Battery (fact from the log; the % curve is an estimate)\n")
    batt = [r for r in samples if r["batt_mv"] and not (r["flags"] & 1) and not (r["flags"] & 8)]  # not USB voltage, not no-battery
    if len(batt) < 5:
        print("Too few battery readings (USB-only run, or no battery connected).\n")
    else:
        # use the longest run for the discharge curve
        by_run = collections.defaultdict(list)
        for r in batt:
            by_run[r["boot"]].append(r)
        b, rs = max(by_run.items(), key=lambda kv: kv[1][-1]["uptime_s"] - kv[1][0]["uptime_s"])
        t0 = rs[0]["uptime_s"]
        total = rs[-1]["uptime_s"] - t0
        print(f"Longest discharge: boot {b}, from {rs[0]['batt_mv']} mV to {rs[-1]['batt_mv']} mV in **{fmt_t(total)}** ({clock(rs[0])}).\n")
        print("Runtime so far to each voltage (first time the reading was at or below it; USB charging not counted):\n")
        print("| volts | after | firmware's % there (estimate) | measured: share of the run used (assumption: constant load) |")
        print("|---|---|---|---|")
        for mv in (4100, 4000, 3900, 3800, 3700, 3600, 3500, 3400, 3300, 3200):
            hit = next((r for r in rs if r["batt_mv"] <= mv), None)
            if not hit:
                continue
            el = hit["uptime_s"] - t0
            pct = fw_percent(curve, mv)
            used = 100 * el / total if total else 0
            print(f"| {mv} | {fmt_t(el)} | {'' if pct is None else f'{pct:.0f} %'} | {used:.0f} % used, {100 - used:.0f} % left |")
        print("\nThe two right-hand columns should roughly agree. Where they differ, the firmware curve (`lipoPercent`) is off for this cell.")
        print("The 'measured' column treats time as charge, so it is only as good as the load was constant (screen, sounds and Wi-Fi vary).\n")
        sag = [r for r in rs if r["batt_mv"] < 3300]
        print(f"Readings below 3300 mV: {len(sag)}. Lowest logged: {min(r['batt_mv'] for r in rs)} mV.")
    usbs = [r for r in samples if r["flags"] & 1]
    if usbs:
        print(f"Samples with USB/charger voltage on the pin: {len(usbs)}.")
    print()

    print("## 3. Memory (fact)\n")
    print("| boot | heap free first -> last | lowest heap ever | trend (KB/hour) | largest free block, min | stack left, min |")
    print("|---|---|---|---|---|---|")
    leak = False
    for b, rs in runs.items():
        ss = [r for r in rs if r["type"] == "sample"]
        if len(ss) < 3:
            continue
        hrs = [r["uptime_s"] / 3600 for r in ss]
        s = slope(hrs, [r["heap_free"] / 1024 for r in ss])
        smin = slope(hrs, [r["heap_min"] / 1024 for r in ss])
        if smin is not None and smin < -1 and (ss[-1]["uptime_s"] - ss[0]["uptime_s"]) > 3600:
            leak = True
        print(f"| {b} | {ss[0]['heap_free']} -> {ss[-1]['heap_free']} | {min(r['heap_min'] for r in ss)} | "
              f"{'' if s is None else f'{s:+.2f}'} | {min(r['heap_maxblk'] for r in ss)} | {min(r['stack_left'] for r in ss)} |")
    print("\nPass line (from the soak plan): lowest-heap trend flat (above about -1 KB/hour), stack left above 2048 B.")
    print(f"Heap leak suspected: **{'YES' if leak else 'no'}**\n")

    print("## 4. Timing (fact)\n")
    worst = max(samples, key=lambda r: r["loop_worst_ms"])
    stalls = [r for r in ev if r["name"] == "loop_stall"]
    print(f"Longest loop gap in any one-minute window: **{worst['loop_worst_ms']} ms** (boot {worst['boot']}, up {fmt_t(worst['uptime_s'])}).")
    print(f"Loop stalls over 300 ms logged: {len(stalls)}" + (" (worst " + str(max(r['arg'] for r in stalls)) + " ms)" if stalls else "") + ".")
    print(f"Worst screen render: {max(r['render_worst_ms'] for r in samples)} ms. IMU read errors (max over runs): {max(r['imu_err'] for r in samples)}.\n")

    print("## 5. Wi-Fi clock sync (fact)\n")
    last = samples[-1]
    print(f"Syncs ok / failed in the last run: {last['sync_ok']} / {last['sync_fail']}; last drift {last['drift_s']} s.\n")

    print("## 6. Events (fact)\n")
    print("| boot | up | clock | event | value |")
    print("|---|---|---|---|---|")
    for r in ev:
        if r["name"] in ("host_connected", "dump_started"):
            continue
        print(f"| {r['boot']} | {fmt_t(r['uptime_s'])} | {clock(r)} | {r['name']} | {r['arg']} |")
    print("\n## 7. States seen at samples\n")
    cnt = collections.Counter(states.get(str(r["state"]), r["state"]) for r in samples)
    print(", ".join(f"{k}: {v}" for k, v in cnt.most_common()))


if __name__ == "__main__":
    main()
