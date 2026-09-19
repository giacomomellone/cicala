# Brand

Cicala is Italian for cicada. The identity draws on the heat of an Italian
summer: saturated orange and yellow, dark wine-coloured ink, and a table
photographed after lunch.

## Mark and type

The owner-selected cicada is a filled silhouette with two round eye counters
and a deep split between its wings. The canonical SVG is traced from the
supplied transparent artwork. It uses one even-odd path in a 1034-unit square;
the eyes remain transparent on every background. Preserve its proportions and
use the master path at every size.

The website places the mark inside an orange circle. The circle contains only
the cicada. Hover, focus or a tap gives it a short rocking movement; reduced
motion disables that movement. Small application headers use the circle alone
as the home link. The landing uses a large lowercase `cicala` wordmark.

The wordmark and website controls use Arial, falling back to Helvetica and
sans-serif. The wordmark is bold, with tight tracking. Questions use
self-hosted Zilla Slab 500. Keep question text large and give it a clear
reading surface. The e-paper retains its existing bitmap fonts and monochrome
rendering.

## Colour

| Token         | Value     | Use                                           |
| ------------- | --------- | --------------------------------------------- |
| `--orange`    | `#F46B45` | invitation, circular mark and primary actions |
| `--yellow`    | `#FFD23F` | landing masthead and app navigation           |
| `--paper`     | `#FFF2D6` | app background and footer                     |
| `--panel`     | `#FFF7E6` | question reading surface                      |
| `--ink`       | `#341E24` | primary text, marks and rules                 |
| `--ink-soft`  | `#60454A` | secondary text                                |
| `--ink-faint` | `#73585A` | hints on cream surfaces                       |

Use the dark ink for text on orange and yellow. Its contrast is 5.18:1 on
orange and 10.72:1 on yellow. Orange and yellow do not provide sufficient
contrast against each other for text. Active states also use borders or
weight. Focus indicators remain visible against each adjacent surface.

## Website

The landing at `/` uses a yellow masthead, the selected overhead table scene
on the left and an orange invitation on the right. On a phone these stack in
that order. Its only headline is “Grab a chair, I have a question for you”.
An arrow opens `/play`, with “Try a question” as its accessible label.

The application retains browse, filters, saving, sharing and contribution.
Info gives a short introduction; Device links to the hardware README while
development continues.
Direct question links stay at `/q/<id>`; drawing the next question returns to
`/play`. The application uses the same yellow header and orange actions while
keeping the question on cream. The website does not use decorative dither
behind question text. The physical device retains its existing dither.

The owner-selected landing photograph is an AI-generated scene, not a record
of a real event. Its source is `website/src/assets/landing/dopopranzo.webp`.
Astro emits responsive sizes from that source. Keep the green bottle, wine,
coffee traces and metal dish recognisable in crops. Do not add promotional
captions over the photograph.

## Assets

The canonical source is `website/public/brand/cicala-mark.svg`. The website
inlines it with `currentColor`, and the PCB artwork generator reads the same
path. Run `npm run brand-assets` in `website/` to regenerate the PNGs:

- `brand/cicala-avatar-1024.png`
- `brand/cicala-logo-dark.png` and `brand/cicala-logo-reversed.png`
- `brand/cicala-wordmark-dark.png` and `brand/cicala-wordmark-reversed.png`
- `favicon-16.png`, `favicon-32.png` and `apple-touch-icon.png`
- `og.png`

Square exports use the cicada; transparent lockups include the wordmark.
Do not redraw the mark for individual placements or fill its eye counters.

## Device placement

The device face and e-paper stay free of branding. Do not add startup, sleep
or device logo screens. The setup portal remains self-contained.

The PCB carries the same cicada on the silkscreen, mirrored where it is read
from below. Rev A uses an 8 mm square; Rev B uses 0.7 of that size on both
sides. At the nominal 8 mm size the filled mark is 6.34 mm wide and 7.98 mm
high; its eye counters are approximately 0.75 mm across. Rev B’s counters are
approximately 0.53 mm, above the 0.15 mm silkscreen minimum. The existing
small Literata fabrication wordmark remains alongside the new cicada.

KiCad represents each eye with a zero-width bridge to the outer polygon.
Regeneration is documented in `hardware/tools/pcb/assets/README.md`. Preserve
all electrical and mechanical geometry when updating the artwork, and
regenerate fabrication exports and board previews from the native source.
