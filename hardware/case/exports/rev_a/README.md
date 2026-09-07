# Rev A print and cutting files

Generated from `../../cicala_enclosure.scad` with OpenSCAD 2026.06.12.
All thirteen STL/3MF parts are millimetres, start at Z=0 and pass closed,
oriented, single-solid mesh checks. The 84 × 56 × 24.05 mm case has a flat face.
The shell and caps are already face-down for printing. Do not scale the files.

Print the four coupons, both caps and `button_stop` before a full set. The
stop is a separate insulating plate above the PCB and needs stroke adjustment
against the fitted switches. `lens` and `steel_skin`
are fit surrogates; `lens_cut.svg` and `steel_cut.svg` provide 1:1 sheet-part
outlines. Materials, countersinks, screw selection, print settings and assembly
instructions are in [the case README](../../README.md).

Run `just hw-case-export` after source changes. The checked assembly includes
the rotated display flex, fitted PCB components, protected battery reserve,
NTC and JST/harness corridor. These are nominal CAD checks; no physical print
or assembled-fit test has been performed.
