"""Deterministic electrical placement refinements after place.py.

Run with KiCad Python: refine_placement.py input.kicad_pcb output.kicad_pcb
Reapply project settings after saving. Existing tracks must be regenerated.
"""
import sys
import pcbnew

from unique_ids import normalize_file

# The TPS63802 feedback connection stays beside FB, away from L1/L2.
POSITIONS = {
    'R14': (69.2, 19.75, 270),
    'R15': (72.0, 18.4, 0),
    'U4': (42.0, 42.4, 90),
    'R3': (25.8, 9.0, 270),
    'R4': (28.2, 9.0, 270),
    'TP13': (25.8, 11.9, 0),
    'TP12': (28.2, 11.9, 0),
    'C1': (23.7, 11.4, 90),
    'C2': (30.3, 11.4, 90),
    'C28': (11.5, 11.35, 90),
    'C27': (13.5, 11.35, 90),
    'C29': (15.5, 11.35, 90),
    'C30': (18.0, 11.2, 90),
    'R25': (19.95, 11.35, 90),
    'R26': (21.85, 11.35, 90),
    'TP1': (48.8, 44.2, 0),
    'TP2': (48.0, 41.7, 0),
    'TP16': (51.1, 41.5, 0),
    'TP17': (51.1, 48.7, 0),
    'R21': (15.6, 5.7, 90),
    'C25': (17.8, 5.7, 90),
    'C26': (20.0, 5.7, 90),
    'R22': (22.2, 5.7, 90),
    'C15': (13.0, 38.3, 0),
    'R20': (17.35, 34.65, 270),
    'Q1': (19.75, 35.0, 0),
    'D3': (24.0, 35.5, 180),
    'C4': (74.9, 16.0, 270),
    'C5': (69.2, 16.35, 270),
    'C6': (68.85, 12.85, 90),
    'R8': (78.2, 16.2, 270),
    'U3': (72.0, 21.5, 0),
    'L2': (77.55, 21.5, 90),
    'C9': (74.5, 19.15, 90),
    'C10': (76.0, 25.7, 270),
    'C11': (72.0, 24.4, 0),
    'C8': (69.2, 23.2, 270),
    'TP9': (76.1, 29.5, 90),
    'TP20': (15.5, 46.7, 90),
    'C24': (20.3, 41.85, 90),
    'C23': (22.0, 41.85, 90),
    'C22': (23.7, 41.85, 90),
    'C17': (25.4, 41.85, 90),
    'C21': (27.1, 41.85, 90),
    'C20': (28.8, 41.85, 90),
    'C19': (30.5, 41.85, 90),
    'C18': (32.2, 41.85, 90),
    'TP3': (20.55, 37.4, 90),
    'R19': (18.6, 38.65, 270),
    'R18': (17.5, 41.4, 0),
    'R31': (53.3, 38.1, 90),
    'R32': (55.5, 38.1, 90),
    'TP19': (57.7, 38.1, 90),
    'R12': (44.5, 38.1, 90),
    'C12': (11.2, 44.4, 270),
    'C13': (15.975, 43.85, 270),
    'C14': (16.05, 39.0, 90),
    'TP6': (55.7, 49.0, 90),
    'TP21': (48.9, 38.1, 90),
}

LEGENDS = [
    ('RECOVERY',55.8,43.05),
    ('1 3V3',55.8,43.9), ('2 GND',55.8,44.65),
    ('3 EN',55.8,45.4), ('4 BOOT',55.8,46.15),
    ('5 TX',55.8,46.9), ('6 RX',55.8,47.65),
    ('USB 5V',49.0,51.0), ('LiPo / NTC',76.1,45.1),
    ('+',78.5,42.0), ('T',78.5,40.0), ('-',78.5,38.0),
    ('CICALA A0',71.0,46.0),
]


def main():
    board = pcbnew.LoadBoard(sys.argv[1])
    for fp in board.GetFootprints():
        if fp.GetReference() in POSITIONS:
            x, y, angle = POSITIONS[fp.GetReference()]
            if not fp.IsFlipped():
                fp.Flip(fp.GetPosition(), False)
            fp.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)))
            fp.SetOrientationDegrees(angle)
        if fp.GetReference() in {'U1','J3'}:
            x,y,angle = {'U1':(30.8,22.5,90),'J3':(76.8,42.7,0)}[fp.GetReference()]
            field = fp.Reference()
            field.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x),pcbnew.FromMM(y)))
            field.SetTextAngle(pcbnew.EDA_ANGLE(angle,pcbnew.DEGREES_T))
        field = fp.Reference()
        field.SetTextAngle(pcbnew.EDA_ANGLE(field.GetTextAngle().AsDegrees()%180,pcbnew.DEGREES_T))
        if field.GetLayer() in (pcbnew.F_Fab,pcbnew.B_Fab):
            field.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.8),pcbnew.FromMM(0.8)))
        if fp.GetReference()=='J4':
            for pad in fp.Pads():
                if pad.GetNetname() in {'GND','3V3'}:
                    pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
    for drawing in list(board.GetDrawings()):
        if isinstance(drawing,pcbnew.PCB_TEXT) and drawing.GetText() in {t for t,x,y in LEGENDS}:
            board.Remove(drawing)
    for label,x,y in LEGENDS:
        text = pcbnew.PCB_TEXT(board)
        text.SetText(label)
        text.SetLayer(pcbnew.B_SilkS)
        text.SetMirrored(True)
        text.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x),pcbnew.FromMM(y)))
        text.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.65),pcbnew.FromMM(0.65)))
        text.SetTextThickness(pcbnew.FromMM(0.12))
        board.Add(text)
    pcbnew.SaveBoard(sys.argv[2], board)
    normalize_file(sys.argv[2])


if __name__ == '__main__':
    main()
