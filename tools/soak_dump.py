#!/usr/bin/env python3
"""Fetch the dino's soak log over USB and save it to soak_logs/soak_<date>_<time>.csv (ADR 007).

Plug the dino into the Mac (after the battery run), then, from the repo root:
    firmware/.venv/bin/python tools/soak_dump.py            # finds the port by itself
    firmware/.venv/bin/python tools/soak_dump.py /dev/cu.usbmodem1101
The dino also sends the dump by itself when a monitor connects, so this tool just waits for it and, if it sees
nothing for a few seconds, sends the "w" key. Close any other serial monitor first (only one program can hold the port).
It never erases the log. To start a fresh log afterwards, type "x" twice in a monitor.
Needs pyserial (it is in firmware/.venv, installed with PlatformIO).
"""
import sys, time, glob, datetime, pathlib

try:
    import serial
except ImportError:
    sys.exit("pyserial missing: run with firmware/.venv/bin/python")

OUT_DIR = pathlib.Path(__file__).resolve().parent.parent / "soak_logs"


def find_port():
    ports = sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*") + glob.glob("COM*"))
    if not ports:
        sys.exit("no dino found on USB (looked for /dev/cu.usbmodem*). Plug it in, or pass the port.")
    return ports[0]


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    print(f"opening {port} ...")
    ser = serial.Serial(port, 115200, timeout=0.5)
    ser.dtr = True  # a monitor "opening" the port: the dino sees it and may dump by itself
    lines, state, started, last_rx, asked = [], "wait", time.time(), time.time(), False
    while True:
        raw = ser.readline()
        now = time.time()
        if raw:
            last_rx = now
            line = raw.decode("utf-8", "replace").rstrip()
            if line.startswith("SOAKDUMP BEGIN"):
                lines, state = [line], "dump"   # a later BEGIN replaces an earlier, unfinished one
                print("dump started ...")
            elif state == "dump":
                lines.append(line)
                if line.startswith("SOAKDUMP END"):
                    break
            elif line.startswith("[soak]"):
                print(line)
        if state == "wait" and not asked and now - started > 6:
            print("no dump yet: asking for it (w)")
            ser.write(b"w")
            asked = True
        if state == "wait" and now - started > 30:
            sys.exit("nothing came: is the dino running firmware 0.3.1+ with FEATURE_SOAK_LOG, and the monitor closed?")
        if state == "dump" and now - last_rx > 15:
            sys.exit("the dump stopped half way (USB unplugged?). Nothing saved; try again.")
    OUT_DIR.mkdir(exist_ok=True)
    name = OUT_DIR / datetime.datetime.now().strftime("soak_%Y-%m-%d_%H%M%S.csv")
    name.write_text("\n".join(lines) + "\n")
    print(f"saved {len(lines)} lines to {name}")
    print(f"next:  python3 tools/soak_report.py {name}")


if __name__ == "__main__":
    main()
