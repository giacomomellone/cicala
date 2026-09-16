# Hardware tools

Run commands from the repository root through `just`.

| Directory | Responsibility |
| --- | --- |
| `pcb/` | KiCad parsing, geometry, assembly export and electrical/copper audits; historical Rev A routing helpers |
| `case/` | STEP-derived component bounds and mesh checks |
| `rev_a/` | Rev A review and manufacturing/enclosure exports |
| `rev_b/` | Rev B contract, routing support, review and exports |

Native board files are the source of truth. Routing helpers generate review
candidates; they do not reproduce a finished board by replaying an old script.
Keep candidate files and logs under `build/`, outside the source directories.

Rev B's `silkscreen.py` maintains the native identity and service legends and
checks their pin mappings. `just hw-rev-b-pcb-renders` regenerates orthographic
KiCad PNGs, mirrored underside/normal top silkscreen SVGs and their provenance.
