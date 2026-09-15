# Rev B enclosure

`cicala_enclosure.scad` is the editable source. `contract.scad` and `pcb_component_bounds.scad` are generated from the numeric contract and KiCad placement.

The core is 108 × 66 × 10.2 mm. Its active display is centered, the battery occupies the PCB's open corner, and two guided vertical controls sit on the beveled shoulder. The removable magnetic carrier adds 3.2 mm. Separate display supports keep assembly loads off the battery.

Run `just hw-rev-b-export --renders` for checked native exports, or `just hw-rev-b-fit-export` for a clearly labeled mechanical-only check. Printable parts include the shell, display supports, caps, removable keepers, carrier/cover, wedge, forming tool and tolerance coupons. `exports/assembly/` preserves assembly coordinates; `exports/print/` contains parts oriented and placed on the print bed, grouped by purpose. Reference components and cut patterns have separate folders. Print settings and assembly sequence are in [the engineering guide](../../../docs/hardware_rev_b.md).

Each cap has 1.4 mm retaining tabs, a captured 0.5 mm silicone strip and a keeper attached with two M2 screws. Install these in the empty top shell. Clearance, cap-relief and keeper-stop grades support selection from first-article measurements; the default parts do not guarantee actuation across the tolerance stack.

The R1.55 inside flex radius and connector entry height are assembly assumptions. The R1.5 forming tool is temporary and must be removed. Verify the actual flex, lens/support tolerances, battery clearance, button travel, fasteners and phone fit on first articles. Native CAD intersection checks describe nominal geometry, not a complete tolerance qualification.
