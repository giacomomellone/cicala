# Brand

Kveld uses a typographic identity so the current question remains the dominant
element. The website wordmark is live, selectable text. Raster assets exist for
contexts that cannot render the bundled fonts.

## Wordmark and type

The primary wordmark is lowercase `kveld` set in Literata at normal weight with
optical kerning and restrained negative tracking. Do not capitalize it inside
the wordmark. Use `Kveld` normally in prose.

Literata is also the question typeface. IBM Plex Mono is reserved for
navigation, controls, labels, metadata, and technical information. Both fonts
are self-hosted by the website; do not add an external font request.

Keep clear space around the wordmark at least equal to the width of its
lowercase `v`. Do not compress that space to make the mark fill a container. At
small sizes, keep the wordmark readable rather than enlarging it enough to
compete with a question. Use the avatar below when a square or very small
format cannot accommodate the full wordmark.

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

## Avatar and raster assets

There is no separate Kveld icon. Avatars and favicons use the lowercase
Literata `k` from the wordmark, with generous space on warm paper and a quiet
border. The letter must remain recognizable at 32×32 pixels.

- [1024 px avatar](https://github.com/giacomomellone/kveld/blob/main/website/public/brand/kveld-avatar-1024.png)
- [dark transparent wordmark](https://github.com/giacomomellone/kveld/blob/main/website/public/brand/kveld-wordmark-dark.png)
- [reversed transparent wordmark](https://github.com/giacomomellone/kveld/blob/main/website/public/brand/kveld-wordmark-reversed.png)
- [32 px favicon](https://github.com/giacomomellone/kveld/blob/main/website/public/favicon-32.png)
- [16 px favicon](https://github.com/giacomomellone/kveld/blob/main/website/public/favicon-16.png)
- [180 px Apple touch icon](https://github.com/giacomomellone/kveld/blob/main/website/public/apple-touch-icon.png)
- [1200×630 Open Graph image](https://github.com/giacomomellone/kveld/blob/main/website/public/og.png)

These are the production PNGs. Do not create or use an SVG logo.

## Device placement

The logo does not appear on the e-paper. The normal device face stays free of
branding, including the areas beside the Category and Next controls. Do not add
startup, sleep, or device logo screens. The phone captive portal may use the
live-text wordmark and palette, but it must remain self-contained and make no
font or image requests. If a physical mark is useful later, use only an
optional small blind emboss on the underside or a concealed lower edge.

## Prohibited treatments

Do not use a question mark, pictogram, speech bubble, table symbol, moon,
colourful tile, `KV` ligature, decorative dot, or another metaphorical mark.
Do not put the wordmark in rust, add gradients or glow, introduce another brand
colour, distort the letterforms, or turn the full reference presentation board
into a production asset. The questions should remain visually dominant.
