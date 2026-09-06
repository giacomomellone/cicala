"""Run with: python -m unittest discover -s hardware/pcb/tools -p 'test_*.py'."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import numpy as np
from shapely.geometry import Point

import ksexp
import route as maze
from copper_geometry import Copper, LAYERS, pad_shape, segment_shape, via_shape
from finish_routes import physical_groups
from unique_ids import normalize, uuid_nodes
from usb_pair import make_pair
from chamfer_routes import chamfer
from apply_silkscreen import apply as apply_silk, keyhole
from cleanup_dangling import clean
from recovery_keepout import polygon as recovery_keepout
from thermal_relief import apply as apply_thermal
from audit_reference import audit as audit_reference


def board(pads=()):
    return {'outline':[(0,0),(10,0),(10,10),(0,10)],'outline_holes':[],
            'keepouts':[], 'pads':list(pads), 'footprints':[]}


def pad(**kwargs):
    return dict({'ref':'J1','pad':'SH','uuid':'a','net':'A','layers':['B.Cu'],
                 'x':5,'y':5,'w':1,'h':1,'shape':'rect','r':0,'angle':0,
                 'drill':0,'npth':False},**kwargs)


class GeometryTests(unittest.TestCase):
    def test_reference_audit_reports_missing_plane_without_zero_coverage(self):
        geom = board()
        geom['filled_zones'] = []
        tracks = {'vias':[{'net':'GND','at':[5,5]}],
                  'usb_reference_paths':[[[2,5],[8,5]]]}
        result = audit_reference(geom,tracks)
        self.assertEqual(result['in1_ground_polygons'],0)
        self.assertEqual(result['usb_centreline_without_in1_reference_mm'],[6])
        self.assertIsNone(result['max_ground_stitch_distance_mm_on_0_5_mm_samples'])

    def test_reference_audit_reports_missing_stitches_without_zero_distance(self):
        geom = board()
        geom['filled_zones'] = [{'layer':'In1.Cu','net':'GND',
                                'outline':geom['outline'],'holes':[]}]
        tracks = {'vias':[], 'usb_reference_paths':[[[2,5],[8,5]]]}
        result = audit_reference(geom,tracks)
        self.assertEqual(result['in1_ground_polygons'],1)
        self.assertEqual(result['usb_centreline_without_in1_reference_mm'],[0])
        self.assertIsNone(result['max_ground_stitch_distance_mm_on_0_5_mm_samples'])

    def test_recovery_contact_clearance_is_larger_than_signal_clearance(self):
        p = pad(ref='J4',pad='1',net='A',shape='circle',w=0.7874,h=0.7874)
        copper = Copper(board([p]))
        s = {'start':[5.85,4],'end':[5.85,6],'width':0.2}
        self.assertFalse(copper.clear(segment_shape(s),'B',('B.Cu',)))
        self.assertTrue(copper.clear(segment_shape(s),'B',('F.Cu',)))
        self.assertTrue(copper.clear(segment_shape(s),'A',('B.Cu',)))

    def test_recovery_shaded_area_excludes_contact_disks(self):
        pads = [pad(ref='J4',pad=str(i),x=x,y=y,w=0.7874,h=0.7874,shape='circle')
                for i,(x,y) in enumerate([(3,3),(3,4.27),(4.27,3),(4.27,4.27),
                                          (5.54,3),(5.54,4.27)],1)]
        shape = recovery_keepout(pads)
        self.assertTrue(shape.covers(Point(3.635,3.635)))
        for p in pads:
            self.assertFalse(shape.intersects(pad_shape(p)))

    def test_oval_is_capsule_not_ellipse(self):
        shape = pad_shape(pad(x=0,y=0,w=3,h=1,shape='oval'))
        self.assertTrue(shape.covers(Point(0.9,0.4)))
        self.assertFalse(shape.covers(Point(1.4,0.4)))

    def test_repeated_pad_numbers_remain_separate(self):
        geom = board([pad(x=2,uuid='a'),pad(x=8,uuid='b')])
        self.assertEqual(len(physical_groups('A',geom,{'segments':[],'vias':[]})),2)

    def test_mechanical_copper_blocks_signal(self):
        copper = Copper(board([pad(net='')]))
        s = {'start':[4,5],'end':[6,5],'width':0.2}
        self.assertFalse(copper.clear(segment_shape(s),'A',('B.Cu',)))

    def test_surface_pad_does_not_connect_other_layer(self):
        geom = board([pad()])
        tracks = {'segments':[{'net':'A','layer':'F.Cu','start':[4,5],
                              'end':[6,5],'width':0.2}],'vias':[]}
        self.assertEqual(len(physical_groups('A',geom,tracks)),2)

    def test_via_in_own_pad_is_rejected(self):
        copper = Copper(board([pad()]))
        via = {'at':[5,5],'size':0.45,'drill':0.2,'net':'A'}
        self.assertFalse(copper.clear(via_shape(via),'A',LAYERS,via=True))

    def test_added_drill_blocks_same_net_second_via(self):
        copper = Copper(board())
        copper.add_via({'at':[5,5],'size':0.45,'drill':0.2,'net':'A'})
        via = {'at':[5.4,5],'size':0.45,'drill':0.2,'net':'A'}
        self.assertFalse(copper.clear(via_shape(via),'A',LAYERS,via=True))

    def test_all_four_grid_edges_are_blocked(self):
        copper = Copper(board())
        tracks,vias = copper.masks('A',0.2,0.6,0.1,(0,0),(101,101))
        for mask in tracks+vias:
            m = np.frombuffer(mask,dtype=bool).reshape((101,101))
            for x,y in ((0,50),(100,50),(50,0),(50,100)):
                self.assertTrue(m[y,x])
            self.assertFalse(m[50,50])

    def test_shortcut_preserves_via_transition(self):
        a,b,c = 10*maze.W+10,10*maze.W+15,15*maze.W+15
        path = [(a,0),(b,0),(b,2),(c,2)]
        masks = [bytes(maze.W*maze.H) for _ in range(3)]
        result = maze.simplify(masks,maze.merge_collinear(path))
        segments,vias = maze.path_to_items(result,'A',0.2)
        self.assertEqual(len(vias),1)
        self.assertEqual({s['layer'] for s in segments},{'F.Cu','B.Cu'})


class ArtifactTests(unittest.TestCase):
    def test_thermal_override_requires_the_explicit_ground_return(self):
        node=ksexp.parse('(kicad_pcb (footprint "U" (property "Reference" "U5") '
                         '(pad "2" smd roundrect (at -1.1375 0) (net "GND"))))')
        with self.assertRaises(ValueError):
            apply_thermal(node,45)

    def test_silk_is_idempotent_and_does_not_change_copper(self):
        node = ksexp.parse('(kicad_pcb (segment (start 1 2) (end 3 4) '
                           '(layer "B.Cu") (net "A") (width 0.2)))')
        copper = copy.deepcopy(ksexp.children(node,'segment'))
        apply_silk(node)
        first = copy.deepcopy(node)
        apply_silk(node)
        self.assertEqual(node,first)
        self.assertEqual(ksexp.children(node,'segment'),copper)
        self.assertEqual(len(ksexp.children(node,'group')),1)

    def test_wordmark_counters_keep_their_area(self):
        from shapely.geometry import Polygon
        art = json.loads(Path(__file__).with_name('assets').joinpath('cicala-wordmark.json').read_text())
        for p in art['polygons']:
            expected = Polygon(p['outline'],p['holes'])
            flat = Polygon(keyhole(p['outline'],p['holes']))
            self.assertAlmostEqual(flat.area,expected.area,places=6)

    def test_stub_cleanup_preserves_via_that_bridges_a_gap(self):
        geom = board([pad(x=2),pad(x=8,uuid='b')])
        geom.update(segments=[
            {'net':'A','layer':'B.Cu','start':[2,5],'end':[4.8,5],'width':0.2,'uuid':'s1'},
            {'net':'A','layer':'B.Cu','start':[5.2,5],'end':[8,5],'width':0.2,'uuid':'s2'}],
            vias=[{'net':'A','at':[5,5],'size':0.6,'drill':0.3,'uuid':'v1'}])
        drc = {'violations':[{'type':'via_dangling','items':[{'uuid':'v1'}]}]}
        result = clean(geom,geom,drc)
        self.assertEqual(len(result['vias']),1)
        self.assertEqual(len(result['dangling_cleanup']['retained']),1)

    def test_chamfer_preserves_connected_pad_chain(self):
        geom=board([pad(x=2,y=5),pad(x=5,y=8,uuid='b')])
        tracks={'segments':[
            {'net':'A','layer':'B.Cu','start':[2,5],'end':[5,5],'width':0.2},
            {'net':'A','layer':'B.Cu','start':[5,5],'end':[5,8],'width':0.2}], 'vias':[]}
        result=chamfer(geom,tracks,{'segments':[],'vias':[]})
        self.assertEqual(result['chamfered_corners'],1)
        self.assertEqual(len(physical_groups('A',geom,result)),1)
        self.assertEqual(chamfer(geom,tracks,tracks)['chamfered_corners'],0)

    def test_duplicate_ids_are_fixed_deterministically(self):
        node = ksexp.parse('(kicad_pcb (uuid "board") '
            '(footprint "R" (property "Reference" "R1") (uuid "same")) '
            '(footprint "R" (property "Reference" "R2") (uuid "same")))')
        twin = copy.deepcopy(node)
        self.assertEqual(normalize(node),2)
        normalize(twin)
        self.assertEqual(node,twin)
        self.assertEqual(normalize(node),0)
        ids = [n[1] for _,n in uuid_nodes(node)]
        self.assertEqual(len(ids),len(set(ids)))

    def test_writer_uses_emitted_via_dimensions(self):
        with tempfile.TemporaryDirectory() as work:
            path = Path(work)/'board.kicad_pcb'
            data = Path(work)/'tracks.json'
            path.write_text('(kicad_pcb)')
            data.write_text(json.dumps({'segments':[],'vias':[
                {'net':'VBAT','at':[5,5],'size':0.45,'drill':0.2}]}))
            subprocess.run([sys.executable,str(Path(__file__).with_name('apply_tracks.py')),
                            str(path),str(data)],check=True,capture_output=True)
            via = ksexp.children(ksexp.load(path),'via')[0]
            self.assertEqual(float(ksexp.child(via,'size')[1]),0.45)
            self.assertEqual(float(ksexp.child(via,'drill')[1]),0.2)
            first=path.read_bytes()
            subprocess.run([sys.executable,str(Path(__file__).with_name('apply_tracks.py')),
                            str(path),str(data)],check=True,capture_output=True)
            self.assertEqual(path.read_bytes(),first)

    def test_usb_channels_and_connector_branches_match(self):
        geom = board()
        geom['outline'] = [(3,3.5),(81,3.5),(81,52.5),(3,52.5)]
        seed = make_pair(geom,validate_placement=False)
        m = seed['usb_metrics']
        self.assertAlmostEqual(m['dp_copper_mm'],m['dn_copper_mm'],places=6)
        self.assertAlmostEqual(m['connector_dp_branch_mm'],m['connector_dn_branch_mm'],places=6)
        self.assertEqual(m['test_point_stub_mm'],0)
        self.assertTrue(all(v['drill'] >= 0.2 for v in seed['vias']))

    def test_usb_seed_rejects_missing_placement(self):
        with self.assertRaises(ValueError):
            make_pair(board())


if __name__ == '__main__':
    unittest.main()
