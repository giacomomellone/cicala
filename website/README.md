# tischkarte website

Astro 5, static output, vanilla TypeScript islands — no UI framework, no Tailwind, no third-party scripts. See spec §7 and [docs/DESIGN.md](../docs/DESIGN.md).

## Develop

```sh
# 1. generate the data payloads from the question database (requires python3 + pyyaml + jsonschema)
npm run data

# 2. run the site
npm install
npm run dev
```

`src/data/*.json` is generated and gitignored; if the build complains about missing data, run `npm run data` again.

## Budgets (spec §7.1)

- ≤ 60 KB gzipped JS total per page
- Lighthouse mobile ≥ 95 on the play page
- Only the active language's payload is fetched at runtime

## Structure

- `src/pages/` — play (`index`, `q/[id]`), `browse`, `contribute`, `device`, `deck`, `404`
- `src/lib/` — client islands (play/browse/contribute/deck controllers, storage, data loading)
- `src/i18n.ts` — all UI strings, one object per shipped language
- `src/styles/tokens.css` — the design tokens (DECIDED in spec §7.2; dark mode lands here in v2)
- `src/config.ts` — repo/site URLs (placeholder org — one-commit rename)
