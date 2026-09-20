# Device page imagery

## Xteink X4 Pro / CrossPoint

`x4pro-home.png`, `x4pro-question.png` and `x4pro-filters.png` are 480 × 800
interface illustrations exported on 20 September 2026 from the approved Reading
room layout. They show the 32 px menu mark, 36 px question mark, Noto Sans 18 pt
question and inverted Next button. The webpage adds a schematic device bezel.
These are layout renders, not photographs or captures from physical hardware;
the surrounding CrossPoint Home screen is schematic and depends on its theme.

The question is copied verbatim from `website/placeholder-corpus/en/questions.yaml`.
It is an existing layout fixture, not part of the empty production English corpus.
Bitmap fonts come from the public CrossPoint font assets (Noto Sans / Ubuntu,
under their bundled OFL / Ubuntu Font License). The symbol is the public
`website/public/brand/cicala-mark.svg`. The firmware implementation lives in
[`CicalaActivity.cpp`](https://github.com/giacomomellone/cicala-crosspoint/blob/cicala/src/activities/cicala/CicalaActivity.cpp).
Keep these selected, approved illustrations aligned with that implementation.

## Rev B renders

The current device page imports four native Rev B enclosure PNGs directly from
`hardware/rev_b/case/renders/`: assembly, exploded, inside and section.
The dedicated button-design and carrier views are not displayed.
Generate them with `just hw-rev-b-export --renders`. Their source hashes are recorded in
`hardware/rev_b/case/exports/manifest.json`. Astro generates responsive WebP
variants during the website build. They depict nominal CAD geometry of an
unbuilt engineering prototype.

The PCB section additionally imports `hardware/rev_b/pcb/renders/top.png` and
`bottom.png`, rendered directly by KiCad. The adjacent render README records
the command, source fingerprint and component-model limits. These are separate
from the OpenSCAD inside and section views.

When the hardware changes, regenerate the affected views and review the
dated status text in `website/src/i18n.ts` against `docs/hardware_rev_b.md`
and `docs/firmware_rev_a.md`.

## Archived Rev A PCB renders

`pcb-top.webp` and `pcb-bottom.webp` were rendered on 14 September 2026 from
`hardware/rev_a/pcb/cicala_rev_a.kicad_pcb` at commit `c46fb6e` using
KiCad 10 and its installed component models, including the project's local
component envelopes. They depict the prototype design; they do not establish
assembled fit or electrical operation. Hardware-derived images use
[CERN-OHL-S-2.0](../../../../LICENSE-HARDWARE).

From the repository root, render each side with `kicad-cli` on PATH:

```sh
kicad-cli pcb render --output /tmp/cicala-device-pcb-top.png \
  --width 1600 --height 1100 --side top --rotate '25,0,-20' \
  --background transparent --quality high \
  hardware/rev_a/pcb/cicala_rev_a.kicad_pcb
kicad-cli pcb render --output /tmp/cicala-device-pcb-bottom.png \
  --width 1600 --height 1100 --side bottom --rotate '25,0,-20' \
  --background transparent --quality high \
  hardware/rev_a/pcb/cicala_rev_a.kicad_pcb
```

Encode the PNGs as WebP at quality 92 with Sharp. These archived PCB images are
retained for Rev A reference; the current device page uses the Rev B renders.
