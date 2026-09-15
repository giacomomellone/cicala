"""Orient individual meshes for slicing without changing assembly coordinates."""
import math


REFERENCES = {'cap_pad','next_pad','pcb_reference','cell_reference','panel_reference'}
ACCESSORIES = {'carrier','carrier_cover','wedge'}


def role(part):
    if part in REFERENCES:
        return 'reference'
    if part in ACCESSORIES:
        return 'accessories'
    if part == 'flex_former':
        return 'jigs'
    if part.startswith('coupon') or part == 'keeper_coupon':
        return 'coupons'
    return 'core'


def rotation(part):
    if part in {'category_cap','next_cap','coupon_cap'}:
        return [0,180,0]
    if part in {'top_shell','coupon'}:
        return [0,180,0]
    if part == 'flex_former':
        return [0,-90,0]
    if part == 'wedge':
        return [0,90,0]
    return [0,0,0]


def transform(point, angles):
    x,y,z = point
    a,b,c = map(math.radians,angles)
    y,z = y*math.cos(a)-z*math.sin(a),y*math.sin(a)+z*math.cos(a)
    x,z = x*math.cos(b)+z*math.sin(b),-x*math.sin(b)+z*math.cos(b)
    return (x*math.cos(c)-y*math.sin(c),x*math.sin(c)+y*math.cos(c),z)


def placement(faces, angles):
    points = [transform(p,angles) for face in faces for p in face]
    lower = [min(p[i] for p in points) for i in range(3)]
    upper = [max(p[i] for p in points) for i in range(3)]
    return {'rotation_degrees':angles,'translation_mm':[-v for v in lower],
            'size_mm':[b-a for a,b in zip(lower,upper)]}


def check_flat_face(faces, height, minimum_area):
    area = 0.0
    for a,b,c in faces:
        if all(abs(p[2]-height)<1e-4 for p in (a,b,c)):
            area += abs((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))/2
    if area < minimum_area:
        raise ValueError('Button lacks the required flat finger-contact area')
    return area
