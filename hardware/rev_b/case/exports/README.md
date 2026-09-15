# Rev B enclosure exports

Generated from OpenSCAD by `just hw-rev-b-export --renders`. `manifest.json`
records the exact inputs and output hashes. Mechanical-only exports explicitly
record the omitted electrical checks.

- `assembly/`: native assembly coordinates for the installed parts and coupons.
- `print/core/`: individually oriented enclosure, supports, caps and keepers.
- `print/accessories/`: carrier, cover and wedge.
- `print/coupons/`: clearance, relief and stop trials for both buttons.
- `print/jigs/`: the temporary flex former.
- `reference/`: inert component and silicone reference models, not plastic parts to install.
- `patterns/`: lens and silicone outlines for cutting purchased sheet material.

`print-layout.json` records orientation, bed translation and bounding size for
each print. 3MF contains geometry, not a qualified printer profile. Print only the
graded cap/keeper options needed for fitting; the complete matrix is not the
assembly bill of materials.

The cap has 1.4 mm end tabs, a captured 0.5 mm silicone strip and a removable
keeper. Install it from below with the electronics absent. The current default
uses 0.5 mm running clearance, 0.1 mm relief and a 0.65 mm stop. Select the actual
pair on the switch coupon; the 192-corner screening is a displacement model, not
force or durability validation.

Read the [engineering guide](../../../../docs/hardware_rev_b.md#buttons-and-print-preparation)
for print orientation, material limitations, assembly order and physical checks.
