# Brand

Cicala is Italian for cicada. Cicadas fill Italian summers with a steady
background sound; the name gives the object a clear job: make a first voice
easier, invite reflection, and break silence without becoming the conversation.

## Mark, wordmark, and type

The identity pairs a lowercase `cicala` wordmark with a minimal top-view cicada.
The two open wings also read as sound moving away from a central body. Use the
mark alone for square and very small formats. Use the horizontal lockup when the
name needs to be learned.

The wordmark is Literata at normal weight with optical kerning and restrained
negative tracking. The website keeps it as live, selectable text. Use `Cicala`
normally in prose.

Literata is also the question typeface. IBM Plex Mono is reserved for
navigation, controls, labels, metadata, and technical information. Both fonts
are self-hosted by the website; do not add an external font request.

Keep clear space around the mark or lockup at least equal to the mark's body
width. At small sizes, use the mark rather than enlarging the lockup enough to
compete with a question.

## Colour

| Token            | Value     | Use                                       |
| ---------------- | --------- | ----------------------------------------- |
| `--paper`        | `#faf8f2` | page and avatar background                |
| `--paper-raised` | `#fffffe` | raised surfaces                           |
| `--ink`          | `#1f1f1d` | wordmark and primary text                 |
| `--ink-soft`     | `#5f5e5a` | secondary text                            |
| `--ink-faint`    | `#b0ad9f` | hints and disabled text                   |
| `--line`         | `#e3e0d6` | restrained borders                        |
| `--accent`       | `#c24a22` | interaction, selection, hearts, and focus |

The wordmark is monochrome charcoal on warm paper. The reversed treatment uses
warm paper on charcoal. Rust is a functional interface accent and is not part
of the wordmark.

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

The mark and wordmark remain monochrome. The website may make the two wings
open by a few degrees on entry or hover. Do not loop the motion: the logo is not
an activity indicator.

## Device placement

The logo does not appear on the e-paper. The normal device face stays free of
branding, including the areas beside the Category and Next controls. Do not add
startup, sleep, or device logo screens. The phone captive portal may use the
live-text wordmark and palette, but it must remain self-contained and make no
font or image requests. If a physical mark is useful later, use only an
optional small blind emboss on the underside or a concealed lower edge.

## Prohibited treatments

Do not add eyes, legs, realistic veining, sound-wave ornaments, a speech bubble,
or a second symbol to the cicada. Do not put the wordmark in rust, add gradients
or glow, introduce another brand colour, distort the letterforms, or make the
mark larger than the current question. The questions remain visually dominant.
