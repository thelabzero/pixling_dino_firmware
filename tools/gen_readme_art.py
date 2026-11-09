#!/usr/bin/env python3
"""Pixel art for the README, generated from the SAME bitmaps as the boot film (so it cannot drift from the brand).

Reads the "Pixling" wordmark, the Dino sprite and its four faces from firmware/src/boot/boot_film.cpp and writes two
SVGs into art/readme/:  banner.svg (Dino + Pixling wordmark) and moods.svg (Dino's four faces).
Brand colours: Night #282020, cream #FCF8F0, teal #2F9083. Whole cells, crisp edges, no curves, own dark background
(so it looks the same in GitHub's light and dark themes). Deterministic. Run from the repo root:
    python3 tools/gen_readme_art.py
"""
import re, sys, pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
FILM = ROOT / "firmware/src/boot/boot_film.cpp"
OUT = ROOT / "art/readme"
NIGHT, CREAM, TEAL, CARD, MUTED = "#282020", "#FCF8F0", "#2F9083", "#332a2a", "#C9C0B4"


def block(src, name):
    m = re.search(r"static const char\* const %s\[[^\]]*\](?:\[[^\]]*\])? = \{(.*?)\n\};" % name, src, re.S)
    if not m:
        sys.exit("bitmap %s not found in boot_film.cpp" % name)
    return m.group(1)


def rows(text):
    return re.findall(r'"([.#w]+)"', text)


def runs(rs, ch="#", skip=lambda x, y: False):
    d = []
    for y, r in enumerate(rs):
        x = 0
        while x < len(r):
            if r[x] == ch and not skip(x, y):
                s = x
                while x < len(r) and r[x] == ch and not skip(x, y):
                    x += 1
                d.append("M%d %dh%dv1h-%dz" % (s, y, x - s, x - s))
            else:
                x += 1
    return "".join(d)


src = FILM.read_text()
word = rows(block(src, "WORD"))
dino = rows(block(src, "DINO"))
faces = [rows(g) for g in re.findall(r"\{(.*?)\}", block(src, "FACE"), re.S)]
assert len(word) == 12 and all(len(r) == 46 for r in word)
assert len(dino) == 23 and all(len(r) == 24 for r in dino)
assert len(faces) == 4 and all(len(f) == 8 for f in faces)
win_row = next(i for i, r in enumerate(dino) if "w" in r)

body = runs(dino, "#")
letters = runs(word, "#", skip=lambda x, y: x in (9, 10) and y < 2)  # the dot of the first i is drawn in teal


def face_path(f):
    d = ""
    for y, r in enumerate(f):
        for x, ch in enumerate(r):
            if ch == "#":
                d += "M%d %dh1v1h-1z" % (x + 6, y + win_row)
    return d


def dino_at(x, y, s, face):
    return ('<g transform="translate(%d %d) scale(%d)" fill="%s"><path d="%s"/><path d="%s"/></g>'
            % (x, y, s, CREAM, body, face_path(faces[face])))


def svg(w, h, title, inner):
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" role="img" aria-label="%s" '
            'shape-rendering="crispEdges"><title>%s</title><rect width="%d" height="%d" fill="%s"/>%s</svg>\n'
            % (w, h, title, title, w, h, NIGHT, inner))


# ---- banner: Dino (open eyes) and the Pixling wordmark, both at 2x, a teal ground line, a few stars
W, H, S = 176, 64, 2
stars = [(8, 8), (30, 4), (150, 6), (166, 18), (120, 52), (14, 40), (162, 44), (92, 4)]
inner = "".join('<rect x="%d" y="%d" width="2" height="2" fill="%s"/>' % (x, y, TEAL if i % 3 == 0 else MUTED)
                for i, (x, y) in enumerate(stars))
inner += dino_at(14, 8, S, 0)
wx, wy = 70, 8 + (23 * S - 12 * S) // 2
inner += ('<g transform="translate(%d %d) scale(%d)"><path fill="%s" d="%s"/><rect x="9" y="0" width="2" height="2" fill="%s"/></g>'
          % (wx, wy, S, CREAM, letters, TEAL))
inner += '<rect x="0" y="%d" width="%d" height="2" fill="%s"/>' % (H - 4, W, TEAL)
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "banner.svg").write_text(svg(W, H, "dino, a Pixling", inner))

# ---- moods: the four faces of the sprite, each on its own card
names = ["eyes open", "half blink", "blink", "happy"]
CW, CH, GAP, PAD = 56, 66, 6, 6
W2, H2 = PAD * 2 + 4 * CW + 3 * GAP, PAD * 2 + CH
inner = ""
for i, n in enumerate(names):
    x = PAD + i * (CW + GAP)
    inner += '<rect x="%d" y="%d" width="%d" height="%d" fill="%s"/>' % (x, PAD, CW, CH, CARD)
    inner += dino_at(x + 4, PAD + 6, 2, i)
    inner += ('<text x="%d" y="%d" fill="%s" font-family="ui-monospace,Menlo,Consolas,monospace" font-size="5" text-anchor="middle">%s</text>'
              % (x + CW // 2, PAD + CH - 4, MUTED, n))
(OUT / "moods.svg").write_text(svg(W2, H2, "Dino's faces: eyes open, half blink, blink, happy", inner))
print("wrote", OUT / "banner.svg", "and", OUT / "moods.svg")
