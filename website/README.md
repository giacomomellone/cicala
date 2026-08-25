# cicala website

Astro 5, static output, vanilla TypeScript islands. No UI framework, no Tailwind, no third-party scripts. See spec §7 and [docs/design.md](../docs/design.md).

## Develop

From the repo root: `just website` (dev server), `just website-build`, `just test-website`. Or directly in this directory:

```sh
npm run data   # regenerate src/data/*.json from the question database
npm install
npm run dev
npm test       # vitest suite in tests/
```

`src/data/*.json` is generated and gitignored; if the build complains about missing data, run `npm run data` again.

## Budgets (spec §7.1)

- ≤ 60 KB gzipped JS total per page
- Lighthouse mobile ≥ 95 on the play page
- Only the active language's payload is fetched at runtime

## Structure

- `src/pages/`: play (`index`, `q/[id]`), `browse`, `contribute`, `device`, `deck`, `404`
- `src/lib/`: client islands (play/browse/contribute/deck controllers, storage, data loading)
- `src/i18n.ts`: all UI strings, one object per shipped language
- `src/styles/tokens.css`: the design tokens (DECIDED in spec §7.2; dark mode lands here in v2)
- `src/config.ts`: repo/site URLs (placeholder org, renamed in one commit later)
