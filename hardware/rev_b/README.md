# Rev B

Landscape prototype with a centered 3.52-inch display, ESP32-S3 module,
L-shaped PCB, 500 mAh battery and two flat, recessed Omron controls at the right: Filters above the larger Next button.
The core envelope is 128 × 66 × 16.8 mm (1 mm nominal button recess). The phone carrier is removable.

The routed board and nominal mechanical design pass the digital checks.
This is an engineering prototype; physical qualification and factory order
review remain open. See [validation](validation.md) for the measured scope.

- `contract.json` — shared mechanical dimensions and manufacturing limits
- `pcb/` — native KiCad project, local libraries and component models
- `case/` — OpenSCAD enclosure and generated geometry
- [Complete assembly BOM](BOM.md)
- [Fabrication and assembly order notes](pcb/fabrication_notes.md)
- `component_selection.csv` — selected components and rationale
- [Hardware specification](../../docs/hardware_rev_b.md)
- [Component evidence](../../docs/hardware_rev_b_selection.md)

Digital validation and first-article measurements are separate records.
The first assembled board is needed to qualify electrical behavior, printed
fits, display flex, temperature sensing and the optional phone carrier.
