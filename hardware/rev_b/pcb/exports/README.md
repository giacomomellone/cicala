# Rev B engineering prototype package

`cicala_rev_b_engineering_prototype.zip` contains checked Gerbers, Excellon drills,
assembly BOM and positions, JLCPCB BOM/CPL, assembly drawings, schematic PDF,
IPC-2581, STEP, fabrication notes and review reports. Its SHA-256 file is alongside it. `manifest.json`
records source/output hashes; native KiCad remains the editable source.

Generate with `just hw-rev-b-fab-export`. The export requires passing electrical,
USB-reference, temperature-model and mechanical checks before writing this
package. The fabrication readback confirms the expected Gerber layers and
compares all drill and slot positions, sizes and axes with the native board.

This is an unbuilt engineering prototype. Factory CAM, stack/impedance, parts,
stencil and panel approval belong to the prototype order. Physical qualification
is listed in [validation](../../validation.md). No order has been submitted.
