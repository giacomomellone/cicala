# Rev B assembly bill of materials

The exact fitted PCB component list is generated from schematic fields into
`pcb/exports/assembly/cicala_rev_b_bom.csv`. The factory-specific BOM and placement
files are generated from that list and the native PCB. The JLC BOM/CPL contains 87 SMT references; `manual_assembly.csv` lists both through-hole switches and the two purchased caps. The full native BOM/position files retain all 89 electronic references. Empty distributor IDs
require sourcing the specified MPN; they do not authorize substitutions.

The following parts complete one standalone prototype. Dimensions and assembly
allowances are in [the engineering guide](../../docs/hardware_rev_b.md).

| Quantity | Part | Selection |
| --- | --- | --- |
| 1 | Populated PCB | Rev B, 1.2 mm four-layer FR-4, ENIG; 87 SMT parts plus 2 through-hole switches |
| 1 | Display | Waveshare 22609, 3.52-inch V1.1 UC8253 |
| 1 | Protected cell | Renata ICP303450PA-02 / 100701, 500 mAh minimum |
| 1 | Temperature sensor | Semitec 103JT-025, 10 kΩ B3435 thin film |
| 1 | Battery plug | JST SHR-03V-S |
| 3 | Crimp contacts | JST SSH-003T-P0.2 |
| 2 | Power pigtails | AWG28, each ≤40 mm; insulated splices to stock pack leads |
| 1 set | Sensor wiring and insulation | Route without loading the sensor head; bond to the cell through electrical insulation |
| 1 | Lens blank | Clear 0.8 mm sheet, cut to `case/exports/patterns/lens_cut.svg`; kerf and material fit to be established on a coupon |
| 1 set | Printed core | Base, top shell, display frame, support bar |
| 2 | Switches (included in PCB BOM) | Omron B3F-4050, through-hole; solder after SMT |
| 1 | Filters cap | Omron B32-1200, ivory, 9 × 9 mm; upper control |
| 1 | Next cap | Omron B32-1320, orange, 12 × 12 mm; lower control |
| 1 set | Display foam and attachment | Nominal 0.2 mm removable perimeter foam; keep the active screen and flex bond unloaded |
| 1 set | Battery attachment and insulation | Within the 54 × 36 × 4.8 mm reservation; preserve the pack's protection board |
| 4 | Core fasteners | M2.5 × 10 mm countersunk screws; confirm engagement and tip clearance in the printed posts |

The remaining meshes are assembly tools and purchased-part references. The flex
former is removed after forming. The reference battery, panel and PCB meshes are
not installed alongside the real components.

## Optional accessories

The wedge is a separate print. The magnetic carrier uses its printed body and
cover, the made-to-drawing CIC-MAG-01 array and shield in `contract.json`, and four
M2.5 × 12 mm screws in place of the core screws. The carrier adds 3.2 mm. No
catalogue magnet is a verified substitute; the array, shield, adhesive stack and
phone compatibility need supplier and first-article qualification.

These are engineering selections, not stock guarantees or a production release.
The prototype must be assembled and tested before accepting the hardware.
