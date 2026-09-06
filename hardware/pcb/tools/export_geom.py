"""Dump the board geometry the router needs, as JSON.

Runs under KiCad's Python; the router itself runs under the system Python,
which has numpy.
"""
import json, sys
import pcbnew

board = pcbnew.LoadBoard(sys.argv[1])
MM = pcbnew.ToMM

SHAPES = {
    pcbnew.PAD_SHAPE_CIRCLE: 'circle',
    pcbnew.PAD_SHAPE_RECTANGLE: 'rect',
    pcbnew.PAD_SHAPE_OVAL: 'oval',
    pcbnew.PAD_SHAPE_ROUNDRECT: 'roundrect',
    pcbnew.PAD_SHAPE_TRAPEZOID: 'rect',
    pcbnew.PAD_SHAPE_CHAMFERED_RECT: 'roundrect',
    pcbnew.PAD_SHAPE_CUSTOM: 'rect',
}

pads = []
footprints = []
for fp in board.GetFootprints():
    courtyard = fp.GetCourtyard(pcbnew.B_CrtYd if fp.IsFlipped() else pcbnew.F_CrtYd)
    courts = []
    for i in range(courtyard.OutlineCount()):
        c = courtyard.Outline(i)
        courts.append([[MM(c.CPoint(j).x),MM(c.CPoint(j).y)] for j in range(c.PointCount())])
    footprints.append({'ref': fp.GetReference(), 'value': fp.GetValue(),
                       'x': MM(fp.GetPosition().x), 'y': MM(fp.GetPosition().y),
                       'angle': fp.GetOrientationDegrees(),
                       'layer': pcbnew.LayerName(fp.GetLayer()), 'courtyards':courts})
    for p in fp.Pads():
        ls = p.GetLayerSet().Seq()
        layers = [pcbnew.LayerName(l) for l in ls if l in
                  (pcbnew.F_Cu, pcbnew.In1_Cu, pcbnew.In2_Cu, pcbnew.B_Cu)]
        pos = p.GetPosition()
        sz = p.GetSize()
        r = 0.0
        if p.GetShape() in (pcbnew.PAD_SHAPE_ROUNDRECT,
                            pcbnew.PAD_SHAPE_CHAMFERED_RECT):
            r = MM(p.GetRoundRectCornerRadius())
        drill = max(MM(p.GetDrillSize().x), MM(p.GetDrillSize().y))
        pads.append({
            'uuid': p.m_Uuid.AsString(),
            'ref': fp.GetReference(), 'pad': p.GetNumber(),
            'fp_x': MM(fp.GetPosition().x), 'fp_y': MM(fp.GetPosition().y),
            'net': p.GetNetname(),
            'layers': layers,
            'x': MM(pos.x), 'y': MM(pos.y),
            'w': MM(sz.x), 'h': MM(sz.y),
            'r': r,
            'shape': SHAPES.get(p.GetShape(), 'rect'),
            'angle': p.GetOrientationDegrees(),
            'drill': drill,
            'drill_w': MM(p.GetDrillSize().x),
            'drill_h': MM(p.GetDrillSize().y),
            'npth': p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH,
        })

outline = pcbnew.SHAPE_POLY_SET()
board.GetBoardPolygonOutlines(outline, False)
o = outline.Outline(0)
poly = [[MM(o.CPoint(i).x), MM(o.CPoint(i).y)] for i in range(o.PointCount())]
holes = []
for i in range(outline.HoleCount(0)):
    h = outline.Hole(0, i)
    holes.append([[MM(h.CPoint(j).x), MM(h.CPoint(j).y)] for j in range(h.PointCount())])

keepouts = []
for z in board.Zones():
    if not z.GetIsRuleArea():
        continue
    ol = z.Outline().Outline(0)
    keepouts.append({
        'layers': [pcbnew.LayerName(l) for l in z.GetLayerSet().Seq()],
        'tracks': not z.GetDoNotAllowTracks(),
        'vias': not z.GetDoNotAllowVias(),
        'pts': [[MM(ol.CPoint(i).x), MM(ol.CPoint(i).y)] for i in range(ol.PointCount())],
    })
for fp in board.GetFootprints():
    for z in fp.Zones():
        if not z.GetIsRuleArea():
            continue
        ol = z.Outline().Outline(0)
        keepouts.append({
            'layers': [pcbnew.LayerName(l) for l in z.GetLayerSet().Seq()],
            'tracks': not z.GetDoNotAllowTracks(),
            'vias': not z.GetDoNotAllowVias(),
            'pts': [[MM(ol.CPoint(i).x), MM(ol.CPoint(i).y)] for i in range(ol.PointCount())],
        })

tracks = []
vias = []
filled_zones = []
for z in board.Zones():
    if z.GetIsRuleArea():
        continue
    for layer in z.GetLayerSet().Seq():
        filled = z.GetFilledPolysList(layer)
        for i in range(filled.OutlineCount()):
            boundary = filled.Outline(i)
            exterior = [[MM(boundary.CPoint(j).x), MM(boundary.CPoint(j).y)]
                        for j in range(boundary.PointCount())]
            interiors = []
            for j in range(filled.HoleCount(i)):
                hole = filled.Hole(i, j)
                interiors.append([[MM(hole.CPoint(k).x), MM(hole.CPoint(k).y)]
                                  for k in range(hole.PointCount())])
            filled_zones.append({'net': z.GetNetname(), 'layer': pcbnew.LayerName(layer),
                                 'name': z.GetZoneName(), 'outline': exterior,
                                 'holes': interiors})
for t in board.GetTracks():
    if isinstance(t, pcbnew.PCB_VIA):
        vias.append({'net': t.GetNetname(), 'at': [MM(t.GetPosition().x), MM(t.GetPosition().y)],
                     'size': MM(t.GetWidth(pcbnew.F_Cu)), 'drill': MM(t.GetDrillValue()),
                     'uuid': t.m_Uuid.AsString()})
    elif isinstance(t, pcbnew.PCB_TRACK):
        tracks.append({'net': t.GetNetname(), 'layer': pcbnew.LayerName(t.GetLayer()),
                       'width': MM(t.GetWidth()),
                       'start': [MM(t.GetStart().x), MM(t.GetStart().y)],
                       'end': [MM(t.GetEnd().x), MM(t.GetEnd().y)],
                       'uuid': t.m_Uuid.AsString()})

json.dump({'pads': pads, 'footprints': footprints, 'segments': tracks, 'vias': vias,
           'filled_zones': filled_zones,
           'outline': poly, 'outline_holes': holes,
           'keepouts': keepouts}, open(sys.argv[2], 'w'))
print('exported %d pads, %d outline points, %d holes, %d rule areas'
      % (len(pads), len(poly), len(holes), len(keepouts)))
