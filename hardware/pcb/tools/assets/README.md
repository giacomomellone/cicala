# Silkscreen artwork

`cicala-mark.json` is derived from the public
`website/public/brand/cicala-mark.svg`. It stores the SVG's SHA256 and native
cubic curves and lines, centred in an 8 mm square. The selected mark is stroked
throughout: open wings and the round, open head all use 0.513 mm. The converter
still emits filled polygons for any filled path, and flattens their curves
within 0.002 mm; the current artwork has none.
`apply_silkscreen.py` mirrors it onto `B.SilkS` at (61, 8.5) mm beside the
wordmark. The group is named `Cicala cicada mark` in KiCad.

The stroked mark is about 8.05 mm across because half of the outer stroke falls
outside the nominal 8 mm square. Keep that in mind when judging clearance to
the wordmark and the board edge; DRC checks the result.

Regenerate with fontTools installed, then apply just the mark to preserve
existing labels, reference positions and all electrical geometry:

```sh
python3 hardware/pcb/tools/make_cicada.py \
  website/public/brand/cicala-mark.svg \
  hardware/pcb/tools/assets/cicala-mark.json
python3 hardware/pcb/tools/apply_silkscreen.py \
  hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pcb \
  hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pcb --mark-only
```

`cicala-wordmark.json` is polygon artwork for the public lowercase wordmark,
set in Literata Latin 400 normal with -0.035 em tracking at 22 mm ink width.
It contains only this word's outlines, not an embedded font program. The
board uses native filled silkscreen polygons, mirrored for underside reading.

Font source: the website's pinned `@fontsource/literata` package. Copyright
2017 The Literata Project Authors (https://github.com/googlefonts/literata),
SIL Open Font License 1.1. The font license does not apply to documents made
with it. The project's brand identity is documented in `docs/brand.md`.

Regeneration from the repository root, with fontTools, Brotli and Shapely
installed in system Python:

```sh
python3 hardware/pcb/tools/make_wordmark.py \
  website/node_modules/@fontsource/literata/files/literata-latin-400-normal.woff2 \
  hardware/pcb/tools/assets/cicala-wordmark.json
```

Ordinary board generation reads the committed artwork and does not require
Node, website dependencies, fontTools or Brotli. Regenerate the fabrication
exports after updating the board.
