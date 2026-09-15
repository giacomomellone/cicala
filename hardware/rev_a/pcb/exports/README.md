# Generated Rev A outputs

The current fabrication files use **Matched 10 with a wider body**, the open
outline mark selected on 2026-09-10. The
[logo update record](review/logo_2026_09_10/verification.json) fingerprints the
artwork and regenerated exports, records the checks rerun, and confirms that
all PCB items outside the cicada group are unchanged. The
[preceding record](review/logo_2026_09_09/verification.json) covers the filled
mark it replaced.

`cicala_rev_a_prototype_2026-09-08.zip` is a historical source/review snapshot.
It retains the earlier logo and predates subsequent firmware, case-label and
schematic-symbol changes. Its inner Gerber ZIP is superseded by the current
`cicala_rev_a_gerbers.zip` beside it. The dated full-product evidence in
`review/verification.json` and `review/source_sha256.json` belongs to that
historical snapshot. A new complete snapshot requires a full review of the
intervening changes; the logo update does not requalify them.

Run `bash hardware/tools/rev_a/export_fab.sh` from the repository root after design
changes. `cicala_rev_a_gerbers.zip` is the fabrication upload; the separate
`cicala_rev_a_ipc2581.zip` contains IPC-2581. Assembly CSVs/PDFs are in
`assembly/`; all 89 fitted references agree between BOM and CPL. Bare contacts,
mounts and DNP parts are excluded. The STEP export contains every fitted body.

`review/` contains the schematic PDF, board/assembly views, independent Gerber
views and validation evidence. These are generated conveniences; KiCad sources
remain authoritative. The board views and the independent Gerber views are made
from the repository root with KiCad's renderer and Gerbonara, after
`export_fab.sh` has refreshed the ZIP:

```sh
for side in top bottom; do
  kicad-cli pcb render --side "$side" --width 1600 --height 1000 \
      --quality high --floor --background opaque \
      -o "hardware/rev_a/pcb/exports/review/board_$side.png" \
      hardware/rev_a/pcb/cicala_rev_a.kicad_pcb
  python3 -m gerbonara.cli render "--$side" \
      hardware/rev_a/pcb/exports/cicala_rev_a_gerbers.zip \
      "hardware/rev_a/pcb/exports/review/gerber_$side.svg"
done
cp hardware/rev_a/pcb/exports/assembly/assembly_*.pdf \
   hardware/rev_a/pcb/exports/review/
```

KiCad trims the requested render size, so 1600 × 1000 writes the committed
1568 × 984 images. Two runs of the same board differ in a few percent of
pixels from anti-aliasing; that is not a design change. See [the manufacturing specification](../../MANUFACTURING.md)
and [engineering review](../../REVIEW.md) for the factory handoff and the
physical acceptance tests still required on engineering prototypes.

The complete build/flash guide is [Rev A firmware](../../../../docs/firmware_rev_a.md).
`review/usb_audit.json` measures actual board routes; `review/contract.json`
compares PCB/case datums and compiled application/MCUboot settings.
`review/source_sha256.json` records the historical snapshot's file integrity.
It does not stand in for running the checks. After a verified source revision and updated
`review/verification.json`, regenerate/check the archive from the repo root:

```sh
python3 hardware/tools/pcb/package_prototype.py YYYY-MM-DD
python3 hardware/tools/pcb/package_prototype.py YYYY-MM-DD --check
```
