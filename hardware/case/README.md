# Case — prototype direction

STEP/3MF exports plus FreeCAD source will live here (phase D). Licensed [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

The current form proposal, concept renders, mechanical stack, and validation
gates live in [docs/device_prototype.md](../../docs/device_prototype.md). The
rough 84 × 56 × 16 mm envelope is a prototype target, not a frozen
manufacturing dimension.

Envelope requirements from the product design ([docs/design.md](../../docs/design.md)):

- Pocketable; the e-paper window is the face. The device lies flat mid-table like a place card.
- Two adjacent buttons: a small, nearly flush round Category cap on the left
  and a larger, raised rounded-pill Next cap on the right. Each cap needs an
  overload stop independent of the PCB.
- The active category is printed on the e-paper above the question; no category
  legend is printed on the shell.
- USB-C on the bottom edge; no visible LEDs on the tabletop faces.
- Battery: 503035 LiPo (30 × 35 × 5 mm) behind the display stack.
- Two-part, reopenable shell; no destructive adhesive for the cell, display, or PCB.
- E-paper glass supported around its perimeter and protected from spills and point loads.
- Tested ingress paths around both button caps, the lens, USB opening, and shell seam.
