import copy
import json
import unittest
from check_contract import BOARD
from generate_contract import CONTRACT
import ksexp as k
from switch_footprint import check

class SwitchFootprintTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.board=k.load(BOARD);cls.contract=json.loads(CONTRACT.read_text())
    def test_selected_terminal_pairs_and_boss_holes(self):check(self.board,self.contract)
    def test_reversed_button_order_is_rejected(self):
        contract=copy.deepcopy(self.contract)
        contract['buttons']['centres'].reverse()
        with self.assertRaisesRegex(ValueError,'above'):check(self.board,contract)
    def test_missing_positioning_boss_is_rejected(self):
        b=copy.deepcopy(self.board);fp=next(f for f in k.children(b,'footprint') if k.ref_of(f)=='SW1')
        fp.remove(next(p for p in k.children(fp,'pad') if not p[1]))
        with self.assertRaisesRegex(ValueError,'holes'):check(b,self.contract)
    def test_accidentally_shorted_terminal_pair_is_rejected(self):
        b=copy.deepcopy(self.board);fp=next(f for f in k.children(b,'footprint') if k.ref_of(f)=='SW2')
        p=next(p for p in k.children(fp,'pad') if p[1]=='1');k.child(p,'net')[1]='GND'
        with self.assertRaisesRegex(ValueError,'terminal-pair'):check(b,self.contract)

if __name__=='__main__':unittest.main()
