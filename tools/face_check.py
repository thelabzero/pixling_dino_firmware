#!/usr/bin/env python3
"""Check the C++ dino face rig against the design's reference frames.

Compiles tools/face_native.cpp + firmware/src/face + firmware/src/scenes/dino_clips.cpp on the desktop,
renders every frame it has (groups R, L, U so far) and compares pixels and hold times with
art/reference/dino-v1/ (dino_frames.bin, dino_timing.json). Runs once with double and once with
float maths (the firmware uses float). Exit code 0 only if everything matches.

    python3 tools/face_check.py            # check
    python3 tools/face_check.py --png out  # also write what the rig drew, one PNG per differing frame
"""
import argparse, json, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "firmware", "src")
REF = os.path.join(ROOT, "art", "reference", "dino-v1")
SRCS = ["face/face.cpp", "scenes/dino_clips.cpp"]
W, H = 128, 64


def compiler():
    for c in ("c++", "g++", "clang++"):
        if shutil.which(c):
            return [c]
    sys.exit("no C++ compiler found (xcode-select --install)")


def load_ref():
    t = json.load(open(os.path.join(REF, "dino_timing.json")))
    raw = open(os.path.join(REF, "dino_frames.bin"), "rb").read()
    frames, ms, names = [], [], []
    for c in t["clips"]:
        for k, f in enumerate(c["frames"]):
            i = f["index"]
            frames.append("".join("1" if b else "0" for b in raw[i * W * H:(i + 1) * W * H]))
            ms.append(f["ms"]); names.append(f'{c["id"]} {c["key"]} #{k}')
    return frames, ms, names


def run(real, tmp):
    exe = os.path.join(tmp, "face_" + real)
    cmd = compiler() + ["-O2", "-std=c++17", "-Wall", "-Wextra", "-DRIG_REAL=" + real, "-o", exe,
                        os.path.join(ROOT, "tools", "face_native.cpp")] + [os.path.join(SRC, s) for s in SRCS]
    subprocess.run(cmd, check=True)
    out = subprocess.run([exe], check=True, capture_output=True, text=True)
    sys.stderr.write("  " + real + ": " + out.stderr)
    return [(int(l.split()[1]), l.split()[2]) for l in out.stdout.splitlines()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--png", help="directory for PNGs of differing frames (needs Pillow)")
    args = ap.parse_args()
    ref, ms, names = load_ref()
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        for real in ("double", "float"):
            got = run(real, tmp)
            n = len(got)
            if n > len(ref):
                print(f"  {real}: {n} frames, reference has only {len(ref)}"); ok = False; continue
            bad = [(i, sum(a != b for a, b in zip(bits, ref[i])), t) for i, (t, bits) in enumerate(got)
                   if bits != ref[i] or t != ms[i]]
            if bad:
                ok = False
                print(f"  {real}: {len(bad)} of {n} frames differ")
                for i, d, t in bad[:15]:
                    print(f"    frame {i} ({names[i]}): {d} px differ, hold {t} ms (expected {ms[i]})")
                if args.png and real == "float":
                    from PIL import Image
                    os.makedirs(args.png, exist_ok=True)
                    for i, d, t in bad:
                        a = Image.new("L", (W, H)); a.putdata([255 if c == "1" else 0 for c in got[i][1]])
                        b = Image.new("L", (W, H)); b.putdata([255 if c == "1" else 0 for c in ref[i]])
                        s = Image.new("L", (W * 2 + 4, H), 80); s.paste(a, (0, 0)); s.paste(b, (W + 4, 0))
                        s.resize((s.width * 3, s.height * 3), Image.NEAREST).save(os.path.join(args.png, f"frame_{i:03d}.png"))
            else:
                print(f"  {real}: all {n} frames match (of {len(ref)} in the design; games and friends not ported yet)")
    sys.exit(0 if ok else 1)


main()
