# Generated Rev A outputs

The current fabrication files use **Filled wings with Round arch**, selected
on 2026-09-09. The [logo update record](review/logo_2026_09_09/verification.json)
fingerprints the artwork and regenerated exports, records the checks rerun,
and confirms that all PCB items outside the cicada group are unchanged.

`cicala_rev_a_prototype_2026-09-08.zip` is a historical source/review snapshot.
It retains the earlier logo and predates subsequent firmware, case-label and
schematic-symbol changes. Its inner Gerber ZIP is superseded by the current
`cicala_rev_a_gerbers.zip` beside it. The dated full-product evidence in
`review/verification.json` and `review/source_sha256.json` belongs to that
historical snapshot. A new complete snapshot requires a full review of the
intervening changes; the logo update does not requalify them.

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
`review/source_sha256.json` records the historical snapshot's file integrity.
It does not stand in for running the checks. After a verified source revision and updated
`review/verification.json`, regenerate/check the archive from the repo root:

```sh
python3 hardware/pcb/tools/package_prototype.py YYYY-MM-DD
python3 hardware/pcb/tools/package_prototype.py YYYY-MM-DD --check
```
