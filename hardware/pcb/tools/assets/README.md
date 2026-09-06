# Silkscreen artwork

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
Node, website dependencies, fontTools or Brotli.
