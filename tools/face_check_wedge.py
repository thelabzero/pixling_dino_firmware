#!/usr/bin/env python3
"""Check the wedge face of the firmware (DINO_WEDGE) against the wedge sheet's rig.

Builds the sheet rig (art/reference/dino-wedge-v1/rig_wedge.js, via node) and the C++ rig with -DDINO_WEDGE
(tools/face_native.cpp + firmware/src/face + scenes/dino_clips.cpp), then compares every frame the C++ has
(groups R, L, U) pixel by pixel and by hold time, with double and with float maths.
The default (non-wedge) face is still checked by tools/face_check.py.

    python3 tools/face_check_wedge.py
"""
import json, os, subprocess, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import face_check as fc

WEDGE = os.path.join(fc.ROOT, "art", "reference", "dino-wedge-v1")
JS = ("const {build}=require(process.argv[1]);let o=[];"
      "for(const G of build('10:24','SAM'))for(const s of G.scenes)for(const f of s.frames)"
      "o.push(s.key+' '+(f[3]||f[1]||1000)+' '+Array.from(f[0]).join(''));console.log(o.join('\\n'));")


def main():
    ref = [l.split() for l in subprocess.run(["node", "-e", JS, os.path.join(WEDGE, "rig_wedge.js")], check=True,
                                             capture_output=True, text=True).stdout.splitlines()]
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        for real in ("double", "float"):
            exe = os.path.join(tmp, "wedge_" + real)
            subprocess.run(fc.compiler() + ["-O2", "-std=c++17", "-Wall", "-Wextra", "-DDINO_WEDGE=1", "-DRIG_REAL=" + real, "-o", exe,
                           os.path.join(fc.ROOT, "tools", "face_native.cpp")] + [os.path.join(fc.SRC, s) for s in fc.SRCS], check=True)
            out = subprocess.run([exe], check=True, capture_output=True, text=True).stdout.splitlines()
            got = [l.split() for l in out]
            bad = [(i, g[0], sum(a != b for a, b in zip(g[2], ref[i][2])), g[1], ref[i][1]) for i, g in enumerate(got)
                   if i >= len(ref) or g[2] != ref[i][2] or g[1] != ref[i][1]]
            if bad:
                ok = False
                print(f"  {real}: {len(bad)} of {len(got)} frames differ")
                for i, k, d, t, e in bad[:15]:
                    print(f"    frame {i} ({k}): {d} px differ, hold {t} ms (expected {e})")
            else:
                print(f"  {real}: all {len(got)} frames match the wedge sheet (of {len(ref)}; games and friends not ported yet)")
    sys.exit(0 if ok else 1)


main()
