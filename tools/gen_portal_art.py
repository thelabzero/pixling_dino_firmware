#!/usr/bin/env python3
"""Pixel art for the Wi-Fi setup page, generated from the SAME bitmaps as the boot film.

Reads the "Pixling" wordmark, the Dino sprite and its face window out of firmware/src/boot/boot_film.cpp and
rewrites the two blocks between <!--logo-->...<!--/logo--> and <!--dino-->...<!--/dino--> in
firmware/src/drivers/wifi_portal.cpp (inline SVG: one <path> per bitmap, whole cells, crisp edges, no curves).
Deterministic: same bitmaps -> same output. Run from the repo root:  python3 tools/gen_portal_art.py
"""
import re, sys, pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
FILM = ROOT / "firmware/src/boot/boot_film.cpp"
PORTAL = ROOT / "firmware/src/drivers/wifi_portal.cpp"

def block(src, name):
    m = re.search(r"static const char\* const %s\[[^\]]*\](?:\[[^\]]*\])? = \{(.*?)\n\};" % name, src, re.S)
    if not m: sys.exit("bitmap %s not found in boot_film.cpp" % name)
    return m.group(1)

def rows(text):
    return re.findall(r'"([.#w]+)"', text)

def runs(rs, ch="#", skip=lambda x, y: False):
    """Horizontal runs of `ch` as SVG path commands: M x y h w v 1 h -w z (cells are 1 unit)."""
    d = []
    for y, r in enumerate(rs):
        x = 0
        while x < len(r):
            if r[x] == ch and not skip(x, y):
                s = x
                while x < len(r) and r[x] == ch and not skip(x, y): x += 1
                d.append("M%d %dh%dv1h-%dz" % (s, y, x - s, x - s))
            else:
                x += 1
    return "".join(d)

src = FILM.read_text()
word = rows(block(src, "WORD"))
dino = rows(block(src, "DINO"))
faces_txt = block(src, "FACE")
faces = [rows(g) for g in re.findall(r"\{(.*?)\}", faces_txt, re.S)]
assert len(word) == 12 and all(len(r) == 46 for r in word)
assert len(dino) == 23 and all(len(r) == 24 for r in dino)
assert len(faces) == 4 and all(len(f) == 8 for f in faces)
win_row = next(i for i, r in enumerate(dino) if "w" in r)

# logo: the letters (not the dot of the first i) + the dot as its own teal 2x2 block that hops
letters = runs(word, "#", skip=lambda x, y: x in (9, 10) and y < 2)
logo = ('<svg class="logo" viewBox="0 0 46 12" role="img" aria-label="Pixling" shape-rendering="crispEdges">'
        '<path fill="currentColor" d="%s"/><rect class="dot" x="9" y="0" width="2" height="2"/></svg>' % letters)

# dino: body = '#'; the face window is dark; one face is shown at a time (open / half / closed / happy)
body = runs(dino, "#")
names = ["open", "half", "closed", "happy"]
face_paths = ""
for n, f in zip(names, faces):
    d = ""
    for y, r in enumerate(f):
        for x, ch in enumerate(r):
            if ch == "#": d += "M%d %dh1v1h-1z" % (x + 6, y + win_row)
    face_paths += '<path class="f-%s" fill="currentColor" d="%s"/>' % (n, d)
dino_svg = ('<svg class="dino" viewBox="0 0 24 23" role="img" aria-label="Dino" shape-rendering="crispEdges">'
            '<path fill="currentColor" d="%s"/>%s</svg>' % (body, face_paths))

page = PORTAL.read_text()
for tag, svg in (("logo", logo), ("dino", dino_svg)):
    pat = re.compile(r"<!--%s-->.*?<!--/%s-->" % (tag, tag), re.S)
    if not pat.search(page): sys.exit("marker <!--%s--> not found in wifi_portal.cpp" % tag)
    page = pat.sub(lambda m: "<!--%s-->%s<!--/%s-->" % (tag, svg, tag), page)
PORTAL.write_text(page)
print("portal art updated: logo %d bytes, dino %d bytes" % (len(logo), len(dino_svg)))
