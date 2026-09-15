# Rev B enclosure

`cicala_enclosure.scad` is editable. `contract.scad` and `pcb_component_bounds.scad` are generated from the contract and native KiCad placement.

The housing is 128 × 66 × 14.2 mm; the purchased cap tops reach 15.8 mm overall. The active screen remains centered. Filters uses a 9 × 9 mm ivory B32-1200 cap; Next uses a 12 × 12 mm orange B32-1320 cap. Both seat directly on B3F-4050 switches. The enclosure supplies clearance openings, with no printed actuators or retaining mechanism.

Run `just hw-rev-b-export --renders` for checked native exports. `exports/assembly/` preserves assembly coordinates; `exports/print/` contains eight individually oriented printed parts, grouped by purpose. Purchased cap, switch, battery, panel and PCB models stay in `exports/reference/` and must not be printed. The clear lens outline is in `exports/patterns/`.

Four core prints form the housing and display supports. The other prints are the optional carrier, ring cover, wedge and temporary flex former. See [the assembly guide](../../../docs/hardware_rev_b.md#buttons-and-print-preparation) for soldering, cap installation, print tolerances and screws.

The carrier adds 3.2 mm. Its increased portrait length fails the former iPhone 16 Plus fit target; phone compatibility is unqualified. The R1.55 flex bend and connector entry height remain first-article checks. No integrated Rev B prototype has been assembled.
