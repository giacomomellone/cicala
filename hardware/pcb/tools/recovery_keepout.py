"""Contact-side TC2030 shaded keepout from its revision B footprint drawing."""
from shapely import union_all
from shapely.geometry import Point, box


def polygon(pads):
    contacts = [p for p in pads if p['ref']=='J4' and p['pad'] in '123456' and p['pad']]
    if len(contacts) != 6:
        return None
    xs, ys = [p['x'] for p in contacts], [p['y'] for p in contacts]
    # The shaded rectangle stops at pad centres; each contact disk is excluded.
    # A 1 um margin keeps serialized polygon vertices outside the contact disk.
    return box(min(xs),min(ys),max(xs),max(ys)).difference(union_all([
        Point(p['x'],p['y']).buffer(p['w']/2+0.001,quad_segs=32) for p in contacts]))
