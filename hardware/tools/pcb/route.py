"""Grid maze router for the Cicala Rev A board.

Runs under the system Python for numpy. Reads the geometry dumped by
export_geom.py and writes the tracks and vias it found as JSON.

Signals route on F.Cu and B.Cu; In1.Cu is a ground plane and In2.Cu is the
3V3 plane with a ground island, so a layer change is a through via.
"""
import json, heapq, math, sys, time
import numpy as np

GRID = 0.075
ORIGIN = (3.0, 3.5)
W, H = 1040, 654
# In1.Cu stays an unbroken ground plane; the other three carry signals, with
# ground poured on F.Cu and B.Cu and 3V3 poured on In2.Cu around them.
F, M, B = 0, 1, 2
NL = 3
LAYER_NAME = {F: 'F.Cu', M: 'In2.Cu', B: 'B.Cu'}

# DRC resolves a clearance as the larger of the two objects' rules, so the
# router has to plan for the widest rule on the board, not the net's own.
MAX_CLEARANCE = 0.20
MAX_HALF_WIDTH = 0.15
# Copper is modelled as cells, so its edge can lie up to half a cell beyond
# the outermost cell centre. Every clearance radius carries that slack.
GRID_SLACK = 1

POWER = {'GND', 'VBAT', 'VBAT_CELL', 'VSYS', '3V3', 'EPD_3V3', 'USB_VBUS',
         'Net-(U3-L1)', 'Net-(U3-L2)', 'EPD_SW'}
USB = {'USB_DP', 'USB_DN'}
HV = {'EPD_VGH', 'EPD_VGL', 'EPD_VSH1', 'EPD_VSH2', 'EPD_VSL', 'EPD_VCOM',
      'EPD_VDD', 'EPD_PUMP', 'EPD_SW', 'EPD_GDR', 'EPD_RESE'}

# Parts whose pin pitch leaves no room to route past: escape these first.
TIGHT = {'J2', 'U2', 'U3', 'U4', 'U5', 'U6', 'Q1', 'D4', 'U1', 'J1'}


# Nets that touch a 0.4 or 0.5 mm-pitch part have to leave it between its own
# neighbours: at 0.4 mm pitch only a 0.15 mm track clears 0.15 mm on each side.
FINE_PITCH = {'U2', 'U3', 'J2', 'U1'}
NARROW = set()


def rules(net):
    if net in NARROW:
        return 0.15, 0.15, 0.6
    if net in USB:
        return 0.29, 0.20, 0.6
    if net in POWER:
        # 0.3 mm at 1 oz carries about 1.2 A for a 10 K rise (IPC-2152); the
        # charge current is 250 mA and the system peak about 500 mA.
        return 0.30, 0.15, 0.8
    if net in HV:
        return 0.25, 0.20, 0.6
    return 0.20, 0.15, 0.6

def to_cell(x, y):
    return int(round((x - ORIGIN[0]) / GRID)), int(round((y - ORIGIN[1]) / GRID))

def to_mm(ix, iy):
    return ORIGIN[0] + ix * GRID, ORIGIN[1] + iy * GRID

XS = (np.arange(W) * GRID + ORIGIN[0])[None, :]
YS = (np.arange(H) * GRID + ORIGIN[1])[:, None]

def disk_offsets(r_cells):
    out = []
    for dy in range(-r_cells, r_cells + 1):
        for dx in range(-r_cells, r_cells + 1):
            if dx * dx + dy * dy <= r_cells * r_cells:
                out.append((dx, dy))
    return out

def dilate(mask, r_cells):
    if r_cells <= 0:
        return mask.copy()
    out = np.zeros_like(mask)
    for dx, dy in disk_offsets(r_cells):
        xs0, xs1 = max(0, dx), min(W, W + dx)
        ys0, ys1 = max(0, dy), min(H, H + dy)
        out[ys0:ys1, xs0:xs1] |= mask[ys0 - dy:ys1 - dy, xs0 - dx:xs1 - dx]
    return out

def polygon_mask(pts):
    """Even-odd scanline fill of a polygon given in mm."""
    m = np.zeros((H, W), dtype=bool)
    n = len(pts)
    for iy in range(H):
        y = ORIGIN[1] + iy * GRID
        xs = []
        for i in range(n):
            x1, y1 = pts[i]
            x2, y2 = pts[(i + 1) % n]
            if (y1 <= y < y2) or (y2 <= y < y1):
                t = (y - y1) / (y2 - y1)
                xs.append(x1 + t * (x2 - x1))
        xs.sort()
        for a, b in zip(xs[0::2], xs[1::2]):
            ia = max(0, int(math.ceil((a - ORIGIN[0]) / GRID)))
            ib = min(W - 1, int(math.floor((b - ORIGIN[0]) / GRID)))
            if ib >= ia:
                m[iy, ia:ib + 1] = True
    return m

def pad_mask(pad):
    """Cells covered by one pad, in its own frame."""
    cx, cy, w, h = pad['x'], pad['y'], pad['w'], pad['h']
    ang = math.radians(-pad['angle'])
    rad = math.hypot(w, h) / 2 + GRID
    x0, x1 = to_cell(cx - rad, 0)[0], to_cell(cx + rad, 0)[0]
    y0, y1 = to_cell(0, cy - rad)[1], to_cell(0, cy + rad)[1]
    x0, x1 = max(0, x0), min(W - 1, x1)
    y0, y1 = max(0, y0), min(H - 1, y1)
    if x1 < x0 or y1 < y0:
        return None
    gx = XS[0, x0:x1 + 1][None, :] - cx
    gy = YS[y0:y1 + 1, 0][:, None] - cy
    ca, sa = math.cos(ang), math.sin(ang)
    lx = gx * ca - gy * sa
    ly = gx * sa + gy * ca
    shape = pad['shape']
    if shape == 'circle':
        m = (lx ** 2 + ly ** 2) <= (w / 2) ** 2
    elif shape == 'oval':
        a, b = w / 2, h / 2
        m = (lx / a) ** 2 + (ly / b) ** 2 <= 1.0
    elif shape == 'roundrect':
        r = pad['r']
        ax = np.abs(lx) - (w / 2 - r)
        ay = np.abs(ly) - (h / 2 - r)
        ax = np.maximum(ax, 0)
        ay = np.maximum(ay, 0)
        m = (ax ** 2 + ay ** 2 <= r ** 2) & (np.abs(lx) <= w / 2) & (np.abs(ly) <= h / 2)
    else:
        m = (np.abs(lx) <= w / 2) & (np.abs(ly) <= h / 2)
    return (y0, y1, x0, x1, m)


def build(geom):
    """copper[layer]: net id per cell (0 free). hard[layer]: never routable."""
    nets = {}
    def nid(name):
        if name not in nets:
            nets[name] = len(nets) + 1
        return nets[name]

    copper = [np.zeros((H, W), dtype=np.int32) for _ in range(NL)]
    hard = [np.zeros((H, W), dtype=bool) for _ in range(NL)]
    no_via = [np.zeros((H, W), dtype=bool) for _ in range(NL)]

    inside = polygon_mask(geom['outline'])
    for hole in geom['outline_holes']:
        inside &= ~polygon_mask(hole)
    # 0.3 mm edge clearance is to the copper, so the centre line has to keep
    # its own half width on top of that
    edge_r = int(math.ceil((0.30 + MAX_HALF_WIDTH) / GRID)) + GRID_SLACK
    routable = ~dilate(~inside, edge_r)
    outside = ~inside
    for l in range(NL):
        hard[l] |= ~routable

    for ko in geom['keepouts']:
        m = polygon_mask(ko['pts'])
        for l in range(NL):
            if LAYER_NAME[l] in ko['layers'] and not ko['tracks']:
                hard[l] |= m
            if LAYER_NAME[l] in ko['layers'] and not ko['vias']:
                no_via[l] |= m

    pad_cells = {}
    for pad in geom['pads']:
        pm = pad_mask(pad)
        if pm is None:
            continue
        y0, y1, x0, x1, m = pm
        if pad['npth'] or (pad['drill'] > 0 and not pad['net']):
            # a bare hole blocks both layers
            hole = np.zeros((H, W), dtype=bool)
            hole[y0:y1 + 1, x0:x1 + 1] = m
            # hole-to-copper is 0.25 mm, plus the track's own half width
            hole = dilate(hole, int(math.ceil((0.25 + MAX_HALF_WIDTH) / GRID))
                          + GRID_SLACK)
            for l in range(NL):
                hard[l] |= hole
            continue
        if not pad['net']:
            # mechanical and shield pads carry no net but are still copper
            solid = np.zeros((H, W), dtype=bool)
            solid[y0:y1 + 1, x0:x1 + 1] = m
            layers = [l for l in (F, M, B) if LAYER_NAME[l] in pad['layers']]
            for l in (layers or [F, M, B]):
                hard[l] |= dilate(solid, int(math.ceil(
                    (MAX_CLEARANCE + MAX_HALF_WIDTH) / GRID)) + GRID_SLACK)
            continue
        n = nid(pad['net'])
        layers = [l for l in (F, M, B) if LAYER_NAME[l] in pad['layers']]
        if pad['drill'] > 0:
            layers = [F, M, B]
        for l in layers:
            sub = copper[l][y0:y1 + 1, x0:x1 + 1]
            sub[m] = n
        key = (pad['ref'], pad['pad'])
        pad_cells[key] = (pad['net'], n, layers, (y0, y1, x0, x1, m))
    return nets, copper, hard, no_via, outside, pad_cells


def cells_of(entry):
    y0, y1, x0, x1, m = entry[3]
    ys, xs = np.nonzero(m)
    return [( (y0 + int(a)) * W + (x0 + int(b)) ) for a, b in zip(ys, xs)]


STEP, DIAG, VIA_COST = 10, 14, 150
NB = [(1, 0, STEP), (-1, 0, STEP), (0, 1, STEP), (0, -1, STEP),
      (1, 1, DIAG), (1, -1, DIAG), (-1, 1, DIAG), (-1, -1, DIAG)]


def as_buffers(masks):
    """numpy scalar indexing dominates the search; bytes do not."""
    return [m.tobytes() for m in masks]


def astar(blk_track, blk_via, sources, targets, target_xy, cost=None,
          heuristic_weight=1.0, max_expansions=600000):
    """Multi-source A* over (cell, layer). Returns a list of (cell, layer).

    Targets are (cell, layer) pairs: a pad that exists only on B.Cu is not
    reached by arriving above it on F.Cu.
    """
    tset = set(targets)
    INF = float('inf')
    best = {}
    heap = []
    tx, ty = target_xy
    tlayers = {l for _c, l in tset}

    def hcost(c, l):
        x, y = c % W, c // W
        dx, dy = abs(x - tx), abs(y - ty)
        h = STEP * (dx + dy) + (DIAG - 2 * STEP) * min(dx, dy)
        # reaching a pad that only exists on one layer costs at least one via
        return heuristic_weight * (h + (0 if l in tlayers else VIA_COST))

    for c, l in sources:
        if blk_track[l][c]:
            continue
        best[(c, l)] = 0
        heapq.heappush(heap, (hcost(c, l), 0, c, l, None))
    # A hopeless net otherwise explores the whole board before giving up.
    budget = max_expansions
    seen = {}
    while heap:
        budget -= 1
        if budget <= 0:
            return None
        _f, g, c, l, parent = heapq.heappop(heap)
        if (c, l) in seen:
            continue
        seen[(c, l)] = parent
        if (c, l) in tset:
            path = []
            k = (c, l)
            while k is not None:
                path.append(k)
                k = seen[k]
            return path[::-1]
        x, y = c % W, c // W
        for dx, dy, cost_step in NB:
            nx, ny = x + dx, y + dy
            if nx < 0 or ny < 0 or nx >= W or ny >= H:
                continue
            nc = ny * W + nx
            if blk_track[l][nc]:
                continue
            if (nc, l) in seen:
                continue
            ng = g + cost_step + (0 if cost is None else cost[l][nc])
            if best.get((nc, l), INF) <= ng:
                continue
            best[(nc, l)] = ng
            heapq.heappush(heap, (ng + hcost(nc, l), ng, nc, l, (c, l)))
        # a through via reaches every routing layer at once
        if all(not blk_via[k][c] for k in range(NL)):
            for ol in range(NL):
                if ol == l or (c, ol) in seen:
                    continue
                ng = g + VIA_COST + (0 if cost is None else cost[ol][c])
                if best.get((c, ol), INF) > ng:
                    best[(c, ol)] = ng
                    heapq.heappush(heap, (ng + hcost(c, ol), ng, c, ol, (c, l)))
    return None


def line_cells(a, b):
    """Cells a straight run covers. The clearance check and the copper model
    must agree exactly, so both go through here."""
    ax, ay = a % W, a // W
    bx, by = b % W, b // W
    # Sampling on the longer axis alone lets a 45 degree run clip the corner
    # of a blocked cell it never visits, so step on the Manhattan distance.
    n = abs(bx - ax) + abs(by - ay)
    if n == 0:
        return [a]
    out = []
    prev = None
    for i in range(n + 1):
        x = ax + int(round((bx - ax) * i / n))
        y = ay + int(round((by - ay) * i / n))
        c = y * W + x
        if c != prev:
            out.append(c)
            prev = c
    return out


def clear_line(blk, a, b):
    return not any(blk[c] for c in line_cells(a, b))


def densify(path):
    """Every cell a polyline covers, using the same rasterisation the
    clearance check used."""
    out = [path[0]]
    for (ac, al), (bc, bl) in zip(path, path[1:]):
        if al != bl:
            out.append((bc, bl))
            continue
        for c in line_cells(ac, bc)[1:]:
            out.append((c, bl))
    return out


def two_seg_45(a, b):
    """A route from a to b as one diagonal plus one orthogonal run.

    Returns the corner points, or None when a and b are not reachable that
    way. Every leg is horizontal, vertical or exactly 45 degrees, which is
    what the board should look like and what a fabricator expects.
    """
    ax, ay = a % W, a // W
    bx, by = b % W, b // W
    dx, dy = bx - ax, by - ay
    if dx == 0 or dy == 0 or abs(dx) == abs(dy):
        return [a, b]
    sx = 1 if dx > 0 else -1
    sy = 1 if dy > 0 else -1
    m = min(abs(dx), abs(dy))
    opts = []
    # diagonal first, then straight
    opts.append((ax + sx * m, ay + sy * m))
    # straight first, then diagonal
    if abs(dx) > abs(dy):
        opts.append((ax + sx * (abs(dx) - m), ay))
    else:
        opts.append((ax, ay + sy * (abs(dy) - m)))
    out = []
    for cx, cy in opts:
        if 0 <= cx < W and 0 <= cy < H:
            out.append([a, cy * W + cx, b])
    return out if out else None


def merge_collinear(path):
    """Collapse a unit-step path into its straight legs.

    The search already moves only orthogonally or diagonally, so every leg is
    0, 45 or 90 degrees. Emitting exactly what was searched keeps the copper
    identical to what the clearance model checked; shortcutting afterwards
    reintroduces geometry the model never saw.
    """
    out = [path[0]]
    for k in range(1, len(path)):
        c, l = path[k]
        pc, pl = path[k - 1]
        if l != pl:
            out.append(path[k])
            continue
        if len(out) >= 2 and out[-1][1] == l and out[-2][1] == l:
            ax, ay = out[-2][0] % W, out[-2][0] // W
            bx, by = out[-1][0] % W, out[-1][0] // W
            cx, cy = c % W, c // W
            d1 = (bx - ax, by - ay)
            d2 = (cx - bx, cy - by)
            if d1[0] * d2[1] == d1[1] * d2[0] and \
               (d1[0] * d2[0] >= 0) and (d1[1] * d2[1] >= 0):
                out[-1] = path[k]
                continue
        out.append(path[k])
    return out


def simplify(blk_track, path):
    """Pull the path straight, keeping every leg at 0, 45 or 90 degrees."""
    out = [path[0]]
    i = 0
    n = len(path)
    while i < n - 1:
        if path[i][1] != path[i + 1][1]:
            out.append(path[i + 1])
            i += 1
            continue
        layer = path[i][1]
        blk = blk_track[layer]
        # A shortcut may not reach past a layer change: jumping over one
        # deletes the via that connects the two sides.
        last = i + 1
        while last + 1 < n and path[last + 1][1] == layer:
            last += 1
        chosen = None
        j = last
        while j > i + 1:
            cand = two_seg_45(path[i][0], path[j][0])
            if cand is None:
                j -= 1
                continue
            options = [cand] if isinstance(cand[0], int) else list(cand)
            for opt in options:
                if all(not any(blk[c] for c in line_cells(u, v))
                       for u, v in zip(opt, opt[1:])):
                    chosen = (j, opt)
                    break
            if chosen:
                break
            j -= 1
        if chosen is None:
            # no legal shortcut: take the longest run that stays in line
            j = i + 1
            ax, ay = path[i][0] % W, path[i][0] // W
            bx, by = path[j][0] % W, path[j][0] // W
            dx, dy = bx - ax, by - ay
            while (j + 1 < n and path[j + 1][1] == layer
                   and (path[j + 1][0] % W - path[j][0] % W,
                        path[j + 1][0] // W - path[j][0] // W) == (dx, dy)):
                j += 1
            out.append(path[j])
            i = j
            continue
        j, opt = chosen
        for c in opt[1:]:
            out.append((c, layer))
        i = j
    # drop repeats a shortcut may have introduced
    dedup = [out[0]]
    for p in out[1:]:
        if p != dedup[-1]:
            dedup.append(p)
    return dedup


def path_to_items(path, net, width):
    """One segment per leg of the polyline, one via per layer change."""
    out_segs, out_vias = [], []
    for (ac, al), (bc, bl) in zip(path, path[1:]):
        if al != bl:
            vx, vy = to_mm(ac % W, ac // W)
            out_vias.append({'net': net, 'at': [round(vx, 4), round(vy, 4)]})
            continue
        ax, ay = to_mm(ac % W, ac // W)
        bx, by = to_mm(bc % W, bc // W)
        if (ax, ay) == (bx, by):
            continue
        out_segs.append({'net': net, 'layer': LAYER_NAME[al], 'width': width,
                         'start': [round(ax, 4), round(ay, 4)],
                         'end': [round(bx, 4), round(by, 4)]})
    return out_segs, out_vias


def net_order(geom, by_net, skip):
    order = []
    for net, entries in by_net.items():
        if net in skip or len(entries) < 2 or net.startswith('unconnected-'):
            continue
        xs = [geom_pad_xy(geom, k) for k, _ in entries]
        span = (max(p[0] for p in xs) - min(p[0] for p in xs)) + \
               (max(p[1] for p in xs) - min(p[1] for p in xs))
        order.append((0 if net in USB else 1, len(entries), span, net))
    order.sort()
    return [o[3] for o in order]


def route_net(net, entries, geom, blk_track, blk_via, cost, via_r=7):
    """Join a net's pads, closest pair first. Returns (paths, failed keys)."""
    width, clear, via_d = rules(net)
    comps = []
    for key, entry in entries:
        cells = set()
        for c in cells_of(entry):
            for l in entry[2]:
                cells.add((c, l))
        comps.append({'keys': [key], 'cells': cells,
                      'xy': to_cell(*geom_pad_xy(geom, key))})
    # Vias this net has already placed have to keep their distance from the
    # next one; the shared map only knows about other nets.
    blk_via = [bytearray(b) for b in blk_via]
    via_offs = disk_offsets(via_r)

    def block_via_at(c):
        x, y = c % W, c // W
        for dx, dy in via_offs:
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < H:
                idx = ny * W + nx
                for layer in range(NL):
                    blk_via[layer][idx] = 1

    paths, dead = [], set()
    while len(comps) > 1:
        pair = None
        for i in range(len(comps)):
            for j in range(i + 1, len(comps)):
                if (i, j) in dead:
                    continue
                d = (abs(comps[i]['xy'][0] - comps[j]['xy'][0]) +
                     abs(comps[i]['xy'][1] - comps[j]['xy'][1]))
                if pair is None or d < pair[0]:
                    pair = (d, i, j)
        if pair is None:
            break
        _d, i, j = pair
        path = astar(blk_track, blk_via, sorted(comps[i]['cells']),
                     comps[j]['cells'], comps[j]['xy'], cost)
        if path is None:
            dead.add((i, j))
            continue
        paths.append(path)
        for (pc, pl), (nc, nl) in zip(path, path[1:]):
            if pl != nl:
                block_via_at(pc)
        comps[i]['cells'] |= comps[j]['cells'] | set(path)
        comps[i]['keys'] += comps[j]['keys']
        comps.pop(j)
        dead = {(a, b) for (a, b) in dead if a != j and b != j}
        dead = {(a - (a > j), b - (b > j)) for (a, b) in dead}
    failed = [(net, key) for c in comps[1:] for key in c['keys']]
    return paths, failed


def route_all(geom, skip, bias, nets, copper_base, hard, no_via, outside,
              pad_cells, by_net, order):
    """One hard-blocked pass: every net is routed clear of everything already
    placed, so the geometry cannot violate clearance by construction."""
    copper = [c.copy() for c in copper_base]
    segs, vias, failed = [], [], []
    for net in order:
        width, clear, via_d = rules(net)
        rt = int(math.ceil((width / 2 + MAX_CLEARANCE) / GRID)) + GRID_SLACK
        # the copper map already carries the other object's own width, so a
        # via only has to add its own radius and the clearance
        rv = int(math.ceil((via_d / 2 + MAX_CLEARANCE) / GRID)) + GRID_SLACK
        nid = nets[net]
        bt_np = [hard[l] | dilate((copper[l] != 0) & (copper[l] != nid), rt)
                 for l in range(NL)]
        bt = as_buffers(bt_np)
        # a via is wider than a track, so it needs its own board-edge margin
        via_edge = dilate(outside,
                          int(math.ceil((0.30 + via_d / 2) / GRID)) + GRID_SLACK)
        bv = as_buffers([hard[l] | no_via[l] | via_edge | dilate(copper[l] != 0, rv)
                         for l in range(NL)])
        t0 = time.time()
        paths, f = route_net(net, by_net[net], geom, bt, bv, None, rv)
        dt = time.time() - t0
        if dt > 2.0:
            print('    %-18s %5.1f s  %d pads %s' % (net, dt, len(by_net[net]),
                                                     'FAILED' if f else ''),
                  flush=True)
        failed += f
        for path in paths:
            spath = simplify(bt, merge_collinear(path))
            s, v = path_to_items(spath, net, width)
            segs += s
            vias += v
            mark_path(copper, nid, densify(spath), width, via_d)
    return segs, vias, failed


def route_board(geom, skip, passes=8):
    nets, copper_base, hard, no_via, outside, pad_cells = build(geom)
    by_net = {}
    for key, entry in pad_cells.items():
        by_net.setdefault(entry[0], []).append((key, entry))
    NARROW.clear()
    for net, entries in by_net.items():
        if any(k[0] in FINE_PITCH for k, _ in entries):
            NARROW.add(net)

    base = net_order(geom, by_net, skip)
    bias, best = set(), None
    for i in range(passes):
        order = [n for n in base if n in bias] + [n for n in base if n not in bias]
        segs, vias, failed = route_all(geom, skip, bias, nets, copper_base,
                                       hard, no_via, outside, pad_cells,
                                       by_net, order)
        print('pass %d: %d segments, %d vias, %d failed'
              % (i + 1, len(segs), len(vias), len(failed)))
        if best is None or len(failed) < len(best[2]):
            best = (segs, vias, failed)
        if not failed:
            break
        before = len(bias)
        bias |= {n for n, _k in failed}
        if len(bias) == before:
            break
    return best


def main():
    geom = json.load(open(sys.argv[1]))
    skip = set(sys.argv[3].split(',')) if len(sys.argv) > 3 else set()
    passes = int(sys.argv[4]) if len(sys.argv) > 4 else 8
    segs, vias, failed = route_board(geom, skip, passes)
    json.dump({'segments': segs, 'vias': vias,
               'failed': [list(f) for f in failed]}, open(sys.argv[2], 'w'))
    print('best: %d segments, %d vias, %d failed connections'
          % (len(segs), len(vias), len(failed)))


_PADXY = {}
def geom_pad_xy(geom, key):
    if not _PADXY:
        for p in geom['pads']:
            _PADXY[(p['ref'], p['pad'])] = (p['x'], p['y'])
    return _PADXY[key]


def mark_path(copper, nid, path, width, via_d):
    rw = max(1, int(math.ceil((width / 2) / GRID)))
    rv = int(math.ceil((via_d / 2) / GRID))
    for c, l in path:
        x, y = c % W, c // W
        for dx, dy in disk_offsets(rw):
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < H and copper[l][ny, nx] == 0:
                copper[l][ny, nx] = nid
    for (c, l1), (c2, l2) in zip(path, path[1:]):
        if l1 != l2:
            x, y = c % W, c // W
            for dx, dy in disk_offsets(rv):
                nx, ny = x + dx, y + dy
                if 0 <= nx < W and 0 <= ny < H:
                    for l in range(NL):
                        if copper[l][ny, nx] == 0:
                            copper[l][ny, nx] = nid


if __name__ == '__main__':
    main()
