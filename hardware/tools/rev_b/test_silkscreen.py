"""Protect electrical geometry and the meaning of the printed service guide."""
import copy
from pathlib import Path
import tempfile
import unittest

from silkscreen import BOARD, apply, check_signals, k, save_preserving


def engineering_geometry(board):
    board = copy.deepcopy(board)
    for item in list(board):
        if not isinstance(item, list):
            continue
        layer = k.child(item, 'layer')
        if item[0] == 'group' or (item[0] == 'gr_text') or (
                layer and layer[1] in ('F.SilkS', 'B.SilkS')):
            board.remove(item)
        elif item[0] == 'footprint':
            for graphic in list(item):
                if isinstance(graphic, list):
                    layer = k.child(graphic, 'layer')
                    if layer and layer[1] in ('F.SilkS', 'B.SilkS'):
                        item.remove(graphic)
    return board


class SilkscreenTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.board = k.load(BOARD)

    def test_repeatable_without_changing_electrical_or_mechanical_geometry(self):
        board = copy.deepcopy(self.board)
        before = engineering_geometry(board)
        apply(board)
        self.assertEqual(board, self.board, 'Regenerate the committed silkscreen')
        first = copy.deepcopy(board)
        apply(board)
        self.assertEqual(board, first)
        self.assertEqual(engineering_geometry(board), before)

    def test_changed_probe_or_connector_net_fails_before_editing(self):
        for ref, pin in [('TP7', '1'), ('J3', '1'), ('J4', '5')]:
            board = copy.deepcopy(self.board)
            fp = next(f for f in k.children(board, 'footprint') if k.ref_of(f) == ref)
            pad = next(p for p in k.children(fp, 'pad') if str(p[1]) == pin)
            k.child(pad, 'net')[1] = 'WRONG_NET'
            before = copy.deepcopy(board)
            with self.assertRaisesRegex(ValueError, ref):
                apply(board)
            self.assertEqual(board, before)

    def test_new_probe_cannot_be_silently_omitted(self):
        board = copy.deepcopy(self.board)
        board.append(k.parse('(footprint "TestPoint" (property "Reference" "TP99"))'))
        with self.assertRaisesRegex(ValueError, 'coverage'):
            check_signals(board)

    def test_back_text_is_mirrored_and_front_text_is_not(self):
        for text in k.children(self.board, 'gr_text'):
            layer = k.child(text, 'layer')[1]
            if layer not in ('F.SilkS', 'B.SilkS'):
                continue
            justify = k.child(k.child(text, 'effects'), 'justify') or []
            self.assertEqual('mirror' in justify, layer == 'B.SilkS', text[1])

    def test_native_zone_format_and_quoted_parentheses_are_preserved(self):
        original = ('(kicad_pcb\n\t(version 1)\n\t(zone (name "a (b) \\"c\\"")\n'
                    '\t\t(polygon (pts (xy 1 2)\n\t\t\t(xy 3 4))))\n)\n')
        board = k.parse(original)
        board.append(k.parse('(gr_text "new" (layer "F.SilkS"))'))
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'test.kicad_pcb'
            save_preserving(path, original, board)
            self.assertIn(original.split('\t(zone', 1)[1].rsplit('\n)', 1)[0], path.read_text())
            self.assertEqual(k.load(path), board)


if __name__ == '__main__':
    unittest.main()
