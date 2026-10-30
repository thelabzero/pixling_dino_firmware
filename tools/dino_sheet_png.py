#!/usr/bin/env python3
"""Stacks every frame of art/reference/dino-v1/dino_frames.bin into dino_frames.png (128x64 each,
1-bit, top to bottom) and a 3x contact sheet per group. Run from the repo root after
export_dino_reference.js. Deterministic."""
import json, sys
from PIL import Image
D = 'art/reference/dino-v1/'
t = json.load(open(D + 'dino_timing.json'))
W, H, N = t['width'], t['height'], t['frames']
raw = open(D + 'dino_frames.bin', 'rb').read()
assert len(raw) == W * H * N, 'frame data size mismatch'
img = Image.new('1', (W, H * N))
img.putdata([255 if b else 0 for b in raw])
img.save(D + 'dino_frames.png')
cols = 8; sc = 3
for g in sorted({c['group'] for c in t['clips']}):
    idx = [f['index'] for c in t['clips'] if c['group'] == g for f in c['frames']]
    rows = (len(idx) + cols - 1) // cols
    sheet = Image.new('L', (cols * (W * sc + 6), rows * (H * sc + 6)), 60)
    for k, i in enumerate(idx):
        fr = Image.frombytes('L', (W, H), bytes(255 if b else 0 for b in raw[i*W*H:(i+1)*W*H])).resize((W*sc, H*sc), Image.NEAREST)
        sheet.paste(fr, ((k % cols) * (W*sc+6) + 3, (k // cols) * (H*sc+6) + 3))
    sheet.save(D + 'dino_contact_%s.png' % g)
print('wrote dino_frames.png and', len({c['group'] for c in t['clips']}), 'contact sheets')
