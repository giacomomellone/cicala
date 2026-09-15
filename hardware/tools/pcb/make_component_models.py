"""Generate original, simplified STEP bodies in KiCad footprint coordinates.

Requires CadQuery. Models are mechanical envelopes, not supplier CAD or a
substitute for connector mating, optical, tolerance or assembled-fit checks.
See cicala_rev_a/models/README.md for dimensions and primary references.
"""
from pathlib import Path
import cadquery as cq

ROOT = Path(__file__).resolve().parents[2] / "rev_a/pcb"
OUT = ROOT / "models"
METAL = cq.Color(0.72, 0.73, 0.75)
DARK = cq.Color(0.12, 0.12, 0.13)
WHITE = cq.Color(0.88, 0.86, 0.79)


def box(w, d, h, x=0, y=0, z=0):
    return cq.Workplane("XY").box(w, d, h, centered=(True, True, False)).translate((x, y, z))


def write(name, items):
    assembly = cq.Assembly(name=name)
    for index, (shape, color) in enumerate(items):
        assembly.add(shape, name=f"body_{index}", color=color)
    path = OUT / f"{name}.step"
    assembly.save(str(path))
    path.write_text("".join(line.rstrip() + "\n" for line in path.read_text().splitlines()))


def main():
    OUT.mkdir(exist_ok=True)
    # Model Y is the negative of footprint Y; +Z is above an F.Cu land.
    outer = box(8.94, 7.35, 3.26).edges("|Y").fillet(1.2)
    bore = box(8.34, 7.6, 2.56, z=.35).edges("|Y").fillet(.95)
    shell = outer.cut(bore)
    write("HRO_TYPE_C_31_M_12_envelope", [
        (shell, METAL), (box(7.8, 1.0, 2.5, y=3.1, z=.4), DARK),
        (box(6.6, 5.1, .65, y=-.2, z=1.325), DARK),
    ])
    # JST ePH: SMT side-entry body, 9.9 x 6.0 x 5.5 mm, excluding mating plug.
    housing = box(9.9, 6, 5.5, y=-1.4).cut(box(7.8, 5.6, 3.7, y=-2, z=.7))
    items = [(housing, WHITE)]
    items += [(box(.5, 5.0, .5, x=x, y=-1, z=2.25), METAL) for x in (-2, 0, 2)]
    write("JST_S3B_PH_SM4_TB_envelope", items)
    write("TPS63802_DLA0010A_envelope", [(box(2, 3, 1), DARK)])
    write("Coilcraft_XFL4015_envelope", [(box(4, 4, 1.6), DARK)])
    write("TDK_VLS4012CX_envelope", [(box(4, 4, 1.2), DARK)])
    # Kingbright DSAQ1665: 0.6 mm mounted height; emitting face at local +Y.
    write("Kingbright_APBA2006_envelope", [
        (box(2, .5, .6, y=-.02), WHITE),
        (box(1.5, .55, .6, y=-.545), cq.Color(.76, .82, .68)),
    ])


if __name__ == "__main__":
    main()
