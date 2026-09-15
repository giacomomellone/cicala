# Rev B assembly bill of materials

The exact fitted PCB component list is generated from schematic fields into
`pcb/exports/assembly/cicala_rev_b_bom.csv`. The factory-specific BOM and placement
files are generated from that list and the native PCB. Empty distributor IDs
require sourcing the specified MPN; they do not authorize substitutions.

The following parts complete one standalone prototype. Dimensions and assembly
allowances are in [the engineering guide](../../docs/hardware_rev_b.md).

| Quantity | Part | Selection |
| --- | --- | --- |
| 1 | Populated PCB | Rev B, 1.2 mm four-layer FR-4, ENIG, top assembly |
| 1 | Display | Waveshare 22609, 3.52-inch V1.1 UC8253 |
| 1 | Protected cell | Renata ICP303450PA-02 / 100701, 500 mAh minimum |
| 1 | Temperature sensor | Semitec 103JT-025, 10 kΩ B3435 thin film |
| 1 | Battery plug | JST SHR-03V-S |
| 3 | Crimp contacts | JST SSH-003T-P0.2 |
| 2 | Power pigtails | AWG28, each ≤40 mm; insulated splices to stock pack leads |
| 1 set | Sensor wiring and insulation | Route without loading the sensor head; bond to the cell through electrical insulation |
| 1 | Lens blank | Clear 0.8 mm sheet, cut to `case/exports/patterns/lens_cut.svg`; kerf and material fit to be established on a coupon |
| 1 set | Printed core | Base, top shell, display frame, support bar, category and next caps, two keepers |
| 2 | Captured silicone strips | Solid 0.50 mm silicone, nominal 50 Shore A; cut 1.6 × 8.8 mm and 1.6 × 11.8 mm using the patterns |
| 4 | Keeper fasteners | M2 × 3 mm countersunk metal screws; nominal 4 mm head, qualify 1.65 mm printed pilot engagement |
| 1 set | Display foam and attachment | Nominal 0.2 mm removable perimeter foam; keep the active screen and flex bond unloaded |
| 1 set | Battery attachment and insulation | Within the 54 × 36 × 4.8 mm reservation; preserve the pack's protection board |
| 4 | Core fasteners | M2.5 × 8 mm countersunk screws; confirm engagement and tip clearance in the printed pilot coupon |

The remaining meshes are coupons, assembly tools and inert references. The flex
former is removed after forming. The reference battery, panel and PCB meshes are
not installed alongside the real components.

## Optional accessories

The wedge is a separate print. The magnetic carrier uses its printed body and
cover, the made-to-drawing CIC-MAG-01 array and shield in `contract.json`, and four
M2.5 × 10 mm screws in place of the core screws. The carrier adds 3.2 mm. No
catalogue magnet is a verified substitute; the array, shield, adhesive stack and
phone compatibility need supplier and first-article qualification.

These are engineering selections, not stock guarantees or a production release.
The prototype must be assembled and tested before accepting the hardware.
