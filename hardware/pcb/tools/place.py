"""Place the Rev A board: anchors by hand, passives grid-packed.

Run with KiCad's bundled Python. Reads the schematic netlist, loads every
footprint, sets nets from the netlist, and writes the board.
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import os, sys, math
import pcbnew

REPO = '/Users/giacomomellone/Projects/cicala/cicala'
PCB = REPO + '/hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pcb'
PROJ_FP = REPO + '/hardware/pcb/cicala_rev_a/cicala.pretty'
SYS_FP = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'
NET = os.environ['NETLIST']

import ksexp

MM = pcbnew.FromMM
def V(x, y): return pcbnew.VECTOR2I(MM(x), MM(y))

# ---------------------------------------------------------------- netlist ---
def read_netlist(path):
    n = ksexp.load(path)
    comps = {}
    for c in ksexp.children(ksexp.child(n, 'components'), 'comp'):
        ref = ksexp.child(c, 'ref')[1]
        flds = {}
        f = ksexp.child(c, 'fields')
        if f is not None:
            for fld in ksexp.children(f, 'field'):
                flds[ksexp.child(fld, 'name')[1]] = fld[2] if len(fld) > 2 else ''
        props = {}
        for pr in ksexp.children(c, 'property'):
            nm = ksexp.child(pr, 'name')[1]
            val = ksexp.child(pr, 'value')
            props[nm] = val[1] if val is not None else ''
        sp = ksexp.child(c, 'sheetpath')
        comps[ref] = {
            'fpid': ksexp.child(c, 'footprint')[1],
            'value': ksexp.child(c, 'value')[1],
            'datasheet': (ksexp.child(c, 'datasheet') or [None, ''])[1],
            'description': (ksexp.child(c, 'description') or [None, ''])[1],
            'fields': flds,
            'dnp': 'dnp' in props,
            'sheetname': props.get('Sheetname', ''),
            'sheetfile': props.get('Sheetfile', ''),
            'path': ksexp.child(sp, 'tstamps')[1] + ksexp.child(c, 'tstamps')[1],
        }
    pads = {}
    for net in ksexp.children(ksexp.child(n, 'nets'), 'net'):
        name = ksexp.child(net, 'name')[1]
        for nd in ksexp.children(net, 'node'):
            pads[(ksexp.child(nd, 'ref')[1], ksexp.child(nd, 'pin')[1])] = name
    return comps, pads


# ---------------------------------------------------------------- anchors ---
# ref: (x, y, rotation, bottom side?)
# Rotations were chosen by measuring the flipped footprint: the module's
# pad-free antenna end must point at -X, and both connectors must open toward
# the board's front edge.
ANCHORS = {
    'U1':  (16.25, 23.00, 270, True),   # antenna at the left board edge
    'SW1': (27.00, 12.00, 0,   False),
    'SW2': (53.00, 12.00, 0,   False),
    'J1':  (42.00, 50.10, 180, True),   # opening lands on the front wall
    'J2':  (26.00, 47.00, 180, True),   # flex folds in from the front edge
    'D4':  (52.00, 51.30, 180, True),
    'U4':  (42.00, 43.00, 0,   True),
    'J4':  (61.00, 46.50, 0,   True),
    # power column, clear of both mounting holes so the 0.4 mm-pitch charger
    # can escape on both sides
    'U2':  (72.00, 14.00, 0,   True),
    'U3':  (72.00, 20.00, 0,   True),
    'L2':  (72.00, 25.00, 0,   True),
    'U5':  (72.00, 30.00, 0,   True),
    'J3':  (73.60, 40.00, 90,  True),
    # e-paper boost loop, laid out along its current path rather than packed
    'U6':  (13.00, 41.00, 0,   True),
    'L1':  (14.00, 35.00, 0,   True),
    'Q1':  (19.50, 35.00, 0,   True),
    'D3':  (24.00, 35.50, 0,   True),
    'D1':  (24.00, 39.00, 0,   True),
    'D2':  (29.50, 35.50, 0,   True),
    'C16': (29.50, 39.00, 0,   True),
    # partners of pins that can only leave their package in one direction
    'R7':  (76.50, 13.60, 0,   True),   # BQ25185 ILIM/VSET
    'R8':  (76.50, 16.00, 0,   True),   # BQ25185 ISET
    'R18': (17.50, 41.00, 0,   True),   # TPS22917 QOD
    'R31': (27.25, 42.00, 90,  True),   # panel RESET series
    'R32': (23.75, 42.00, 90,  True),   # panel RESET pull-down
    'R3':  (48.00, 43.95, 0,   True),   # USB D- series
    'R4':  (48.00, 42.05, 0,   True),   # USB D+ series
    # CC pull-downs sit under the pins they terminate, so neither has to cross
    # the connector's own pad row
    'R1':  (45.20, 43.20, 90,  True),   # CC1, under J1 pin A5
    'R2':  (38.60, 43.20, 90,  True),   # CC2, under J1 pin B5
    'C4':  (51.00, 44.40, 90,  True),   # VBUS bulk beside the receptacle
}

# Free rectangles the packer may use, in order of preference per group.
REGIONS = {
    'rear':  (10.4, 4.0, 31.6, 13.6),
    'right': (68.4, 4.0, 80.6, 52.1),
    'mid':   (10.4, 32.4, 31.6, 43.4),
    'below': (32.4, 36.4, 80.6, 43.4),   # under the cell keep-out
    'front': (10.4, 43.6, 80.6, 52.1),
    'top0':  (12.3, 4.0, 20.0, 21.0),
    'top1':  (34.4, 4.0, 41.6, 21.0),
    'top2':  (64.4, 4.0, 80.6, 21.0),
}

# (refs, preferred regions, bottom side?)
GROUPS = [
    (['C28', 'C29', 'C30', 'R25', 'C27', 'R26'], ['rear'], True),
    (['R21', 'C25'], ['top1'], False),
    (['R22', 'C26'], ['top2'], False),
    (['R3', 'R4', 'C1', 'C2', 'R1', 'R2', 'R33', 'C31', 'C4'], ['front', 'below'], True),
    (['R23', 'R24'], ['front', 'below'], True),
    (['C5', 'C6', 'R7', 'R8', 'R9', 'R10', 'R13'], ['right'], True),
    (['C8', 'C9', 'C10', 'C11', 'R14', 'R15', 'R16'], ['right'], True),
    (['R11', 'R12', 'C7', 'R34'], ['right'], True),
    (['R19', 'R20', 'C15'], ['mid', 'below'], True),
    (['C12', 'C13', 'C14', 'R18'], ['mid', 'below'], True),
    (['C17', 'C18', 'C19', 'C20', 'C21', 'C22', 'C23', 'C24'], ['below', 'front', 'mid'], True),
    (['R27', 'R28', 'R29', 'R30', 'R31', 'R32'], ['below', 'mid', 'front'], True),
    (['R5', 'R6', 'C3'], ['rear', 'top0'], True),
    (['TP12', 'TP13', 'TP10'], ['front', 'below'], True),
    (['TP8', 'TP9', 'TP22', 'TP14', 'TP15', 'TP11', 'TP7', 'TP3'], ['right', 'front'], True),
    (['TP16', 'TP17', 'TP18', 'TP19', 'TP20', 'TP21'], ['below', 'mid', 'front'], True),
    (['TP1', 'TP2', 'TP4', 'TP5', 'TP6'], ['below', 'front', 'rear'], True),
]

# The module footprint's courtyard covers Espressif's whole antenna keep-out,
# which would exclude the entire left half of the board from placement. The
# keep-out is enforced by its own zone; here only the body matters.
BOX_OVERRIDE = {'U1': (25.6, 18.1)}

# Circles on Edge.Cuts, so the board-edge clearance rule applies around them.
MOUNT_HOLES = [(13, 7), (77.5, 7), (13, 49), (77.5, 49)]
# Access holes in the base and steel, with no footprint of their own: keep a
# little room so a probe can actually reach the underside through them.
SERVICE_HOLES = [(6.5, 40), (77.5, 40)]
SERVICE_KEEPOUT_R = 2.0

# Rectangles no placed part's courtyard may enter.
# (x0, y0, x1, y1, 'both'|'bottom')
FORBIDDEN = [
    (3.0, 3.5, 10.2, 52.5, 'both'),     # module antenna keep-out
    (32.0, 6.0, 68.0, 36.0, 'bottom'),  # cell volume
    # Space under the receptacle: the two D+ pins are separated by a D- pin,
    # so tying them needs a via pair and a run on an inner layer.
    (37.2, 42.3, 47.6, 46.6, 'bottom'),
]

PITCH_X, PITCH_Y = 2.20, 3.40
EDGE_MARGIN = 0.45
COURTYARD_GAP = 0.25


_FP_CACHE = {}

def load_fp(fpid):
    """One library read per distinct footprint; the rest are copies."""
    if fpid not in _FP_CACHE:
        lib, name = fpid.split(':', 1)
        d = PROJ_FP if lib == 'cicala' else os.path.join(SYS_FP, lib + '.pretty')
        fp = pcbnew.FootprintLoad(d, name)
        if fp is None:
            sys.exit('footprint not found: ' + fpid)
        _FP_CACHE[fpid] = fp
    return pcbnew.FOOTPRINT(_FP_CACHE[fpid])


def courtyard_poly(fp):
    """The polygon DRC itself tests, not its bounding box."""
    layer = pcbnew.B_CrtYd if fp.IsFlipped() else pcbnew.F_CrtYd
    poly = fp.GetCourtyard(layer)
    if poly.OutlineCount() == 0:
        other = fp.GetCourtyard(pcbnew.F_CrtYd if layer == pcbnew.B_CrtYd
                                else pcbnew.B_CrtYd)
        if other.OutlineCount():
            return other
        return None
    return poly


def courtyard_box(fp):
    layer = pcbnew.B_CrtYd if fp.IsFlipped() else pcbnew.F_CrtYd
    poly = fp.GetCourtyard(layer)
    if poly.OutlineCount() == 0:
        poly = fp.GetCourtyard(pcbnew.F_CrtYd if layer == pcbnew.B_CrtYd
                               else pcbnew.B_CrtYd)
    if poly.OutlineCount() == 0:
        return fp.GetBoundingBox()
    return poly.BBox()


def boxes_overlap(a, b, gap_iu):
    return not (a.GetRight() + gap_iu < b.GetLeft()
                or b.GetRight() + gap_iu < a.GetLeft()
                or a.GetBottom() + gap_iu < b.GetTop()
                or b.GetBottom() + gap_iu < a.GetTop())


def main():
    comps, padnets = read_netlist(NET)
    # Always start from the geometry-only snapshot so a re-run is idempotent.
    board = pcbnew.LoadBoard(os.environ['GEOM'])

    nets = {}
    for name in sorted({n for n in padnets.values()}):
        ni = board.FindNet(name)
        if ni is None:
            ni = pcbnew.NETINFO_ITEM(board, name)
            board.Add(ni)
        nets[name] = ni

    placed = {}
    occupied = []        # (ref, courtyard polygon, flipped)

    outline = pcbnew.SHAPE_POLY_SET()
    board.GetBoardPolygonOutlines(outline, False)
    inner = pcbnew.SHAPE_POLY_SET(outline)
    inner.Deflate(MM(EDGE_MARGIN), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS,
                  MM(0.005))

    def inside_board(poly):
        rest = pcbnew.SHAPE_POLY_SET(poly)
        rest.BooleanSubtract(inner)
        return rest.OutlineCount() == 0

    def install(ref, x, y, rot, bottom):
        spec = comps[ref]
        fp = load_fp(spec['fpid'])
        board.Add(fp)
        fp.SetReference(ref)
        # Everything DRC's schematic-parity check compares.
        lib, name = spec['fpid'].split(':', 1)
        fp.SetFPID(pcbnew.LIB_ID(lib, name))
        fp.SetValue(spec['value'])
        fp.SetLibDescription(spec['description'])
        fp.SetSheetname(spec['sheetname'])
        fp.SetSheetfile(spec['sheetfile'])
        fp.SetPath(pcbnew.KIID_PATH(spec['path']))
        fp.SetDNP(spec['dnp'])
        for fname, fvalue in spec['fields'].items():
            if fname in ('Reference', 'Value', 'Footprint'):
                continue
            if fp.HasField(fname):
                fp.GetField(fname).SetText(fvalue)
            else:
                pf = pcbnew.PCB_FIELD(fp, fp.GetNextFieldOrdinal(), fname)
                pf.SetText(fvalue)
                pf.SetVisible(False)
                fp.Add(pf)
        fp.SetPosition(V(x, y))
        if bottom:
            fp.Flip(V(x, y), False)
        fp.SetOrientationDegrees(rot)
        for pad in fp.Pads():
            net = padnets.get((ref, pad.GetNumber()))
            if net is not None:
                pad.SetNet(nets[net])
        placed[ref] = fp
        poly = courtyard_poly(fp)
        occupied.append((ref, poly, fp.IsFlipped()))
        return fp, poly

    for ref, (x, y, rot, bottom) in ANCHORS.items():
        install(ref, x, y, rot, bottom)

    # M2.5 clearance holes as real NPTH footprints rather than circles on
    # Edge.Cuts, so they land in the drill file as drilled holes instead of
    # routed slots, and DRC can reason about them.
    for i, (hx, hy) in enumerate(MOUNT_HOLES, start=1):
        fp = load_fp('MountingHole:MountingHole_2.7mm')
        board.Add(fp)
        fp.SetReference('H%d' % i)
        fp.SetValue('M2.5')
        fp.SetPosition(V(hx, hy))
        fp.SetAttributes(pcbnew.FP_THROUGH_HOLE | pcbnew.FP_BOARD_ONLY
                         | pcbnew.FP_EXCLUDE_FROM_BOM
                         | pcbnew.FP_EXCLUDE_FROM_POS_FILES)
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)
        occupied.append(('H%d' % i, courtyard_poly(fp), None))

    gap = MM(COURTYARD_GAP)
    unplaced = []

    # Nets that a part shares with something already on the board. The rails
    # are poured, so they say nothing useful about where a part should sit.
    BULK = {'GND', '3V3'}
    ref_nets = {}
    for (ref, _pad), net in padnets.items():
        if net not in BULK and not net.startswith('unconnected-'):
            ref_nets.setdefault(ref, set()).add(net)

    def pad_points(net):
        pts = []
        for ref, fp in placed.items():
            for pad in fp.Pads():
                if padnets.get((ref, pad.GetNumber())) == net:
                    c = pad.GetPosition()
                    pts.append((pcbnew.ToMM(c.x), pcbnew.ToMM(c.y)))
        return pts

    def target_for(ref, region):
        pts = []
        for net in ref_nets.get(ref, ()):
            pts.extend(pad_points(net))
        if not pts:
            x0, y0, x1, y1 = REGIONS[region]
            return ((x0 + x1) / 2, (y0 + y1) / 2)
        return (sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts))

    def try_slot(ref, cx, cy, rot, bottom):
        if any((cx - hx) ** 2 + (cy - hy) ** 2 < SERVICE_KEEPOUT_R ** 2
               for hx, hy in SERVICE_HOLES):
            return False
        fp, poly = install(ref, cx, cy, rot, bottom)
        bad = poly is None or not inside_board(poly)
        if not bad:
            for oref, opoly, oflip in occupied[:-1]:
                if opoly is None or (oflip is not None and oflip != bool(bottom)):
                    continue
                if poly.Collide(opoly, MM(COURTYARD_GAP)):
                    bad = True
                    break
        if not bad:
            for fx0, fy0, fx1, fy1, side in FORBIDDEN:
                if side == 'bottom' and not bottom:
                    continue
                r = pcbnew.SHAPE_POLY_SET()
                r.NewOutline()
                for px, py in ((fx0, fy0), (fx1, fy0), (fx1, fy1), (fx0, fy1)):
                    r.Append(MM(px), MM(py))
                if poly.Collide(r, 0):
                    bad = True
                    break
        if bad:
            board.Remove(fp)
            placed.pop(ref)
            occupied.pop()
            return False
        return True

    for refs, region_names, bottom in GROUPS:
        for ref in refs:
            if ref in placed:
                continue
            tx, ty = target_for(ref, region_names[0])
            slots = []
            fallback = [r for r in REGIONS
                        if r.startswith('top') != bottom and r not in region_names]
            for region in list(region_names) + fallback:
                x0, y0, x1, y1 = REGIONS[region]
                nx = int((x1 - x0) / PITCH_X)
                ny = int((y1 - y0) / PITCH_Y)
                for iy in range(ny + 1):
                    for ix in range(nx + 1):
                        cx = x0 + PITCH_X / 2 + ix * PITCH_X
                        cy = y0 + PITCH_Y / 2 + iy * PITCH_Y
                        if cx > x1 or cy > y1:
                            continue
                        rank = 0 if region in region_names else 10000
                        slots.append((rank + (cx - tx) ** 2 + (cy - ty) ** 2, cx, cy))
            slots.sort()
            done = False
            for rot in (90, 0):
                for _d, cx, cy in slots:
                    if try_slot(ref, cx, cy, rot, bottom):
                        done = True
                        break
                if done:
                    break
            if not done:
                # nothing on the coarse grid: sweep finely for any real gap
                fine = []
                for region in list(region_names) + fallback:
                    x0, y0, x1, y1 = REGIONS[region]
                    ix = x0
                    while ix <= x1:
                        iy = y0
                        while iy <= y1:
                            fine.append(((ix - tx) ** 2 + (iy - ty) ** 2, ix, iy))
                            iy += 0.5
                        ix += 0.5
                fine.sort()
                for rot in (90, 0):
                    for _d, cx, cy in fine:
                        if try_slot(ref, cx, cy, rot, bottom):
                            done = True
                            break
                    if done:
                        break
            if not done:
                unplaced.append((ref, region_names))

    # A 78 x 49 mm board with 105 parts cannot carry every designator on silk.
    # Passive references move to the fabrication layers, which is what the
    # assembly drawing prints; silk keeps the parts a human needs to find.
    KEEP_ON_SILK = {'U1', 'J2', 'J3', 'J4', 'SW1', 'SW2',
                    'H1', 'H2', 'H3', 'H4'}
    hidden = 0
    for ref, fp in placed.items():
        if ref not in KEEP_ON_SILK:
            f = fp.Reference()
            if f.GetLayer() in (pcbnew.F_SilkS, pcbnew.B_SilkS):
                f.SetLayer(pcbnew.B_Fab if fp.IsFlipped() else pcbnew.F_Fab)
                hidden += 1
        fp.Value().SetVisible(False)
    print('references moved from silk to fab:', hidden)

    missing = [r for r in comps if r not in placed]
    print('placed %d of %d footprints' % (len(placed), len(comps)))
    if unplaced:
        print('NO SLOT:', unplaced)
    if missing:
        print('NOT PLACED AT ALL:', sorted(missing))

    items = [(r, f) for r, f in sorted(placed.items())]
    clashes = []
    for i in range(len(items)):
        pa = courtyard_poly(items[i][1])
        if pa is None:
            continue
        for j in range(i + 1, len(items)):
            if items[i][1].IsFlipped() != items[j][1].IsFlipped():
                continue
            pb = courtyard_poly(items[j][1])
            if pb is not None and pa.Collide(pb, 0):
                clashes.append((items[i][0], items[j][0]))
    print('courtyard clashes:', clashes if clashes else 'none')

    board.BuildListOfNets()
    pcbnew.SaveBoard(PCB, board)
    from unique_ids import normalize_file
    normalize_file(PCB)
    print('saved', PCB)


if __name__ == '__main__':
    main()
