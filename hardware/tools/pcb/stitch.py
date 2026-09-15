"""Ground stitching vias and plane taps.

The three ground pours and the 3V3 plane are separate copper until something
joins them, and a pad on one side has no path to a plane on another layer.
This drops a via wherever both outer layers are free ground, and one tap
beside every pad that only the plane can reach.
"""
import json, math, sys
import numpy as np
from route import (GRID, ORIGIN, W, H, F, M, B, NL, dilate, polygon_mask,
                   pad_mask, to_cell, to_mm, disk_offsets)

geom = json.load(open(sys.argv[1]))
tracks = json.load(open(sys.argv[2]))
out_path = sys.argv[3]
PITCH = float(sys.argv[4]) if len(sys.argv) > 4 else 4.0

VIA_D, VIA_CLEAR = 0.8, 0.2
r_via = int(math.ceil((VIA_D / 2 + VIA_CLEAR) / GRID))

occupied = [np.zeros((H, W), dtype=bool) for _ in range(NL)]
gnd = [np.zeros((H, W), dtype=bool) for _ in range(NL)]

inside = polygon_mask(geom['outline'])
routable = ~dilate(~inside, 5)

for ko in geom['keepouts']:
    if ko['vias']:
        continue
    m = polygon_mask(ko['pts'])
    for l in range(NL):
        occupied[l] |= m

for pad in geom['pads']:
    pm = pad_mask(pad)
    if pm is None:
        continue
    y0, y1, x0, x1, m = pm
    block = np.zeros((H, W), dtype=bool)
    block[y0:y1 + 1, x0:x1 + 1] = m
    for l in range(NL):
        occupied[l] |= block
    if pad['net'] == 'GND':
        for l in range(NL):
            gnd[l] |= block

LAYER_IDX = {'F.Cu': F, 'In2.Cu': M, 'B.Cu': B}
for s in tracks['segments']:
    l = LAYER_IDX[s['layer']]
    ax, ay = to_cell(*s['start'])
    bx, by = to_cell(*s['end'])
    n = max(abs(bx - ax), abs(by - ay), 1)
    for i in range(n + 1):
        x = ax + (bx - ax) * i // n
        y = ay + (by - ay) * i // n
        if 0 <= x < W and 0 <= y < H:
            occupied[l][y, x] = True
for v in tracks['vias']:
    x, y = to_cell(*v['at'])
    for dx, dy in disk_offsets(4):
        nx, ny = x + dx, y + dy
        if 0 <= nx < W and 0 <= ny < H:
            for l in range(NL):
                occupied[l][ny, nx] = True

blocked = [dilate(occupied[l], r_via) | ~routable for l in range(NL)]
free_both = ~blocked[0] & ~blocked[1] & ~blocked[2]

stitches = []
step = int(round(PITCH / GRID))
for iy in range(step // 2, H, step):
    for ix in range(step // 2, W, step):
        if free_both[iy, ix]:
            x, y = to_mm(ix, iy)
            stitches.append({'net': 'GND', 'at': [round(x, 3), round(y, 3)]})
            for dx, dy in disk_offsets(r_via):
                nx, ny = ix + dx, iy + dy
                if 0 <= nx < W and 0 <= ny < H:
                    free_both[ny, nx] = False

json.dump({'vias': stitches}, open(out_path, 'w'))
print('%d ground stitching vias on a %.1f mm grid' % (len(stitches), PITCH))
