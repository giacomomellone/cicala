# Placeholder corpus

Fixture questions for website work, kept out of `questions/`. The shipped
corpora are empty while the English database is being rewritten, and an empty
payload hides most of the UI: play has nothing to draw, browse and deck render
zero rows, and permalinks resolve to nothing.

`just website-placeholder` (dev server) and `just website-build-placeholder`
(production build) generate `website/src/data/*.json` from this tree instead of
`questions/`. `just data`, `just website` and `just website-build` all rebuild
those payloads from `questions/`, so they drop the fixtures again; the payloads
are gitignored either way.

The fixtures cover what the pages branch on: depth 1 to 3, every tag including
`dark` and `sexual`, entries with and without tags, two lines near the
140-character limit for wrapping, and German and Italian entries carrying
`origin` plus `translated_by: google` so the machine-translation notice appears.

These lines were written to fill a layout, not to enter the database. They are
never shipped, never bundled for the device, and never a source for
`questions/`, which the [Human Reserved policy](../../README.md#ai-policy)
governs.
