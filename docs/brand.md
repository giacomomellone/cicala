# Brand

Cicala is Italian for cicada. Cicadas fill Italian summers with a steady
background sound; the name gives the object a clear job: make a first voice
easier, invite reflection, and break silence without becoming the conversation.

## Mark, wordmark and type

The identity pairs the lowercase `cicala` wordmark with a minimal outline
cicada. The selected mark, Matched 10 with a wider body, draws two tapered
wings and a rounded, open head that suggests speech. Wings and head share one
10-unit stroke, so no line in the mark is heavier than another. The wings sit
2.5 units further from the centre line than the wing geometry itself, which
keeps the gap that suggests the body open at small sizes.
The cicada alone represents the identity in square formats.

The wordmark is Literata at normal weight with optical kerning and restrained
negative tracking, over a 50% dither rule the width of the lockup. The rule
carries the weight so the letterforms do not have to. The website keeps the
wordmark as live, selectable text. Use `Cicala` normally in prose.

Zilla Slab at weight 500 is the question typeface: slab strokes survive at one
bit, which is why e-readers ship serif faces. IBM Plex Mono is reserved for
navigation, controls, labels, metadata, and technical information. All three
fonts are self-hosted by the website; do not add an external font request.

The e-paper renders none of them. The panel draws the CFB bitmap fonts
(10 × 16, 15 × 24, 20 × 32) with a one-pixel overdraw for weight, so the device
mirrors the website's tone and not its typeface.

Keep clear space around the lockup at least equal to the height of its `c`.
Where a name will not fit, use the cicada alone. Keep the logo subordinate to
the question. Preserve the open head, the separation between the wings and
the round stroke ends; use the supplied artwork rather than redrawing it.

## Colour

The panel is one bit deep, so the website is too. There is no accent colour:
state is carried by inversion, by weight, or by the dither.

| Token         | Value     | Use                                    |
| ------------- | --------- | -------------------------------------- |
| `--paper`     | `#dedad2` | page and avatar background             |
| `--panel`     | `#ebeae4` | the question surface, and raised keys  |
| `--ink`       | `#100f0e` | wordmark, primary text, and every rule |
| `--ink-soft`  | `#4c4a45` | secondary text                         |
| `--ink-faint` | `#8f8c85` | separators, hints, and disabled text   |
| `--line`      | `#100f0e` | 1 px borders                           |

Solid ink is reserved for type and for a key under a finger. A saved question
is a filled heart, not a coloured one; a fault is heavier and underlined, not
red.

## Dither

One ordered pattern at four exact fractions of the 1-bit grid — 1/2, 1/4, 1/8,
1/16 — sized in multiples of `--dither-px`, one e-paper pixel. The website
shapes it with a gradient mask; the panel, which cannot fade a pattern, steps
down through the same densities instead.

| Use           | Density | Where                                               |
| ------------- | ------- | --------------------------------------------------- |
| Wordmark rule | 50%     | under the lockup, 2 px                              |
| Floor         | 25%     | rising from the bottom edge of the question surface |

The floor ends at 30% of the surface on the website and at
`CICALA_PANEL_FLOOR_PCT` — 12%, 15 of 122 rows — on the device. Do not fill a
surface uniformly: the panel has roughly 10:1 contrast to give and the question
needs it.

## Assets

The canonical vector source is `website/public/brand/cicala-mark.svg`.
Its three paths are shared by the website and the PCB artwork generator.
The website inlines it beside the live Literata wordmark. The source uses
`currentColor` so the mark follows its surrounding ink colour.

Regenerate the PNGs with `npm run brand-assets` in `website/`. Square assets
use the cicada, and the social preview uses the cicada and wordmark together.
Every size, including the 16 px favicon, retains the master paths and the
single stroke weight. Transparent wordmark-only PNGs remain available alongside
the complete logo lockups.

- [canonical SVG mark](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-mark.svg)
- [dark transparent logo](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-logo-dark.png)
- [reversed transparent logo](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-logo-reversed.png)
- [1024 px avatar](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-avatar-1024.png)
- [dark transparent wordmark](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-wordmark-dark.png)
- [reversed transparent wordmark](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-wordmark-reversed.png)
- [32 px favicon](https://github.com/giacomomellone/cicala/blob/main/website/public/favicon-32.png)
- [16 px favicon](https://github.com/giacomomellone/cicala/blob/main/website/public/favicon-16.png)
- [180 px Apple touch icon](https://github.com/giacomomellone/cicala/blob/main/website/public/apple-touch-icon.png)
- [1200×630 Open Graph image](https://github.com/giacomomellone/cicala/blob/main/website/public/og.png)

Every asset is monochrome.

## Device placement

The logo does not appear on the e-paper. The normal device face stays free of
branding, including the areas beside the Filters and Next controls. Do not add
startup, sleep, or device logo screens. The phone captive portal may use the
live-text wordmark and palette, but it must remain self-contained and make no
font or image requests. If a physical mark is useful later, use only an
optional small blind emboss on the underside or a concealed lower edge.

The PCB carries the cicada beside the existing underside wordmark on
`B.SilkS`, mirrored for reading from below. Its 8 mm square artwork is stroked
throughout at 0.513 mm, well above the 0.15 mm silkscreen minimum. Native KiCad
curves and lines preserve the SVG geometry.
Regeneration and the mark-only update command are documented in
`hardware/pcb/tools/assets/README.md`.

## Prohibited treatments

Do not add speech bubbles, sound waves, faces or other details to the approved
cicada. Do not close the opening in its head. Do not introduce a brand colour, add
gradients or glow, round a corner the device could not round, distort the
letterforms, or make the wordmark larger than the current question. Do not lay
the dither under a question at any density. The questions remain visually
dominant.
