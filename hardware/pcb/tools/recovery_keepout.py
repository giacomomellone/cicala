"""Contact-side TC2030 shaded keepout from its revision B footprint drawing."""
from shapely import union_all
from shapely.geometry import Point, MultiPoint


def polygon(pads):
    contacts = [p for p in pads if p['ref']=='J4' and p['pad'] in '123456' and p['pad']]
    if len(contacts) != 6:
        return None
    # The shaded rectangle stops at pad centres; each contact disk is excluded.
    # A 1 um margin keeps serialized polygon vertices outside the contact disk.
    return MultiPoint([(p['x'], p['y']) for p in contacts]).convex_hull.difference(union_all([
        Point(p['x'],p['y']).buffer(p['w']/2+0.001,quad_segs=32) for p in contacts]))


def from_footprint(footprint):
    """Transform serialized pad coordinates, including the footprint rotation."""
    import math
    import ksexp
    at = ksexp.child(footprint, 'at')
    x, y = map(float, at[1:3])
    angle = math.radians(float(at[3]) if len(at) > 3 else 0)
    c, s = math.cos(angle), math.sin(angle)
    pads = []
    for pad in ksexp.children(footprint, 'pad'):
        if pad[1] not in {'1', '2', '3', '4', '5', '6'}:
            continue
        px, py = map(float, ksexp.child(pad, 'at')[1:3])
        pads.append({'ref': 'J4', 'pad': pad[1], 'x': x + c * px + s * py,
                     'y': y - s * px + c * py, 'w': float(ksexp.child(pad, 'size')[1])})
    result = polygon(pads)
    if result is None:
        raise ValueError('Recovery footprint must contain six numbered contacts')
    return result
