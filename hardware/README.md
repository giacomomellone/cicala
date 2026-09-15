# Hardware

Each revision owns its PCB, enclosure, exported parts and manufacturing notes.
Open the revision's KiCad project so its local libraries and models resolve.

| Directory | Design |
| --- | --- |
| [rev_a](rev_a/README.md) | Original 78 × 49 mm board and deeper enclosure |
| [rev_b](rev_b/README.md) | Landscape device; L-shaped board beside a thin battery |
| [tools](tools/README.md) | Shared checks and revision-specific build commands |

Native KiCad and OpenSCAD files are authoritative. Generated exports belong
inside their revision. Temporary candidates and logs go in the ignored
`build/` directory. Hardware is licensed under [CERN-OHL-S-2.0](../LICENSE-HARDWARE).
