# Rev B enclosure exports

Generated from OpenSCAD by `just hw-rev-b-export --renders`. `manifest.json`
records the exact inputs and geometry hashes. Mechanical-only exports explicitly
record the omitted electrical checks.

- `assembly/`: eight printed parts in native assembly coordinates.
- `print/core/`: base, top shell, display frame and support bar.
- `print/accessories/`: optional carrier, cover and wedge.
- `print/jigs/`: temporary flex former, removed after forming.
- `reference/`: purchased caps, switch, PCB, battery and panel; do not print.
- `patterns/`: outline for cutting the purchased clear lens sheet.

`print-layout.json` records orientation, bed translation and size for each print.
3MF contains geometry, not a qualified printer profile. Filters uses an ivory
B32-1200 cap; Next an orange B32-1320 cap. Both fit purchased B3F-4050 switches
after soldering. There are no printed caps, silicone strips or keepers.

Read the [assembly guide](../../../../docs/hardware_rev_b.md#buttons-and-print-preparation)
for print preparation, cap installation and first-article checks.
