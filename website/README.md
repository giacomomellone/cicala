# cicala website

Astro 5, static output, vanilla TypeScript islands, and one Cloudflare Pages Function at `/api/suggestions`. No UI framework or Tailwind. Play and browse load no third-party scripts; the suggestion page loads Cloudflare Turnstile. See [docs/design.md](../docs/design.md) and [native submission setup](../docs/native_submission_setup.md).

The suggestion form requires confirmation that the contributor wrote the question without generative AI, following the [Human Reserved policy](../README.md#ai-policy), alongside the CC0 dedication. The API requires `humanWritten: true` and records the confirmation in the review issue.

## Develop

From the repo root: `just website` (dev server, available on the local network), `just website-build`, `just test-website`. Astro prints the network URL to open on a phone connected to the same network. Or directly in this directory:

```sh
npm run data   # regenerate src/data/*.json from the question database
npm install
npm run dev
npm test       # vitest suite in tests/
```

`src/data/*.json` is generated and gitignored; if the build complains about missing data, run `npm run data` again.

The shipped corpora are currently empty, so play, browse and deck render nothing. `just website-placeholder` (dev server) and `just website-build-placeholder` (production build) run the same pages against [placeholder-corpus/](placeholder-corpus/README.md), which is layout filler and never shipped content. Plain `just website` and `just website-build` rebuild the payloads from `questions/`, so running one of them after a placeholder build drops the fixtures again.

## Budgets (spec §7.1)

- ≤ 60 KB gzipped JS total per page
- Lighthouse mobile ≥ 95 on the play page
- Only the active language's payload is fetched at runtime

## Structure

- `src/pages/`: play (`index`, `q/[id]`), `browse`, `suggest`, `device`, `deck`, `404`
- `src/lib/`: client islands (play/browse/suggest/deck controllers, storage, data loading)
- `functions/api/suggestions.ts`: same-origin submission API, Turnstile verification, and GitHub App client
- `src/i18n.ts`: all UI strings, one object per shipped language
- `src/styles/tokens.css`: the design tokens (DECIDED in spec §7.2; dark mode lands here in v2)
- `src/config.ts`: repo/site URLs (placeholder org, renamed in one commit later)
