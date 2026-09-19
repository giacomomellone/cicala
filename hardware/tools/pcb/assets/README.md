# Silkscreen artwork

`cicala-mark.json` is derived from the public
`website/public/brand/cicala-mark.svg`. It stores the SVG's SHA256 and the
artwork centred in an 8 mm square. The selected mark is one filled silhouette,
so the file carries a single polygon and no stroked segments; the converter
still emits cubic curves and lines for any stroked path.

Filled paths are stored as an `outline` and the `holes` it encloses, nested by
the SVG's own even-odd fill rule, and their curves are flattened within
0.002 mm. `apply_silkscreen.py` bridges each hole to its outline with
`keyhole()` before writing the polygon, because KiCad's `gr_poly` is flat and
cannot express a counter. The two eye counters are approximately 0.75 mm across, well above
the 0.15 mm silkscreen minimum.

`apply_silkscreen.py` mirrors the mark onto `B.SilkS` at (61, 8.5) mm beside
the wordmark. The group is named `Cicala cicada mark` in KiCad. Rev B places
the same artwork at 0.7 scale on both sides through
`hardware/tools/rev_b/silkscreen.py`.

The filled mark is 6.34 mm wide and 7.98 mm high within the nominal 8 mm square, so
nothing falls outside the square; DRC checks clearance to the wordmark and the
board edge.

Regenerate with fontTools installed, then apply just the mark to preserve
existing labels, reference positions and all electrical geometry:

```sh
python3 hardware/tools/pcb/make_cicada.py \
  website/public/brand/cicala-mark.svg \
  hardware/tools/pcb/assets/cicala-mark.json
python3 hardware/tools/pcb/apply_silkscreen.py \
  hardware/rev_a/pcb/cicala_rev_a.kicad_pcb \
  hardware/rev_a/pcb/cicala_rev_a.kicad_pcb --mark-only
python3 hardware/tools/rev_b/silkscreen.py
```

Rev B has no `--mark-only` path: `silkscreen.py` rewrites the whole silkscreen
group and `test_silkscreen.py` fails until the committed board matches it
again.

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
python3 hardware/tools/pcb/make_wordmark.py \
  website/node_modules/@fontsource/literata/files/literata-latin-400-normal.woff2 \
  hardware/tools/pcb/assets/cicala-wordmark.json
```

Ordinary board generation reads the committed artwork and does not require
Node, website dependencies, fontTools or Brotli. Regenerate the fabrication
exports after updating the board.
