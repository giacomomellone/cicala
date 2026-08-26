# Brand

Cicala is Italian for cicada. Cicadas fill Italian summers with a steady
background sound; the name gives the object a clear job: make a first voice
easier, invite reflection, and break silence without becoming the conversation.

## Mark, wordmark, and type

The identity is a lowercase `cicala` wordmark. The minimal top-view cicada —
whose two open wings also read as sound moving away from a central body — is the
square-format mark: it carries the favicon, the avatar, and the Open Graph
image, where there is no room to read a name. It does not appear beside the
wordmark. A name and a symbol saying the same thing at the same time is one of
them too many, and the header has a question to get out of the way of.

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

Keep clear space around the mark or the wordmark at least equal to the mark's
body width. Where a name will not fit, use the mark; never enlarge the wordmark
enough to compete with a question.

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

The SVG mark is the editable source. The wordmark stays live text on the web;
PNG lockups exist for services that cannot load the bundled font. Regenerate all
raster assets from the checked-in mark and font with `npm run brand-assets` in
`website/`.

- [cicada mark](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-mark.svg)
- [1024 px avatar](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-avatar-1024.png)
- [dark transparent wordmark](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-wordmark-dark.png)
- [reversed transparent wordmark](https://github.com/giacomomellone/cicala/blob/main/website/public/brand/cicala-wordmark-reversed.png)
- [32 px favicon](https://github.com/giacomomellone/cicala/blob/main/website/public/favicon-32.png)
- [16 px favicon](https://github.com/giacomomellone/cicala/blob/main/website/public/favicon-16.png)
- [180 px Apple touch icon](https://github.com/giacomomellone/cicala/blob/main/website/public/apple-touch-icon.png)
- [1200×630 Open Graph image](https://github.com/giacomomellone/cicala/blob/main/website/public/og.png)

The mark and wordmark remain monochrome.

## Device placement

The logo does not appear on the e-paper. The normal device face stays free of
branding, including the areas beside the Category and Next controls. Do not add
startup, sleep, or device logo screens. The phone captive portal may use the
live-text wordmark and palette, but it must remain self-contained and make no
font or image requests. If a physical mark is useful later, use only an
optional small blind emboss on the underside or a concealed lower edge.

## Prohibited treatments

Do not add eyes, legs, realistic veining, sound-wave ornaments, a speech bubble,
or a second symbol to the cicada. Do not set the mark beside the wordmark, put
the mark in the site header, introduce a brand colour, add
gradients or glow, round a corner the device could not round, distort the
letterforms, or make the mark larger than the current question. Do not lay the
dither under a question at any density. The questions remain visually dominant.
