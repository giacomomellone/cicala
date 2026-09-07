# Generated Rev A outputs

`cicala_rev_a_prototype_2026-09-07.zip` is the dated review snapshot containing
the manufacturing files, current public source files (including firmware),
enclosure sources/prints, instructions and checksums. Git metadata and downloaded
dependencies are excluded. Unpack it first; upload its inner `cicala_rev_a_gerbers.zip`
to the PCB service. The dated snapshot must be replaced after a design change.

Run `bash hardware/pcb/export_fab.sh` from the repository root after design
changes. `cicala_rev_a_gerbers.zip` is the fabrication upload; the separate
`cicala_rev_a_ipc2581.zip` contains IPC-2581. Assembly CSVs/PDFs are in
`assembly/`; all 89 fitted references agree between BOM and CPL. Bare contacts,
mounts and DNP parts are excluded. The STEP export contains every fitted body.

`review/` contains the schematic PDF, board/assembly views, independent Gerber
views and validation evidence. These are generated conveniences; KiCad sources
remain authoritative. See [the manufacturing specification](../../MANUFACTURING.md)
and [engineering review](../../REVIEW.md) for the factory handoff and the
physical acceptance tests still required on engineering prototypes.

The complete build/flash guide is [Rev A firmware](../../../../docs/firmware_rev_a.md).
`review/usb_audit.json` measures actual board routes; `review/contract.json`
compares PCB/case datums and compiled application/MCUboot settings.
`review/source_sha256.json` records the snapshot's file integrity. It does not
stand in for running the checks. After a verified source revision and updated
`review/verification.json`, regenerate/check the archive from the repo root:

```sh
python3 hardware/pcb/tools/package_prototype.py 2026-09-07
python3 hardware/pcb/tools/package_prototype.py 2026-09-07 --check
```
