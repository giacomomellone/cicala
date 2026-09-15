"""Verify the native Rev B placement against the mechanical and factory contract."""
import hashlib
import json
from pathlib import Path
import re
import sys

from shapely.geometry import Point, Polygon, box
from shapely.ops import unary_union

from generate_contract import CONTRACT, OUTPUT, ROOT, outline_points, render

sys.path.insert(0, str(ROOT / 'hardware/tools/pcb'))
sys.path.insert(0, str(ROOT / 'hardware/tools/case'))
import ksexp
from audit_placement import audit
from copper_geometry import pad_shape
from export_component_bounds import fingerprint

BOARD = ROOT / 'hardware/rev_b/pcb/cicala_rev_b.kicad_pcb'
BOUNDS = ROOT / 'hardware/rev_b/case/pcb_component_bounds.scad'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_outline(geometry, contract):
    require(geometry['outline_valid'] and geometry['outline_count'] == 1,
            'Board needs exactly one closed outline, with no separate islands')
    actual = Polygon(geometry['outline'], geometry['outline_holes'])
    expected = Polygon(outline_points(contract))
    require(actual.is_valid, 'Self-intersecting board outline')
    # KiCad and OpenSCAD tessellate the R1 arc differently.
    require(actual.boundary.hausdorff_distance(expected.boundary) < .02
            and actual.symmetric_difference(expected).area < .03,
            'Board outline differs from the L-shaped contract')
    return actual


def check(geometry, models_root):
    c = json.loads(CONTRACT.read_text())
    require(geometry['source_sha256'] == hashlib.sha256(BOARD.read_bytes()).hexdigest(),
            'Board changed during export; regenerate geometry and envelopes together')
    require(OUTPUT.read_text() == render(c), 'Regenerate contract.scad')
    board = ksexp.load(BOARD)
    project = json.loads(BOARD.with_suffix('.kicad_pro').read_text())
    rules = project['board']['design_settings']['rules']
    limits = {'min_copper_edge_clearance':c['pcb']['copper_edge_clearance'],
              'min_clearance':c['manufacturing']['clearance'],
              'min_track_width':c['manufacturing']['escape_min_width'],
              'min_via_diameter':c['manufacturing']['via_diameter'],
              'min_through_hole_diameter':c['manufacturing']['via_drill'],
              'min_silk_clearance':.15, 'min_text_height':.8, 'min_text_thickness':.15}
    for name, limit in limits.items():
        require(rules[name] >= limit, f'KiCad manufacturing rule relaxed: {name}')
    from silkscreen import check_signals
    check_signals(board)
    shape = check_outline(geometry, c)
    stack = ksexp.child(ksexp.child(board, 'setup'), 'stackup')
    thickness = sum(float(ksexp.child(layer, 'thickness')[1])
                    for layer in ksexp.children(stack, 'layer')
                    if ksexp.child(layer, 'thickness') and
                    ksexp.child(layer, 'type')[1] in ('copper', 'core', 'prepreg'))
    require(abs(thickness - c['pcb']['thickness']) < .0001, 'STEP substrate thickness drift')
    require(sum(ksexp.child(layer, 'type')[1] == 'copper'
                for layer in ksexp.children(stack, 'layer')) == 4, 'Expected four copper layers')
    fps = {f['ref']: f for f in geometry['footprints']}
    require(all(f['layer'] == 'F.Cu' or f['ref'].startswith('TP') for f in fps.values()),
            'Fitted components must stay on one face; bare test contacts may use B.Cu')
    checks = audit(geometry, BOARD, models_root, front_parts=set(fps))
    require(not any(value for key, value in checks.items() if key != 'footprints'),
            'Placement/model audit: ' + json.dumps(checks))
    for ref, fp in fps.items():
        court = unary_union([Polygon(p) for p in fp['courtyards']])
        # U1's courtyard includes the manufacturer's air clearance beyond the
        # board; J1's shell intentionally overhangs the USB opening.
        # Fastener-head clearance can extend beyond the PCB into the wider
        # case. The actual mounting drill still needs an intact FR-4 annulus.
        if ref not in {'U1', 'J1'} and not ref.startswith('H'):
            require(shape.buffer(.01).covers(court), f'{ref} courtyard crosses the actual board boundary')
    for pad in geometry['pads']:
        if pad['layers'] and not pad['npth']:
            require(shape.buffer(-.499).covers(pad_shape(pad)),
                    f'{pad["ref"]}:{pad["pad"]} lacks 0.5 mm copper-to-edge clearance')
        if pad['drill']:
            require(pad['drill'] >= .3, 'Sub-0.3 mm drills need an explicit manufacturing review')
            if pad['npth']:
                require(shape.buffer(-.5).covers(pad_shape(pad, drill=True)),
                        f'{pad["ref"]} drill breaks the supporting board edge')
    for ref, position in [('SW1', c['buttons']['centres'][0]),
                          ('SW2', c['buttons']['centres'][1]),
                          *[(f'H{i+1}', p) for i, p in enumerate(c['mounts'])]]:
        require(abs(fps[ref]['x']-position[0])+abs(fps[ref]['y']-position[1]) < .001,
                f'{ref} differs between KiCad and OpenSCAD')
    contents = BOUNDS.read_text()
    recorded = re.search(r'placement_sha256: ([0-9a-f]{64})', contents)
    require(recorded and recorded[1] == fingerprint(BOARD, c['pcb']['z']), 'Stale component envelopes')
    rows = json.loads(contents.split('pcb_component_bounds = ', 1)[1].rstrip(';\n '))
    rf = c['rf_keepout']; rf_shape = box(*rf['origin'],
        rf['origin'][0]+rf['size'][0], rf['origin'][1]+rf['size'][1])
    for ref, p, size in rows:
        body = box(p[0], p[1], p[0]+size[0], p[1]+size[1])
        if ref != 'U1':
            require(body.intersection(rf_shape).area < .0001, f'{ref} in antenna clearance')
        require(p[2] >= c['pcb']['z']+c['pcb']['thickness']-.02,
                f'{ref} component datum does not match top of PCB')
    for name, p, size in [('battery', c['battery']['reserved_origin'], c['battery']['reserved_size']),
                          ('display', c['display']['glass_origin'], c['display']['glass'])]:
        require(box(p[0],p[1],p[0]+size[0],p[1]+size[1]).intersection(rf_shape).area == 0,
                f'{name} crosses antenna clearance')
    carrier = c['carrier']
    ring = Point(carrier['ring_centre']).buffer(carrier['ring_outer_diameter']/2)
    require(ring.intersection(rf_shape).area == 0, 'Carrier ring crosses antenna clearance')
    active = c['display']['active']; origin = c['display']['active_origin']
    require(abs(origin[0]+active[0]/2-(c['case']['origin'][0]+c['case']['width']/2)) < .001
            and abs(origin[1]+active[1]/2-c['case']['depth']/2) < .001,
            'Active display must be centered on the case')
    buttons = c['buttons']
    require(buttons['switch_travel'] + buttons['tip_relief'] <= buttons['stop_travel']
            <= buttons['switch_travel'] + buttons['tip_relief'] + buttons['compliant_pad_thickness'],
            'Nominal travel must reach actuation with overtravel within the compliant pad allowance')
    return {'footprints': len(fps), 'component_envelopes': len(rows),
            'outline_area_mm2': round(shape.area, 2), 'assembly_faces': 1,
            'physical_gates': c['gates']}


if __name__ == '__main__':
    print(json.dumps(check(json.loads(Path(sys.argv[1]).read_text()), Path(sys.argv[2])), indent=2))
